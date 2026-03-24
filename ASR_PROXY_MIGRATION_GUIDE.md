# MCU + Proxy + XFYUN ASR 迁移指南

## 1. 方案总览

这套方案的核心是把复杂能力放在代理端：

1. MCU 只做采样、上传音频、接收命令、执行命令。
2. 代理负责 TLS + WebSocket 握手、解析讯飞 JSON、命令匹配、纠错与相似匹配。
3. 代理把命令用纯文本行协议下发给 MCU：
   - `CMD:FORWARD 10M\n`
   - `CMD:HORN 2 SECONDS\n`

这样做的好处：

1. MCU 代码更轻，升级词表和纠错只改 Python。
2. 云端协议变化时，只改代理，不改 MCU。
3. 命令识别策略可持续迭代（别名、纠错、相似匹配、规则匹配）。

---

## 2. 端到端流程

1. MCU 上电初始化，进入 `audio_loop()` 主循环。
2. MCU 连接本地代理 TCP 端口（默认 `8888`）。
3. 代理与讯飞建立 TLS + WS 握手。
4. 代理返回 `READY\n` 给 MCU。
5. MCU 开始发送 WebSocket 文本帧，帧内是讯飞要求的 JSON（包含 base64 音频）。
6. 代理收到讯飞识别结果后提取文本，累计到 `asr_accum_text`。
7. 代理执行 `match_commands()`：
   - 统一归一化
   - 最长匹配
   - 鸣笛规则匹配
   - 有限相似匹配
8. 代理把命中命令逐条下发给 MCU（`CMD:...\n`）。
9. MCU 解析每一条 CMD，加入显示历史并执行任务。

---

## 3. 关键文件与职责

1. `xfyun_proxy.py`
- 代理主程序。
- 负责 TLS、WS、JSON 解析、命令匹配、命令下发。

2. `code/xf_asr/asr_audio.c`
- MCU 语音主流程：采样、分帧发送、结果接收、屏幕显示。
- 当前已改成较多非阻塞节拍推进。

3. `code/xf_asr/websocket_client.c`
- MCU 侧 WebSocket 帧封装/解封。
- 对接代理连接与收发。

4. `code/xf_asr/asr_audio.h`
- 语音模块接口（`audio_init/audio_loop/audio_callback`）。

5. `code/xf_asr/websocket_client.h`
- 网络层接口声明。

6. `user/cpu0_main.c`
- 主函数入口，初始化后持续调用 `audio_loop()`。

7. `libraries/zf_device/zf_device_ips200.h`
- 屏幕引脚与 SPI 配置（你已切到 P15 组并验证可亮屏）。

---

## 4. 协议约定（必须保持一致）

### 4.1 MCU -> 代理

- TCP 连接，内容是 MCU 构造的 WebSocket 帧（文本帧）。
- 文本 payload 是讯飞 IAT 要求的 JSON：
  - status 0: 首帧
  - status 1: 中间帧
  - status 2: 结束帧

### 4.2 代理 -> MCU

- 文本行协议，UTF-8：
  - `READY\n`
  - `CMD:TURN LEFT\n`
  - `CMD:HORN 3 TIMES\n`

注意：

1. 一次网络包可能包含多条 `CMD:`。
2. 一条 `CMD:` 也可能被拆包。
3. MCU 必须做流式缓存按行解析，不能按单包一次性假设完整。

---

## 5. 代理侧关键代码（可直接复用）

以下片段来自 `xfyun_proxy.py` 当前实现思路，附中文注释说明。

### 5.1 归一化与规则匹配骨架

```python
# 1) 命令标准表：保留最小标准短语，不堆冗余别名
CMD_MAP = [
    ("前进10米", "FORWARD 10M"),
    ("后退10米", "BACKWARD 10M"),
    ("蛇形前进10米", "SNAKE FORWARD 10M"),
    ("蛇形后退10米", "SNAKE BACKWARD 10M"),
    ("急促鸣笛", "HORN RAPID"),
    # ... 其他命令
]

# 2) 统一归一化替换：把常见误识别/同义词折叠成标准写法
CANONICAL_REPLACEMENTS = [
    ("急速", "急促"),
    ("蛇行", "蛇形"),
    ("前行", "前进"),
    ("门洞一", "门洞1"),
    ("十米", "10米"),
]

def normalize_text(text: str) -> str:
    # 去空格和标点，降低口语停顿影响
    return re.sub(r"[\s,\.!\?:;，。！？：；、（）()]+", "", text)

def canonicalize_text(text: str) -> str:
    # 先 normalize，再按规则替换
    canonical = normalize_text(text)
    for src, dst in NORMALIZED_CANONICAL_REPLACEMENTS:
        canonical = canonical.replace(src, dst)
    return canonical

CANONICAL_CMD_MAP = [(canonicalize_text(p), c) for p, c in CMD_MAP]
```

### 5.2 鸣笛规则匹配（避免误触发）

```python
def horn_fuzzy_match(normalized: str):
    cmds = []
    if "鸣笛" not in normalized:
        return cmds

    # 只匹配紧邻“秒”前面的数字，避免“一秒”误触发“一声”
    second_tokens = re.findall(r"([一二两三123])秒", normalized)
    for token in second_tokens:
        if token in ("一", "1"):
            cmds.append("HORN 1 SECOND")
        elif token in ("两", "二", "2"):
            cmds.append("HORN 2 SECONDS")
        elif token in ("三", "3"):
            cmds.append("HORN 3 SECONDS")

    # 只匹配紧邻“声”前面的数字
    count_tokens = re.findall(r"([一二两三四1234])声", normalized)
    for token in count_tokens:
        if token in ("一", "1"):
            cmds.append("HORN 1 TIME")
        elif token in ("两", "二", "2"):
            cmds.append("HORN 2 TIMES")
        elif token in ("三", "3"):
            cmds.append("HORN 3 TIMES")
        elif token in ("四", "4"):
            cmds.append("HORN 4 TIMES")

    return cmds
```

### 5.3 命令主匹配函数（顺序 + 最长 + 兜底）

```python
def match_commands(text: str):
    normalized = canonicalize_text(text)
    out = []
    pos = 0

    # 顺序扫描 + 最长匹配
    while pos < len(normalized):
        best = None
        best_len = 0
        for n_phrase, cmd in CANONICAL_CMD_MAP:
            if normalized.startswith(n_phrase, pos) and len(n_phrase) > best_len:
                best = cmd
                best_len = len(n_phrase)

        # 有限相似匹配：仅对核心关键词开放 1 字误差
        if best is None:
            for n_phrase, cmd in CANONICAL_CMD_MAP:
                if not any(k in n_phrase for k in ("鸣笛", "门洞", "前进", "后退", "蛇形")):
                    continue
                seg = normalized[pos:pos + len(n_phrase)]
                if len(seg) == len(n_phrase) and one_char_distance(seg, n_phrase):
                    best = cmd
                    best_len = len(n_phrase)
                    break

        if best is not None:
            out.append(best)
            pos += best_len
        else:
            pos += 1

    # 规则兜底 + 去重保序
    out.extend(horn_fuzzy_match(normalized))
    uniq = []
    seen = set()
    for c in out:
        if c not in seen:
            seen.add(c)
            uniq.append(c)
    return uniq
```

### 5.4 代理下发命令

```python
# asr_accum_text 持续累计识别文本
asr_accum_text += text
for cmd in match_commands(asr_accum_text):
    if cmd not in sent_cmds:     # 会话内去重
        sent_cmds.add(cmd)
        line = f"CMD:{cmd}\n".encode("utf-8")
        client_socket.sendall(line)
```

---

## 6. MCU 侧关键代码（可直接复用）

### 6.1 主循环挂载

`user/cpu0_main.c` 中保持：

```c
system_start();
audio_init();
while (TRUE)
{
    audio_loop();
}
```

说明：

1. `system_start()` 提供 `system_getval_ms()` 基准。
2. `audio_loop()` 内部已经做节拍状态机，不应再外层硬塞固定大延时。

### 6.2 代理命令流式解析（重点）

`code/xf_asr/asr_audio.c` 中应保留：

1. 接收缓存（跨包拼接）
2. 按换行切分命令
3. 每行解析 `CMD:` 前缀

核心思路：

```c
// 伪代码
append(chunk, cache)
while (cache contains '\n') {
    line = pop_one_line(cache)
    if (line starts with "CMD:") {
        append_history(cmd)
    }
}
```

这一步是你修复“同包多条只显示一条”“拆包导致最后一条丢失”的关键。

### 6.3 非阻塞流程状态机

你当前 `audio_loop()` 已包含：

1. 启动阶段 WiFi 非阻塞重试
2. 服务器连接非阻塞重试
3. 结束阶段分相位（补发中间帧 -> 发结束帧 -> 等最终结果 -> WiFi 重连）

迁移时建议保持状态机结构，不要退回 `while + delay` 阻塞写法。

---

## 7. 迁移到新工程的最小步骤

1. 拷贝代理：`xfyun_proxy.py`
2. MCU 侧拷贝：
   - `code/xf_asr/asr_audio.c`
   - `code/xf_asr/asr_audio.h`
   - `code/xf_asr/websocket_client.c`
   - `code/xf_asr/websocket_client.h`
   - 依赖：`base64.*`、`asr_ctrl` 中网络参数
3. 主循环挂接：在新工程主循环持续调用 `audio_loop()`。
4. 屏幕适配：如不用 IPS200，把显示函数替换成你目标工程的 UI 输出函数。
5. 网络参数配置：
   - MCU 指向代理 IP + 端口
   - 代理配置讯飞 APP_ID/API_KEY/API_SECRET
6. 先做链路验证：
   - 代理启动后 MCU 连接是否收到 `READY`
   - 发送首帧是否成功
   - 代理是否打印 `[识别]` 与 `[代理] 下发命令`

---

## 8. 常见问题与排查

1. 代理下发了，屏幕只显示第一条
- MCU 未做流式行缓存，按单包解析导致。

2. 明明说“急促鸣笛”却没命中
- 看归一化规则是否覆盖了误识别词（如“急速”）。

3. 误下发 `HORN 1 TIME`
- 检查鸣笛兜底是否按“紧邻秒/声”解析，而不是全文搜数字。

4. 屏幕不亮但系统在跑
- 先核对 `RST/DC/CS/BLK` 引脚是否与硬件接线一致。

---

## 9. 建议的后续增强

1. 将 `CMD_MAP` 与归一化规则抽成 JSON 配置文件，运行时热加载。
2. 代理下发协议增加 `seq` 与 ACK，提高可靠性与可追踪性。
3. 增加会话超时与清理策略，避免长期累积文本导致误触发。
4. 给每条命令加置信日志，方便后期调参。
