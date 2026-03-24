#include "zf_common_headfile.h"
#include "websocket_client.h"
#include "asr_ctrl.h"
#include "zf_device_wifi_spi.h"

extern char temp_data[10240];

// 连接到代理服务器（通过 TCP）
bool websocket_client_connect(void)
{
    uint32_t deadline_ms = 0;

    printf("[WS] 通过代理连接到讯飞服务器\r\n");
    
    // 直接连接到代理服务器（TCP）
    printf("[WS] 连接代理 %s:%s\r\n", ASR_TARGET_IP, ASR_TARGET_PORT);
    if(wifi_spi_socket_connect("TCP", ASR_TARGET_IP, ASR_TARGET_PORT, ASR_LOCAL_PORT))
    {
        printf("[WS] 连接代理失败\r\n");
        return false;
    }
    printf("[WS] 已连接到代理服务器\r\n");
    
    // 给代理留出足够握手时间，降低误判超时导致的反复重连
    uint8_t ready_buf[16] = {0};
    deadline_ms = system_getval_ms() + 1500U;
    while(!(((int32_t)(system_getval_ms() - deadline_ms)) >= 0))
    {
        uint32_t len = wifi_spi_read_buffer(ready_buf, sizeof(ready_buf));
        if(len > 0 && strstr((char*)ready_buf, "READY"))
        {
            printf("[WS] 代理已就绪\r\n");
            return true;
        }
    }
    
    printf("[WS] 等待代理就绪超时\r\n");
    return false;
}

// 发送数据到代理服务器（代理会处理 WebSocket 封装）
bool websocket_client_send(const uint8_t* data, uint32_t len)
{
    memset(temp_data, 0, sizeof(temp_data));
    uint32_t frame_len = websocket_create_frame((uint8*)temp_data, data, len, 1, 1);
    return wifi_spi_send_buffer((uint8*)temp_data, frame_len) == 0;
}

bool websocket_client_receive(uint8_t* buffer)
{
    uint8_t raw[4096];
    char *cmd_ptr = NULL;
    memset(raw, 0, sizeof(raw));
    memset(buffer, 0, 4096);

    uint32_t bytes_received = wifi_spi_read_buffer(raw, sizeof(raw));

    if(bytes_received == 0)
    {
        printf("[WS] 未收到服务器数据\r\n");
        return false;
    }

    // 代理侧命令通道：支持包内任意位置出现 CMD:XXXX\n
    cmd_ptr = strstr((char *)raw, "CMD:");
    if(cmd_ptr != NULL)
    {
        uint32_t copy_len = (uint32_t)(bytes_received - (uint32_t)(cmd_ptr - (char *)raw));
        if(copy_len > 4095) copy_len = 4095;
        memcpy(buffer, cmd_ptr, copy_len);
        buffer[copy_len] = '\0';
        printf("[WS] 收到代理命令: %s\r\n", buffer);
        return true;
    }

    printf("[WS] 收到数据 len=%lu 前16字节=", (unsigned long)bytes_received);
    for(uint32_t i = 0; i < bytes_received && i < 16; i++)
    {
        uint8_t b = raw[i];
        if(b >= 32 && b <= 126) putchar(b); else printf("(%02X)", b);
    }
    printf("\r\n");

    // 尝试处理 “无 payload 的文本帧 + 后续一帧” 情形：若帧载荷为 0，则直接跳到下一个帧

    uint32_t offset = 0;
    uint32_t out_len = 0;
    while(offset + 2 <= bytes_received)
    {
        uint8_t b1 = raw[offset];
        uint8_t b2 = raw[offset + 1];
        uint8_t opcode = b1 & 0x0F;
        bool fin = (b1 & 0x80) != 0;
        bool masked = (b2 & 0x80) != 0;
        uint64_t payload_len = (uint64_t)(b2 & 0x7F);
        uint32_t idx = offset + 2;

        if(payload_len == 126)
        {
            if(bytes_received < idx + 2) break;
            payload_len = ((uint64_t)raw[idx] << 8) | raw[idx + 1];
            idx += 2;
        }
        else if(payload_len == 127)
        {
            if(bytes_received < idx + 8) break;
            payload_len = 0;
            for(int i = 0; i < 8; i++)
            {
                payload_len = (payload_len << 8) | raw[idx + i];
            }
            idx += 8;
        }

        uint8_t mask_key[4] = {0};
        if(masked)
        {
            if(bytes_received < idx + 4) break;
            memcpy(mask_key, raw + idx, 4);
            idx += 4;
        }

        if(bytes_received < idx) break;

        uint64_t available = bytes_received - idx;
        if(payload_len > available) payload_len = available; // 防止越界

        // 处理文本/二进制帧；控制帧直接跳过
        if(opcode == 0x1 || opcode == 0x2)
        {
            uint64_t copy_len = payload_len;
            if(out_len + copy_len > 4095) copy_len = 4095 - out_len;
            for(uint64_t i = 0; i < copy_len; i++)
            {
                uint8_t byte = raw[idx + i];
                if(masked) byte ^= mask_key[i % 4];
                buffer[out_len + i] = byte;
            }
            out_len += (uint32_t)copy_len;
        }

        // 计算下一个帧的起点；允许 payload_len 为 0 的帧被跳过
        uint64_t frame_total = (uint64_t)(idx - offset) + payload_len;
        if(frame_total == 0)
        {
            frame_total = (uint64_t)(idx - offset); // 遇到空载荷帧，至少前进头部长度
        }
        offset += (uint32_t)frame_total;

        // 如果帧未结束或已经填满缓冲，跳出
        if(!fin || out_len >= 4095) break;
    }

    buffer[out_len] = '\0';
    if(out_len == 0)
    {
        return false;
    }

    // 解帧后若存在 CMD，仅返回 CMD 起始及其后续内容
    cmd_ptr = strstr((char *)buffer, "CMD:");
    if(cmd_ptr != NULL && cmd_ptr != (char *)buffer)
    {
        size_t cmd_len = strlen(cmd_ptr);
        memmove(buffer, cmd_ptr, cmd_len + 1U);
    }
    return true;
}

// 关闭连接
void websocket_client_close()
{
    wifi_spi_socket_disconnect();
}

// WebSocket 数据帧创建函数
uint32_t websocket_create_frame(uint8_t* frame, const uint8_t* payload, uint64_t payload_len, uint8_t type, bool mask)
{
    uint8_t* ptr = frame;
    uint8_t mask_key[4] = {0};
//    uint64_t masked_payload_len = payload_len;

    *ptr++ = 0x80 | (type & 0x0F); // FIN=1, Opcode=操作码

    if(mask)
    {
        // 客户端每帧使用新的掩码键
        for(int i = 0; i < 4; i++)
        {
            mask_key[i] = adc_convert(RANDOM_NUM_ADC) % 256;    // 使用ADC生成随机数
        }
    }
    if(payload_len <= 125)
    {
        *ptr++ = (uint8_t)(payload_len | (mask ? 0x80 : 0x00)); // 设置 MASK 标志位
    }
    else if(payload_len <= 65535)
    {
        *ptr++ = 126 | (mask ? 0x80 : 0x00);                    // 标记使用 16 位长度，并设置 MASK 标志位
        *ptr++ = (uint8_t)((payload_len >> 8) & 0xFF);          // 高字节
        *ptr++ = (uint8_t)(payload_len & 0xFF);                 // 低字节
    }
    else
    {
        *ptr++ = 127 | (mask ? 0x80 : 0x00);                    // 标记使用 64 位长度，并设置 MASK 标志位
        // 将 64 位长度按大端序写入
        for(int i = 7; i >= 0; i--)
        {
            *ptr++ = (uint8_t)((payload_len >> (i * 8)) & 0xFF);
        }
    }

    // 添加掩码键（如果需要）
    if(mask)
    {
        memcpy(ptr, mask_key, 4);
        ptr += 4;
    }

    // 掩码有效载荷（如果需要）
    uint8_t* payload_start = ptr;
    for(uint64_t i = 0; i < payload_len; i++)
    {
        if(mask)
        {
            payload_start[i] = payload[i] ^ mask_key[i % 4];
        }
        else
        {
            payload_start[i] = payload[i];
        }
    }
    // 返回帧总长度
    return (uint32_t)((payload_start - frame) + payload_len);
}

