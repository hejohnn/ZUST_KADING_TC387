#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
讯飞语音识别 WebSocket 代理服务器
MCU 通过 TCP 连接到本代理，代理负责 TLS 加密并转发到讯飞服务器
"""

import socket
import ssl
import threading
import time
import hmac
import hashlib
import base64
from urllib.parse import urlencode, quote
from datetime import datetime, timezone
import json
import struct
import re


# 讯飞 API 配置
APP_ID = "0af3349a"
API_SECRET = "NzZjNDlhZDU0N2IxMmE0MWE3MmZiODVk"
API_KEY = "1fc5af901add57048bf2aed85c36457c"

# 代理配置
PROXY_HOST = "0.0.0.0"  # 监听所有网卡
PROXY_PORT = 8888       # MCU 连接到这个端口

# 讯飞服务器配置
XFYUN_HOST = "ws-api.xfyun.cn"
XFYUN_PORT = 443

# 命令映射（代理侧匹配，MCU 仅接收 CMD）
CMD_MAP = [
    ("打开左转向灯", "TURN ON LEFT SIGNAL"),
    ("打开右转向灯", "TURN ON RIGHT SIGNAL"),
    ("打开远光灯", "TURN ON HIGH BEAM"),
    ("打开近光灯", "TURN ON LOW BEAM"),
    ("打开雾灯", "TURN ON FOG LIGHT"),
    ("打开双闪灯", "TURN ON HAZARD"),
    ("打开车内照明灯", "TURN ON CABIN LIGHT"),
    ("打开雨刷器", "TURN ON WIPER"),
    ("长短鸣笛", "HORN LONG SHORT"),
    ("急促鸣笛", "HORN RAPID"),
    ("警报鸣笛", "HORN ALARM"),
    ("通过门洞1左侧", "PASS GATE1 LEFT"),
    ("通过门洞1", "PASS GATE1"),
    ("通过门洞2", "PASS GATE2"),
    ("通过门洞3右侧", "PASS GATE3 RIGHT"),
    ("通过门洞3", "PASS GATE3"),
    ("门洞1右侧返回", "RETURN GATE1 RIGHT"),
    ("门洞1返回", "RETURN GATE1"),
    ("门洞2返回", "RETURN GATE2"),
    ("门洞3左侧返回", "RETURN GATE3 LEFT"),
    ("门洞3返回", "RETURN GATE3"),
    ("前进10米", "FORWARD 10M"),
    ("后退10米", "BACKWARD 10M"),
    ("蛇形前进10米", "SNAKE FORWARD 10M"),
    ("蛇形后退10米", "SNAKE BACKWARD 10M"),
    ("逆时针转一圈", "CCW ONE CIRCLE"),
    ("顺时针转一圈", "CW ONE CIRCLE"),
    ("左转", "TURN LEFT"),
    ("右转", "TURN RIGHT"),
]

def one_char_distance(a: str, b: str) -> bool:
    """仅允许同长度且最多 1 个字符不同"""
    if len(a) != len(b):
        return False
    diff = 0
    for x, y in zip(a, b):
        if x != y:
            diff += 1
            if diff > 1:
                return False
    return diff == 1

def normalize_text(text: str) -> str:
    """去掉空白和常见标点，提升匹配鲁棒性"""
    return re.sub(r"[\s,\.!\?:;，。！？：；、（）()]+", "", text)

# 文本统一归一化规则：中文数字、近音词、常见同义词都先折叠到标准写法
CANONICAL_REPLACEMENTS = [
    ("极速", "急促"),
    ("急速", "急促"),
    ("急述", "急促"),
    ("急处", "急促"),
    ("蛇行", "蛇形"),
    ("前行", "前进"),
    ("门洞一", "门洞1"),
    ("门洞二", "门洞2"),
    ("门洞两", "门洞2"),
    ("门洞三", "门洞3"),
    ("一秒钟", "1秒钟"),
    ("一秒", "1秒"),
    ("二秒钟", "2秒钟"),
    ("二秒", "2秒"),
    ("两秒钟", "2秒钟"),
    ("两秒", "2秒"),
    ("三秒钟", "3秒钟"),
    ("三秒", "3秒"),
    ("一声", "1声"),
    ("二声", "2声"),
    ("两声", "2声"),
    ("三声", "3声"),
    ("四声", "4声"),
    ("一圈", "1圈"),
    ("十米", "10米"),
]

NORMALIZED_CANONICAL_REPLACEMENTS = [
    (normalize_text(src), normalize_text(dst))
    for src, dst in sorted(CANONICAL_REPLACEMENTS, key=lambda item: len(item[0]), reverse=True)
]

def canonicalize_text(text: str) -> str:
    """将输入文本折叠到标准写法，降低误识别和表达差异影响"""
    canonical = normalize_text(text)
    for src, dst in NORMALIZED_CANONICAL_REPLACEMENTS:
        if src in canonical:
            canonical = canonical.replace(src, dst)
    return canonical

CANONICAL_CMD_MAP = [(canonicalize_text(phrase), cmd) for phrase, cmd in CMD_MAP]

def horn_fuzzy_match(normalized: str):
    cmds = []
    if "鸣笛" not in normalized:
        return cmds

    second_tokens = re.finditer(r"([一二两三123])秒", normalized)
    for m in second_tokens:
        token = m.group(1)
        pos = m.start()
        if token in ("一", "1"):
            cmds.append((pos, "HORN 1 SECOND"))
        elif token in ("两", "二", "2"):
            cmds.append((pos, "HORN 2 SECONDS"))
        elif token in ("三", "3"):
            cmds.append((pos, "HORN 3 SECONDS"))

    count_tokens = re.finditer(r"([一二两三四1234])声", normalized)
    for m in count_tokens:
        token = m.group(1)
        pos = m.start()
        if token in ("一", "1"):
            cmds.append((pos, "HORN 1 TIME"))
        elif token in ("两", "二", "2"):
            cmds.append((pos, "HORN 2 TIMES"))
        elif token in ("三", "3"):
            cmds.append((pos, "HORN 3 TIMES"))
        elif token in ("四", "4"):
            cmds.append((pos, "HORN 4 TIMES"))

    return cmds

def match_commands(text: str):
    normalized = canonicalize_text(text)
    out = []
    pos = 0

    # 优先最长匹配，按文本顺序提取命令
    while pos < len(normalized):
        best = None
        best_len = 0
        for n_phrase, cmd in CANONICAL_CMD_MAP:
            if normalized.startswith(n_phrase, pos) and len(n_phrase) > best_len:
                best = cmd
                best_len = len(n_phrase)

        # 相似匹配兜底：仅对核心命令词允许 1 字误差，降低误匹配风险
        if best is None:
            for n_phrase, cmd in CANONICAL_CMD_MAP:
                if not any(keyword in n_phrase for keyword in ("鸣笛", "门洞", "前进", "后退", "蛇形")):
                    continue
                seg = normalized[pos:pos + len(n_phrase)]
                if len(seg) == len(n_phrase) and one_char_distance(seg, n_phrase):
                    best = cmd
                    best_len = len(n_phrase)
                    break

        if best is not None:
            out.append((pos, best))
            pos += best_len
        else:
            pos += 1

    out.extend(horn_fuzzy_match(normalized))
    out.sort(key=lambda x: x[0])

    # 去重保序
    uniq = []
    seen = set()
    for _, c in out:
        if c not in seen:
            seen.add(c)
            uniq.append(c)
    return uniq

def generate_websocket_url():
    """生成讯飞 WebSocket URL（带鉴权）"""
    # 获取当前时间（RFC1123 格式）
    now = datetime.now(timezone.utc)
    date = now.strftime('%a, %d %b %Y %H:%M:%S GMT')
    
    # 拼接签名原文
    signature_origin = f"host: {XFYUN_HOST}\ndate: {date}\nGET /v2/iat HTTP/1.1"
    
    # 计算 HMAC-SHA256 签名
    signature_sha = hmac.new(
        API_SECRET.encode('utf-8'),
        signature_origin.encode('utf-8'),
        hashlib.sha256
    ).digest()
    signature = base64.b64encode(signature_sha).decode('utf-8')
    
    # 构建 authorization
    authorization_origin = f'api_key="{API_KEY}", algorithm="hmac-sha256", headers="host date request-line", signature="{signature}"'
    authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode('utf-8')
    
    # URL 编码参数
    params = {
        'authorization': authorization,
        'date': date,
        'host': XFYUN_HOST
    }
    
    url = f"/v2/iat?{urlencode(params)}"
    return url, date

def handle_client(client_socket, client_addr):
    """处理单个 MCU 客户端连接"""
    print(f"[代理] MCU 客户端已连接: {client_addr}")
    
    try:
        asr_accum_text = ""
        sent_cmds = set()

        # 生成 WebSocket URL
        ws_path, date_str = generate_websocket_url()
        print(f"[代理] WebSocket 路径: {ws_path[:100]}...")
        
        # 连接到讯飞服务器（TLS）
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        
        with socket.create_connection((XFYUN_HOST, XFYUN_PORT)) as sock:
            with context.wrap_socket(sock, server_hostname=XFYUN_HOST) as tls_sock:
                print(f"[代理] 已连接到讯飞服务器 {XFYUN_HOST}:{XFYUN_PORT}")
                
                # 发送 WebSocket 握手请求
                handshake = (
                    f"GET {ws_path} HTTP/1.1\r\n"
                    f"Host: {XFYUN_HOST}\r\n"
                    f"Date: {date_str}\r\n"
                    f"Upgrade: websocket\r\n"
                    f"Connection: Upgrade\r\n"
                    f"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                    f"Sec-WebSocket-Version: 13\r\n\r\n"
                )
                tls_sock.sendall(handshake.encode('utf-8'))
                print("[代理] 已发送 WebSocket 握手请求")
                
                # 接收握手响应
                response = tls_sock.recv(4096)
                print(f"[代理] 握手响应: {response[:200]}")
                
                if b"101" not in response:
                    print("[代理] WebSocket 握手失败")
                    client_socket.sendall(b"ERROR: WebSocket handshake failed\n")
                    return
                
                print("[代理] WebSocket 握手成功")
                client_socket.sendall(b"READY\n")  # 通知 MCU 准备就绪
                
                # 双向转发数据
                def forward_mcu_to_server():
                    """MCU → 讯飞服务器"""
                    try:
                        while True:
                            data = client_socket.recv(4096)
                            if not data:
                                break
                            tls_sock.sendall(data)
                            print(f"[代理] MCU→服务器: {len(data)} 字节")
                    except Exception as e:
                        print(f"[代理] MCU→服务器转发错误: {e}")
                
                def forward_server_to_mcu():
                    """讯飞服务器 → MCU（解析识别结果）"""
                    nonlocal asr_accum_text, sent_cmds
                    try:
                        while True:
                            data = tls_sock.recv(4096)
                            if not data:
                                break
                            
                                        # 打印原始数据（用于调试）
                            print(f"[调试] 原始数据: {data[:100]}")
                            
                            # 尝试解析 WebSocket 帧并提取识别结果
                            try:
                                if len(data) >= 2:
                                    # 简单解析 WebSocket 帧
                                    payload_start = 2
                                    opcode = data[0] & 0x0F
                                    payload_len = data[1] & 0x7F
                                    
                                    if payload_len == 126:
                                        payload_start = 4
                                        payload_len = struct.unpack('>H', data[2:4])[0]
                                    elif payload_len == 127:
                                        payload_start = 10
                                        payload_len = struct.unpack('>Q', data[2:10])[0]
                                    
                                    if len(data) >= payload_start + payload_len:
                                        payload = data[payload_start:payload_start + payload_len]
                                        print(f"[调试] Payload: {payload}")

                                        if opcode == 0x1:  # 文本帧
                                            try:
                                                text_payload = payload.decode('utf-8')
                                                json_data = json.loads(text_payload)
                                                print(f"[调试] JSON: {json_data}")
                                                # 提取识别结果
                                                if 'data' in json_data and 'result' in json_data['data']:
                                                    ws = json_data['data']['result']['ws']
                                                    text = ''.join([w['cw'][0]['w'] for w in ws])
                                                    print(f"[识别] {text}")

                                                    # 累计识别文本，仅在最终结果(status=2)时统一下发，避免中间结果导致顺序抖动
                                                    asr_accum_text += text
                                                    status = json_data['data'].get('status', 1)
                                                    if status == 2:
                                                        print(f"[代理] 最终识别文本: {asr_accum_text}")
                                                        for cmd in match_commands(asr_accum_text):
                                                            if cmd not in sent_cmds:
                                                                sent_cmds.add(cmd)
                                                                line = f"CMD:{cmd}\n".encode('utf-8')
                                                                client_socket.sendall(line)
                                                                print(f"[代理] 下发命令 -> MCU: {cmd}")

                                                        # 一轮识别会话结束后清理缓存，准备下一轮
                                                        asr_accum_text = ""
                                                        sent_cmds.clear()
                                            except Exception as e:
                                                print(f"[调试] 文本帧JSON解析失败: {e}")
                                        elif opcode == 0x8:  # 关闭帧
                                            close_code = None
                                            close_reason = ""
                                            if len(payload) >= 2:
                                                close_code = struct.unpack('>H', payload[:2])[0]
                                                if len(payload) > 2:
                                                    close_reason = payload[2:].decode('utf-8', errors='replace')
                                            print(f"[代理] 收到关闭帧: code={close_code}, reason={close_reason}")
                                        elif opcode == 0x9:
                                            print("[代理] 收到PING帧")
                                        elif opcode == 0xA:
                                            print("[代理] 收到PONG帧")
                                        else:
                                            print(f"[代理] 收到opcode={opcode}帧，payload_len={payload_len}")
                            except Exception as e:
                                print(f"[调试] 帧解析失败: {e}")

                            if len(data) >= 2 and (data[0] & 0x0F) == 0x8:
                                break
                    except Exception as e:
                        print(f"[代理] 服务器→MCU转发错误: {e}")
                
                # 启动双向转发线程
                t1 = threading.Thread(target=forward_mcu_to_server, daemon=True)
                t2 = threading.Thread(target=forward_server_to_mcu, daemon=True)
                t1.start()
                t2.start()
                
                # 等待任一方向断开
                t1.join()
                t2.join()
                
    except Exception as e:
        print(f"[代理] 处理客户端错误: {e}")
        import traceback
        traceback.print_exc()
    finally:
        client_socket.close()
        print(f"[代理] 客户端 {client_addr} 已断开")

def main():
    """启动代理服务器"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind((PROXY_HOST, PROXY_PORT))
    except Exception as e:
        print(f"[错误] 无法绑定端口 {PROXY_PORT}: {e}")
        print("请检查端口是否被占用，或尝试更换端口")
        return
    server.listen(5)
    server.settimeout(None)  # 阻塞模式
    
    import socket as sock_module
    hostname = sock_module.gethostname()
    local_ip = sock_module.gethostbyname(hostname)
    
    print("="*60)
    print("讯飞语音识别代理服务器已启动")
    print(f"监听地址: {PROXY_HOST}:{PROXY_PORT}")
    print(f"本机 IP: {local_ip}")
    print(f"目标服务器: {XFYUN_HOST}:{XFYUN_PORT}")
    print("="*60)
    print(f"\nMCU 应连接到: {local_ip}:{PROXY_PORT}")
    print("等待 MCU 连接...\n")
    
    try:
        while True:
            client_sock, client_addr = server.accept()
            # 为每个客户端创建新线程
            thread = threading.Thread(
                target=handle_client,
                args=(client_sock, client_addr),
                daemon=True
            )
            thread.start()
    except KeyboardInterrupt:
        print("\n[代理] 服务器关闭")
    finally:
        server.close()

if __name__ == "__main__":
    main()