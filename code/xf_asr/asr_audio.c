#include "asr_audio.h"
#include "websocket_client.h"
#include "zf_device_wifi_spi.h"
#include "zf_device_ips200.h"
#include "zf_driver_pit.h"
#include "asr_ctrl.h"
#include "base64.h"
#include "zf_common_font.h"
#include "loudspeak.h"

// 显式声明常用外部函数，避免无原型警告
extern void pit_disable(pit_index_enum pit_index);
extern void pit_enable(pit_index_enum pit_index);
extern uint8 wifi_spi_init(char *wifi_ssid, char *pass_word);

#include <string.h>

// 置 1 仅做连通性测试，跳过语音采集与上传流程
#define ASR_CONNECT_TEST 0

// 置 1 开启屏幕显示，置 0 跳过屏幕操作
#define ASR_SCREEN_ENABLE 1

// 音频发送缓冲尺寸（采样与Base64后的拼装缓冲大小）
#ifndef ASR_SEND_DATA_MAX_LENTH
#define ASR_SEND_DATA_MAX_LENTH 1600
#endif

// 采样分片阈值：每采集到该数量样本就触发一次发送（8kHz -> 800 样本约 0.1 秒）
#ifndef ASR_CHUNK_SAMPLES
#define ASR_CHUNK_SAMPLES 800
#endif

// 音频 FIFO 深度倍数：数值越大，主循环偶发阻塞时越不容易丢前段语音
#ifndef ASR_FIFO_DEPTH_FACTOR
#define ASR_FIFO_DEPTH_FACTOR 8
#endif

#define ASR_LCD_MAX_COLS 30
#define ASR_LCD_RESULT_COLS 26
#define ASR_CMD_HISTORY_MAX 8
#define ASR_CMD_TEXT_MAX 40
#define ASR_HORN_CMD_QUEUE_MAX 12
#define ASR_MATCH_IN_PROXY 1
#define ASR_CONNECT_RETRY_MS 500U
#define ASR_WIFI_BOOT_RETRY_MS 500U
#define ASR_WIFI_REINIT_RETRY_MS 500U
#define ASR_FINAL_RESULT_POLL_MS 50U
#define ASR_FINAL_RESULT_WAIT_MS 3000U

int16       adc_get_one_data;                                           // 获取麦克风一次的ADC值
int         audio_get_count = -1;                                       // 音频数据计数

fifo_struct adc_data_fifo;                                              // 麦克风数据fifo
uint32      fifo_data_count;                                            // fifo一次接收数据变量
int16       adc_get_data[ASR_SEND_DATA_MAX_LENTH * ASR_FIFO_DEPTH_FACTOR]; // fifo接收数据缓冲区
int16       fifo_get_data[ASR_SEND_DATA_MAX_LENTH];                     // fifo获取缓冲区

char        json_data[ASR_SEND_DATA_MAX_LENTH * 3];                     // json发送缓冲区
char        temp_data[ASR_SEND_DATA_MAX_LENTH * 3 + 2000];              // 临时数据缓冲区
uint8       websocket_receive_buffer[4096];                             // 接收websocket数据

uint8       audio_start_flag = 0;                                       // 语音识别开始标志位
uint8       audio_need_net_flag = 0;                                    // 语音识别需要联网标志位
uint8       audio_server_link_flag = 0;                                 // 语音识别连接服务器标志位
uint8       audio_send_data_flag = 0;                                   // 语音识别发送数据标志位
uint32      asr_max_time = 0;
static char asr_result_text[256];                                       // 最近一次识别结果缓存
static char asr_cmd_history[ASR_CMD_HISTORY_MAX][ASR_CMD_TEXT_MAX];     // 命令历史（按出现顺序）
static uint8 asr_cmd_history_count = 0;
static char asr_cmd_history_displayed[ASR_CMD_HISTORY_MAX][ASR_CMD_TEXT_MAX]; // 已显示历史
static uint8 asr_cmd_history_displayed_count = 0;
static bool asr_header_drawn = false;
static uint32 asr_next_connect_retry_ms = 0;
static uint32 asr_next_finalize_action_ms = 0;
static uint32 asr_finalize_deadline_ms = 0;
static uint32 asr_last_heartbeat_ms = 0;
static bool asr_connect_status_shown = false;
static bool asr_wifi_ready = false;
static bool asr_wifi_connecting_shown = false;
static uint32 asr_next_boot_wifi_retry_ms = 0;
static uint32 asr_boot_wifi_retry_count = 0;
static char asr_proxy_rx_cache[512] = {0};
static size_t asr_proxy_rx_cache_len = 0;
static char asr_horn_cmd_queue[ASR_HORN_CMD_QUEUE_MAX][ASR_CMD_TEXT_MAX];
static uint8 asr_horn_cmd_q_head = 0;
static uint8 asr_horn_cmd_q_tail = 0;
static uint8 asr_horn_cmd_q_count = 0;

typedef enum
{
    ASR_FINALIZE_IDLE = 0,
    ASR_FINALIZE_DRAIN_FIFO,
    ASR_FINALIZE_WAIT_RESULT,
    ASR_FINALIZE_REINIT_WIFI,
} asr_finalize_phase_t;

static asr_finalize_phase_t asr_finalize_phase = ASR_FINALIZE_IDLE;

static void extract_content_fields(const char* input);
static void asr_exec_horn_cmd(const char *cmd_text);
static void asr_enqueue_horn_cmd(const char *cmd_text);
static uint8 asr_dequeue_horn_cmd(char *out_cmd, uint8 out_size);
static void asr_execute_one_queued_horn_cmd(void);

static bool asr_time_reached(uint32 now_ms, uint32 target_ms)
{
    return ((int32)(now_ms - target_ms)) >= 0;
}

void audio_prepare_service(void)
{
    // 进入ASR服务前复位状态，支持多次进入
    audio_start_flag = 0;
    audio_need_net_flag = 0;
    audio_send_data_flag = 0;
    audio_server_link_flag = 0;
    audio_get_count = -1;
    asr_max_time = 0;
    asr_finalize_phase = ASR_FINALIZE_IDLE;
    asr_next_connect_retry_ms = 0;
    asr_next_finalize_action_ms = 0;
    asr_finalize_deadline_ms = 0;
    asr_connect_status_shown = false;

    if(audio_server_link_flag)
    {
        websocket_client_close();
    }

    // 每次进入ASR都重置WiFi连接节拍状态，避免卡在旧状态机
    if(!asr_wifi_ready)
    {
        asr_wifi_connecting_shown = false;
        asr_next_boot_wifi_retry_ms = 0;
        asr_boot_wifi_retry_count = 0;
    }
    // 如果WiFi已经就绪，恢复采样/按键回调中断，确保P20_9可再次触发
    else
    {
        pit_enable(ASR_PIT);
    }
}

static uint8 asr_utf8_char_len(uint8 b)
{
    if((b & 0x80) == 0x00) return 1;
    if((b & 0xE0) == 0xC0) return 2;
    if((b & 0xF0) == 0xE0) return 3;
    if((b & 0xF8) == 0xF0) return 4;
    return 1;
}

#if !ASR_MATCH_IN_PROXY
typedef struct
{
    const char *phrase;
    const char *lcd_text;
} asr_phrase_map_item_t;

static const asr_phrase_map_item_t g_asr_phrase_map[] =
{
    {"打开左转向灯", "CMD: TURN ON LEFT SIGNAL"},
    {"打开右转向灯", "CMD: TURN ON RIGHT SIGNAL"},
    {"打开远光灯", "CMD: TURN ON HIGH BEAM"},
    {"打开近光灯", "CMD: TURN ON LOW BEAM"},
    {"打开雾灯", "CMD: TURN ON FOG LIGHT"},
    {"打开双闪灯", "CMD: TURN ON HAZARD"},
    {"打开车内照明灯", "CMD: TURN ON CABIN LIGHT"},
    {"打开雨刷器", "CMD: TURN ON WIPER"},

    {"鸣笛一秒钟", "CMD: HORN 1 SECOND"},
    {"鸣笛1秒钟", "CMD: HORN 1 SECOND"},
    {"鸣笛一秒", "CMD: HORN 1 SECOND"},
    {"鸣笛1秒", "CMD: HORN 1 SECOND"},

    {"鸣笛两秒钟", "CMD: HORN 2 SECONDS"},
    {"鸣笛2秒钟", "CMD: HORN 2 SECONDS"},
    {"鸣笛两秒", "CMD: HORN 2 SECONDS"},
    {"鸣笛2秒", "CMD: HORN 2 SECONDS"},

    {"鸣笛三秒钟", "CMD: HORN 3 SECONDS"},
    {"鸣笛3秒钟", "CMD: HORN 3 SECONDS"},
    {"鸣笛三秒", "CMD: HORN 3 SECONDS"},
    {"鸣笛3秒", "CMD: HORN 3 SECONDS"},

    {"鸣笛一声", "CMD: HORN 1 TIME"},
    {"鸣笛1声", "CMD: HORN 1 TIME"},
    {"鸣笛两声", "CMD: HORN 2 TIMES"},
    {"鸣笛2声", "CMD: HORN 2 TIMES"},
    {"鸣笛三声", "CMD: HORN 3 TIMES"},
    {"鸣笛3声", "CMD: HORN 3 TIMES"},
    {"鸣笛四声", "CMD: HORN 4 TIMES"},
    {"鸣笛4声", "CMD: HORN 4 TIMES"},
    {"长短鸣笛", "CMD: HORN LONG SHORT"},
    {"急促鸣笛", "CMD: HORN RAPID"},
    {"警报鸣笛", "CMD: HORN ALARM"},

    {"通过门洞1左侧", "CMD: PASS GATE1 LEFT"},
    {"通过门洞1", "CMD: PASS GATE1"},
    {"通过门洞2", "CMD: PASS GATE2"},
    {"通过门洞3右侧", "CMD: PASS GATE3 RIGHT"},
    {"通过门洞3", "CMD: PASS GATE3"},

    {"门洞1右侧返回", "CMD: RETURN GATE1 RIGHT"},
    {"门洞1返回", "CMD: RETURN GATE1"},
    {"门洞2返回", "CMD: RETURN GATE2"},
    {"门洞3左侧返回", "CMD: RETURN GATE3 LEFT"},
    {"门洞3返回", "CMD: RETURN GATE3"},

    {"前行10米", "CMD: FORWARD 10M"},
    {"后退10米", "CMD: BACKWARD 10M"},
    {"蛇形前进10米", "CMD: SNAKE FORWARD 10M"},
    {"蛇形后退10米", "CMD: SNAKE BACKWARD 10M"},
    {"逆时针转一圈", "CMD: CCW ONE CIRCLE"},
    {"顺时针转一圈", "CMD: CW ONE CIRCLE"},
    {"左转", "CMD: TURN LEFT"},
    {"右转", "CMD: TURN RIGHT"},
};

static size_t asr_normalize_text_for_match(const char *src, char *dst, size_t dst_cap)
{
    if(dst_cap == 0)
    {
        return 0;
    }

    // 归一化：去掉空格与常见标点，提升短语命中率
    {
        const uint8 *s = (const uint8 *)src;
        size_t si = 0, di = 0;
        while(s[si] != '\0' && di + 1 < dst_cap)
        {
            uint8 b = s[si];
            uint8 l = asr_utf8_char_len(b);

            if(b < 0x80)
            {
                if(b == ' ' || b == ',' || b == '.' || b == ':' || b == ';' || b == '!' || b == '?')
                {
                    si++;
                    continue;
                }
                dst[di++] = (char)b;
                si++;
                continue;
            }

            // 中文标点直接忽略：，。、！？：；（）
            if(l == 3)
            {
                if((s[si] == 0xEF && s[si + 1] == 0xBC && (s[si + 2] == 0x8C || s[si + 2] == 0x81 || s[si + 2] == 0x9F || s[si + 2] == 0x9A || s[si + 2] == 0x9B || s[si + 2] == 0x88 || s[si + 2] == 0x89)) ||
                   (s[si] == 0xE3 && s[si + 1] == 0x80 && (s[si + 2] == 0x82 || s[si + 2] == 0x81)))
                {
                    si += 3;
                    continue;
                }
            }

            if(di + l >= dst_cap)
            {
                break;
            }
            for(uint8 k = 0; k < l; k++)
            {
                dst[di++] = (char)s[si + k];
            }
            si += l;
        }
        dst[di] = '\0';
        return di;
    }
}

static void asr_append_cmd_history(const char *cmd)
{
    if(asr_cmd_history_count > 0)
    {
        if(strcmp(asr_cmd_history[asr_cmd_history_count - 1], cmd) == 0)
        {
            return;
        }
    }

    if(asr_cmd_history_count < ASR_CMD_HISTORY_MAX)
    {
        snprintf(asr_cmd_history[asr_cmd_history_count], ASR_CMD_TEXT_MAX, "%s", cmd);
        asr_cmd_history_count++;
    }
    else
    {
        for(uint8 i = 1; i < ASR_CMD_HISTORY_MAX; i++)
        {
            snprintf(asr_cmd_history[i - 1], ASR_CMD_TEXT_MAX, "%s", asr_cmd_history[i]);
        }
        snprintf(asr_cmd_history[ASR_CMD_HISTORY_MAX - 1], ASR_CMD_TEXT_MAX, "%s", cmd);
    }
}

static bool asr_history_contains(const char *cmd)
{
    for(uint8 i = 0; i < asr_cmd_history_count; i++)
    {
        if(strcmp(asr_cmd_history[i], cmd) == 0)
        {
            return true;
        }
    }
    return false;
}

static void asr_append_fuzzy_horn_cmds(const char *normalized_src)
{
    bool has_horn = (strstr(normalized_src, "鸣笛") != NULL);
    if(!has_horn)
    {
        return;
    }

    // 时长类：兜底识别（兼容阿拉伯数字与中文数字）
    if(strstr(normalized_src, "秒") != NULL)
    {
        if((strstr(normalized_src, "一") != NULL || strstr(normalized_src, "1") != NULL) && !asr_history_contains("CMD: HORN 1 SECOND"))
        {
            asr_append_cmd_history("CMD: HORN 1 SECOND");
        }
        if((strstr(normalized_src, "两") != NULL || strstr(normalized_src, "2") != NULL || strstr(normalized_src, "二") != NULL) && !asr_history_contains("CMD: HORN 2 SECONDS"))
        {
            asr_append_cmd_history("CMD: HORN 2 SECONDS");
        }
        if((strstr(normalized_src, "三") != NULL || strstr(normalized_src, "3") != NULL) && !asr_history_contains("CMD: HORN 3 SECONDS"))
        {
            asr_append_cmd_history("CMD: HORN 3 SECONDS");
        }
    }

    // 次数类：兜底识别
    if(strstr(normalized_src, "声") != NULL)
    {
        if((strstr(normalized_src, "一") != NULL || strstr(normalized_src, "1") != NULL) && !asr_history_contains("CMD: HORN 1 TIME"))
        {
            asr_append_cmd_history("CMD: HORN 1 TIME");
        }
        if((strstr(normalized_src, "两") != NULL || strstr(normalized_src, "2") != NULL || strstr(normalized_src, "二") != NULL) && !asr_history_contains("CMD: HORN 2 TIMES"))
        {
            asr_append_cmd_history("CMD: HORN 2 TIMES");
        }
        if((strstr(normalized_src, "三") != NULL || strstr(normalized_src, "3") != NULL) && !asr_history_contains("CMD: HORN 3 TIMES"))
        {
            asr_append_cmd_history("CMD: HORN 3 TIMES");
        }
        if((strstr(normalized_src, "四") != NULL || strstr(normalized_src, "4") != NULL) && !asr_history_contains("CMD: HORN 4 TIMES"))
        {
            asr_append_cmd_history("CMD: HORN 4 TIMES");
        }
    }
}

static void asr_rebuild_cmd_history(const char *src)
{
    char normalized_src[512] = {0};
    size_t src_len = asr_normalize_text_for_match(src, normalized_src, sizeof(normalized_src));
    size_t pos = 0;

    asr_cmd_history_count = 0;
    memset(asr_cmd_history, 0, sizeof(asr_cmd_history));

    while(pos < src_len)
    {
        int best_idx = -1;
        size_t best_len = 0;

        for(size_t i = 0; i < (sizeof(g_asr_phrase_map) / sizeof(g_asr_phrase_map[0])); i++)
        {
            char normalized_phrase[128] = {0};
            size_t phrase_len = asr_normalize_text_for_match(g_asr_phrase_map[i].phrase, normalized_phrase, sizeof(normalized_phrase));
            if(phrase_len == 0 || pos + phrase_len > src_len)
            {
                continue;
            }
            if(memcmp(&normalized_src[pos], normalized_phrase, phrase_len) == 0)
            {
                if(phrase_len > best_len)
                {
                    best_len = phrase_len;
                    best_idx = (int)i;
                }
            }
        }

        if(best_idx >= 0)
        {
            asr_append_cmd_history(g_asr_phrase_map[best_idx].lcd_text);
            pos += best_len;
        }
        else
        {
            pos++;
        }
    }

    // 精确匹配未覆盖到的情况，使用关键词兜底一次
    asr_append_fuzzy_horn_cmds(normalized_src);
}
#endif

#if ASR_MATCH_IN_PROXY
static void asr_append_cmd_history(const char *cmd)
{
    if(asr_cmd_history_count > 0)
    {
        if(strcmp(asr_cmd_history[asr_cmd_history_count - 1], cmd) == 0)
        {
            return;
        }
    }

    if(asr_cmd_history_count < ASR_CMD_HISTORY_MAX)
    {
        snprintf(asr_cmd_history[asr_cmd_history_count], ASR_CMD_TEXT_MAX, "%s", cmd);
        asr_cmd_history_count++;
    }
    else
    {
        for(uint8 i = 1; i < ASR_CMD_HISTORY_MAX; i++)
        {
            snprintf(asr_cmd_history[i - 1], ASR_CMD_TEXT_MAX, "%s", asr_cmd_history[i]);
        }
        snprintf(asr_cmd_history[ASR_CMD_HISTORY_MAX - 1], ASR_CMD_TEXT_MAX, "%s", cmd);
    }
}
#endif

static void asr_append_proxy_cmd_from_line(const char *line)
{
    const char *p = line;
    if(NULL == p)
    {
        return;
    }

    // 同一包里可能包含多条 CMD，逐条提取
    while((p = strstr(p, "CMD:")) != NULL)
    {
        char cmd[ASR_CMD_TEXT_MAX] = {0};
        size_t i = 0;

        p += 4;
        while(*p == ' ' || *p == '\t')
        {
            p++;
        }

        while(*p != '\0' && *p != '\r' && *p != '\n' && i + 1 < sizeof(cmd))
        {
            cmd[i++] = *p++;
        }
        cmd[i] = '\0';

        if(i > 0)
        {
            asr_append_cmd_history(cmd);
            asr_enqueue_horn_cmd(cmd);
        }

        while(*p == '\r' || *p == '\n')
        {
            p++;
        }
    }
}

static void asr_enqueue_horn_cmd(const char *cmd_text)
{
    if(NULL == cmd_text)
    {
        return;
    }

    if(asr_horn_cmd_q_count >= ASR_HORN_CMD_QUEUE_MAX)
    {
        // 队列满时丢弃最老命令，保证新命令可进队
        asr_horn_cmd_q_head = (uint8)((asr_horn_cmd_q_head + 1U) % ASR_HORN_CMD_QUEUE_MAX);
        asr_horn_cmd_q_count--;
    }

    snprintf(asr_horn_cmd_queue[asr_horn_cmd_q_tail], ASR_CMD_TEXT_MAX, "%s", cmd_text);
    printf("[ASR][HORN] ENQUEUE: %s\r\n", asr_horn_cmd_queue[asr_horn_cmd_q_tail]);
    asr_horn_cmd_q_tail = (uint8)((asr_horn_cmd_q_tail + 1U) % ASR_HORN_CMD_QUEUE_MAX);
    asr_horn_cmd_q_count++;
}

static uint8 asr_dequeue_horn_cmd(char *out_cmd, uint8 out_size)
{
    if(asr_horn_cmd_q_count == 0 || out_cmd == NULL || out_size == 0)
    {
        return 0;
    }

    snprintf(out_cmd, out_size, "%s", asr_horn_cmd_queue[asr_horn_cmd_q_head]);
    printf("[ASR][HORN] DEQUEUE: %s\r\n", out_cmd);
    asr_horn_cmd_q_head = (uint8)((asr_horn_cmd_q_head + 1U) % ASR_HORN_CMD_QUEUE_MAX);
    asr_horn_cmd_q_count--;
    return 1;
}

static void asr_execute_one_queued_horn_cmd(void)
{
    char cmd[ASR_CMD_TEXT_MAX] = {0};
    if(asr_dequeue_horn_cmd(cmd, (uint8)sizeof(cmd)))
    {
        asr_exec_horn_cmd(cmd);
    }
}

static void asr_exec_horn_cmd(const char *cmd_text)
{
    const char *cmd = cmd_text;

    if(NULL == cmd)
    {
        return;
    }

    // 兼容可能带有 "CMD:" 前缀的文本
    if(strncmp(cmd, "CMD:", 4) == 0)
    {
        cmd += 4;
        while(*cmd == ' ' || *cmd == '\t')
        {
            cmd++;
        }
    }

    if(strcmp(cmd, "HORN 1 SECOND") == 0)
    {
        LoudSpeak_Horn_1s();
    }
    else if(strcmp(cmd, "HORN 2 SECONDS") == 0)
    {
        LoudSpeak_Horn_2s();
    }
    else if(strcmp(cmd, "HORN 3 SECONDS") == 0)
    {
        LoudSpeak_Horn_3s();
    }
    else if(strcmp(cmd, "HORN 2 TIMES") == 0)
    {
        LoudSpeak_Horn_2Times();
    }
    else if(strcmp(cmd, "HORN 3 TIMES") == 0)
    {
        LoudSpeak_Horn_3Times();
    }
    else if(strcmp(cmd, "HORN 4 TIMES") == 0)
    {
        LoudSpeak_Horn_4Times();
    }
    else if(strcmp(cmd, "HORN LONG SHORT") == 0)
    {
        LoudSpeak_Horn_LongShort();
    }
    else if(strcmp(cmd, "HORN RAPID") == 0)
    {
        LoudSpeak_Horn_Rapid();
    }
    else if(strcmp(cmd, "HORN ALARM") == 0)
    {
        LoudSpeak_Horn_Alarm();
    }
}

static void asr_feed_proxy_cmd_stream(const char *chunk)
{
    size_t chunk_len = 0;

    if(NULL == chunk)
    {
        return;
    }

    chunk_len = strlen(chunk);
    if(chunk_len == 0)
    {
        return;
    }

    // 追加到缓存，处理 TCP 分包导致的 CMD 关键字/行尾断裂
    if(asr_proxy_rx_cache_len + chunk_len >= sizeof(asr_proxy_rx_cache))
    {
        asr_proxy_rx_cache_len = 0;
        asr_proxy_rx_cache[0] = '\0';
    }

    memcpy(asr_proxy_rx_cache + asr_proxy_rx_cache_len, chunk, chunk_len);
    asr_proxy_rx_cache_len += chunk_len;
    asr_proxy_rx_cache[asr_proxy_rx_cache_len] = '\0';

    // 同时按“换行”和“下一条CMD起始”切分，避免最后一条无换行时丢失
    {
        size_t cursor = 0;

        while(cursor < asr_proxy_rx_cache_len)
        {
            char *cmd_start_ptr = strstr(asr_proxy_rx_cache + cursor, "CMD:");
            if(NULL == cmd_start_ptr)
            {
                break;
            }

            size_t cmd_start = (size_t)(cmd_start_ptr - asr_proxy_rx_cache);
            char *line_end_ptr = strpbrk(asr_proxy_rx_cache + cmd_start, "\r\n");
            char *next_cmd_ptr = strstr(asr_proxy_rx_cache + cmd_start + 4, "CMD:");
            size_t cmd_end = 0;

            if(line_end_ptr && (!next_cmd_ptr || line_end_ptr < next_cmd_ptr))
            {
                cmd_end = (size_t)(line_end_ptr - asr_proxy_rx_cache);
            }
            else if(next_cmd_ptr)
            {
                cmd_end = (size_t)(next_cmd_ptr - asr_proxy_rx_cache);
            }
            else
            {
                // 当前缓存里只有不完整尾包，留待下次拼包
                cursor = cmd_start;
                break;
            }

            if(cmd_end > cmd_start)
            {
                char line[ASR_CMD_TEXT_MAX * 2] = {0};
                size_t line_len = cmd_end - cmd_start;
                if(line_len >= sizeof(line))
                {
                    line_len = sizeof(line) - 1;
                }
                memcpy(line, asr_proxy_rx_cache + cmd_start, line_len);
                line[line_len] = '\0';
                asr_append_proxy_cmd_from_line(line);
            }

            if(line_end_ptr && (!next_cmd_ptr || line_end_ptr < next_cmd_ptr))
            {
                cursor = cmd_end;
                while(cursor < asr_proxy_rx_cache_len &&
                      (asr_proxy_rx_cache[cursor] == '\r' || asr_proxy_rx_cache[cursor] == '\n'))
                {
                    cursor++;
                }
            }
            else
            {
                cursor = cmd_end;
            }
        }

        if(cursor > 0)
        {
            size_t remain = asr_proxy_rx_cache_len - cursor;
            if(remain > 0)
            {
                memmove(asr_proxy_rx_cache, asr_proxy_rx_cache + cursor, remain);
            }
            asr_proxy_rx_cache_len = remain;
            asr_proxy_rx_cache[asr_proxy_rx_cache_len] = '\0';
        }
    }
}

static const uint8* asr_match_chinese_glyph(const uint8 *s)
{
    // 仅匹配当前字库中现有的 4 个汉字：逐飞科技
    if(s[0] == 0xE9 && s[1] == 0x80 && s[2] == 0x90) return (const uint8 *)&chinese_test[0]; // 逐
    if(s[0] == 0xE9 && s[1] == 0xA3 && s[2] == 0x9E) return (const uint8 *)&chinese_test[2]; // 飞
    if(s[0] == 0xE7 && s[1] == 0xA7 && s[2] == 0x91) return (const uint8 *)&chinese_test[4]; // 科
    if(s[0] == 0xE6 && s[1] == 0x8A && s[2] == 0x80) return (const uint8 *)&chinese_test[6]; // 技
    return NULL;
}

static char asr_map_utf8_to_ascii(const uint8 *s, uint8 char_len)
{
    // 常见中文标点/全角符号映射到 ASCII，提升无字模时可读性
    if(char_len == 3)
    {
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x8C) return ','; // ，
        if(s[0] == 0xE3 && s[1] == 0x80 && s[2] == 0x82) return '.'; // 。
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x81) return '!'; // ！
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x9F) return '?'; // ？
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x9A) return ':'; // ：
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x9B) return ';'; // ；
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x88) return '('; // （
        if(s[0] == 0xEF && s[1] == 0xBC && s[2] == 0x89) return ')'; // ）
        if(s[0] == 0xE3 && s[1] == 0x80 && s[2] == 0x81) return ','; // 、
    }
    return '*';
}

static void asr_show_text_wrapped(uint16 start_x, uint16 start_y, uint16 line_step, size_t line_cols, uint16 max_y, const char *text)
{
    const uint8 *s = (const uint8 *)text;
    uint16 x = start_x;
    uint16 y = start_y;
    size_t cols_used = 0;
    size_t idx = 0;

    while(s[idx] != '\0' && y < max_y)
    {
        uint8 c = s[idx];
        uint8 char_len = asr_utf8_char_len(c);
        uint8 draw_cols = 1;
        bool draw_chinese = false;
        const uint8 *glyph = NULL;

        if(c == '\n')
        {
            x = start_x;
            y = (uint16)(y + line_step);
            cols_used = 0;
            idx++;
            continue;
        }

        if(char_len == 3)
        {
            glyph = asr_match_chinese_glyph(&s[idx]);
            if(glyph)
            {
                draw_chinese = true;
                draw_cols = 2;
            }
        }

        if(cols_used + draw_cols > line_cols)
        {
            x = start_x;
            y = (uint16)(y + line_step);
            cols_used = 0;
            if(y >= max_y)
            {
                break;
            }
        }

        if(draw_chinese)
        {
            ips200_show_chinese(x, y, 16, glyph, 1, RGB565_WHITE);
            x = (uint16)(x + 16);
            cols_used += 2;
            idx += 3;
            continue;
        }

        // 中文标点/全角字符映射为 ASCII，未支持字模仍安全降级
        if(c >= 0x80)
        {
            char mapped = asr_map_utf8_to_ascii(&s[idx], char_len);
            ips200_show_char(x, y, mapped);
            x = (uint16)(x + 8);
            cols_used += 1;
            idx += char_len;
            continue;
        }

        ips200_show_char(x, y, (c >= 32 && c <= 126) ? (char)c : ' ');
        x = (uint16)(x + 8);
        cols_used += 1;
        idx += 1;
    }
}

static void asr_receive_nonblocking_once(void)
{
    memset(websocket_receive_buffer, 0, sizeof(websocket_receive_buffer));
    if(websocket_client_receive(websocket_receive_buffer))
    {
        extract_content_fields((const char*)websocket_receive_buffer);
    }
}

static void asr_receive_nonblocking_drain(uint8 max_packets)
{
    for(uint8 i = 0; i < max_packets; i++)
    {
        memset(websocket_receive_buffer, 0, sizeof(websocket_receive_buffer));
        if(!websocket_client_receive(websocket_receive_buffer))
        {
            break;
        }
        extract_content_fields((const char*)websocket_receive_buffer);
    }
}

static void asr_render_text(void)
{
    uint16 y = 20;
    bool unchanged = true;
    bool append_only = true;
    uint8 min_count = 0;

    if(!ASR_SCREEN_ENABLE)
    {
        printf("[ASR] 屏幕未启用，跳过文本渲染\r\n");
        return;
    }

#if !ASR_MATCH_IN_PROXY
    asr_rebuild_cmd_history(asr_result_text);
#endif

    // 对比本次与上次显示内容，决定是否增量刷新
    if(asr_cmd_history_count != asr_cmd_history_displayed_count)
    {
        unchanged = false;
    }

    min_count = (asr_cmd_history_count < asr_cmd_history_displayed_count) ? asr_cmd_history_count : asr_cmd_history_displayed_count;
    for(uint8 i = 0; i < min_count; i++)
    {
        if(strcmp(asr_cmd_history[i], asr_cmd_history_displayed[i]) != 0)
        {
            unchanged = false;
            if(i != asr_cmd_history_displayed_count)
            {
                append_only = false;
            }
            break;
        }
    }

    if(unchanged)
    {
        return;
    }

    // 仅当不是追加场景时才整屏重绘
    if(!asr_header_drawn || !(append_only && asr_cmd_history_count >= asr_cmd_history_displayed_count))
    {
        ips200_clear();
        ips200_show_string(0, 0, "ASR Result:");
        asr_header_drawn = true;
        for(uint8 i = 0; i < asr_cmd_history_count && y < 220; i++)
        {
            asr_show_text_wrapped(0, y, 20, ASR_LCD_RESULT_COLS, (uint16)(y + 20), asr_cmd_history[i]);
            y = (uint16)(y + 20);
        }
    }
    else
    {
        // 追加场景仅绘制新增行，显著降低刷新负担
        y = (uint16)(20 + asr_cmd_history_displayed_count * 20);
        for(uint8 i = asr_cmd_history_displayed_count; i < asr_cmd_history_count && y < 220; i++)
        {
            asr_show_text_wrapped(0, y, 20, ASR_LCD_RESULT_COLS, (uint16)(y + 20), asr_cmd_history[i]);
            y = (uint16)(y + 20);
        }
    }

    if(asr_cmd_history_count == 0)
    {
        if(!asr_header_drawn)
        {
            ips200_clear();
            ips200_show_string(0, 0, "ASR Result:");
            asr_header_drawn = true;
        }
        ips200_show_string(0, 20, "Waiting for command...");
    }

    // 更新已显示缓存
    asr_cmd_history_displayed_count = asr_cmd_history_count;
    memset(asr_cmd_history_displayed, 0, sizeof(asr_cmd_history_displayed));
    for(uint8 i = 0; i < asr_cmd_history_count; i++)
    {
        snprintf(asr_cmd_history_displayed[i], ASR_CMD_TEXT_MAX, "%s", asr_cmd_history[i]);
    }
}

static void asr_result_reset(void)
{
    memset(asr_result_text, 0, sizeof(asr_result_text));
    memset(asr_cmd_history, 0, sizeof(asr_cmd_history));
    memset(asr_cmd_history_displayed, 0, sizeof(asr_cmd_history_displayed));
    asr_cmd_history_count = 0;
    asr_cmd_history_displayed_count = 0;
    asr_header_drawn = false;
    asr_proxy_rx_cache_len = 0;
    asr_proxy_rx_cache[0] = '\0';
    if(!ASR_SCREEN_ENABLE)
    {
        printf("[ASR] 屏幕未启用，跳过清屏提示\r\n");
        return;
    }
    printf("[ASR] asr_result_reset -> clear\r\n");
    ips200_clear();
    printf("[ASR] asr_result_reset -> show title\r\n");
    ips200_show_string(0, 0, "ASR Result:");
    printf("[ASR] asr_result_reset -> show waiting\r\n");
    ips200_show_string(0, 20, "Waiting for speech...");
}

#if !ASR_MATCH_IN_PROXY
static void asr_append_result(const char *fragment)
{
    if(!fragment) return;
    size_t cur = strlen(asr_result_text);
    size_t cap = sizeof(asr_result_text);
    if(cur + 1 >= cap) return;

    size_t frag_len = strlen(fragment);
    if(frag_len > (cap - cur - 1)) frag_len = cap - cur - 1;
    memcpy(&asr_result_text[cur], fragment, frag_len);
    asr_result_text[cur + frag_len] = '\0';

    asr_render_text();
}
#endif

static void asr_display_status(const char *msg)
{
    if(!ASR_SCREEN_ENABLE)
    {
        printf("[ASR] 屏幕未启用，状态: %s\r\n", msg);
        return;
    }
    printf("[ASR] asr_display_status -> clear\r\n");
    ips200_clear();
    printf("[ASR] asr_display_status -> title\r\n");
    ips200_show_string(0, 0, "ASR Status:");
    printf("[ASR] asr_display_status -> msg\r\n");
    asr_show_text_wrapped(0, 20, 20, ASR_LCD_MAX_COLS, 80, msg);
}

// 解析语音识别字段
static void extract_content_fields(const char* input)
{
#if ASR_MATCH_IN_PROXY
    // 代理下发格式：CMD:XXXX\n
    if(strstr(input, "CMD:") != NULL)
    {
        asr_feed_proxy_cmd_stream(input);
        asr_render_text();
        return;
    }
    return;
#else
    const char* ptr = input;
    char w_value[256];
    while(*ptr)
    {
        // 查找 "w":" 模式
        const char* w_start = strstr(ptr, "\"w\":\"");
        if(!w_start)
        {
            break;
        }
        w_start += 5; // 跳过 "\"w\":\""
        const char* w_end = strchr(w_start, '"');
        if(!w_end)
        {
            break;
        }
        // 提取 w 字段的值
        int length = w_end - w_start;
        strncpy(w_value, w_start, length);
        w_value[length] = '\0';
        // 打印 w 字段的值并在屏幕上追加展示
        printf("%s", w_value);
        asr_append_result(w_value);
        ptr = w_end + 1;
    }
#endif
}

// 语音识别定时器回调函数
void audio_callback()
{
    uint8 btn_pressed = 0;
#if ASR_BUTTON_ACTIVE_HIGH
    btn_pressed = (gpio_get_level(ASR_BUTTON) == 1);
#else
    btn_pressed = (gpio_get_level(ASR_BUTTON) == 0);
#endif
    static uint8 last_pressed = 0;
    static uint32 press_log_cnt = 0;

    // 边沿触发：按下开始，再按一次停止
    if(btn_pressed && !last_pressed)
    {
        printf("[ASR] 检测到按键下降沿，准备切换识别状态 (#%lu)\r\n", (unsigned long)(++press_log_cnt));
        if(!audio_start_flag && audio_get_count == -1)
        {
            audio_get_count = 0;
            audio_server_link_flag = 0;
            audio_start_flag = 1;
            asr_max_time = 0;
            printf("[ASR] 按键按下，开始连接服务器...\r\n");
        }
        else if(audio_server_link_flag)
        {
            audio_start_flag = 0;
            audio_get_count = 0;
            audio_send_data_flag = 0;
            audio_need_net_flag = 1; // 触发发送结束帧并收尾
            printf("[ASR] 按键按下，停止识别并结束会话\r\n");
        }
    }
    last_pressed = btn_pressed;
    if(audio_start_flag)
    {
        adc_get_one_data = (adc_convert(ASR_AUDIO_ADC) - 1870);
        if(audio_server_link_flag)
        {
            audio_get_count++;
            fifo_write_buffer(&adc_data_fifo, &adc_get_one_data, 1);
        }
        if((audio_get_count + 1) % ASR_CHUNK_SAMPLES == 0)
        {
            if(audio_server_link_flag)
            {
                asr_max_time += ASR_CHUNK_SAMPLES;
                audio_send_data_flag = 1;
            }
        }
    }
    // 超时自动结束
    if(asr_max_time > (60 * 8000) && audio_server_link_flag)
    {
        audio_start_flag = 0;
        audio_get_count = 0;
        audio_send_data_flag = 0;
        audio_need_net_flag = 1;
    }
}

// 语音数据发送
void audio_data_send(uint8 status)
{
    int json_len = 0;
    uint32 used_samples = fifo_used(&adc_data_fifo);

    // status=0/1/2 都尽量携带当前缓冲中的音频，避免首段或尾段语音丢失
    if(used_samples > 0)
    {
        fifo_data_count = used_samples;
        if(fifo_data_count > ASR_SEND_DATA_MAX_LENTH)
        {
            fifo_data_count = ASR_SEND_DATA_MAX_LENTH;
        }
        memset(fifo_get_data, 0, sizeof(fifo_get_data));
        fifo_read_buffer(&adc_data_fifo, fifo_get_data, &fifo_data_count, FIFO_READ_AND_CLEAN);
        memset(temp_data, 0, sizeof(temp_data));
        base64_encode((uint8*)fifo_get_data, temp_data, fifo_data_count * 2);
        memset(json_data, 0, sizeof(json_data));
    }
    else
    {
        fifo_data_count = 0;
        memset(temp_data, 0, sizeof(temp_data));
    }

    json_len = snprintf(json_data, sizeof(json_data),
                        "{\"common\":{\"app_id\":\"%s\"},"
                        "\"business\":{\"domain\":\"iat\",\"language\":\"zh_cn\",\"accent\":\"mandarin\",\"vinfo\":1,\"vad_eos\":10000},"
                        "\"data\":{\"status\":%u,\"format\":\"audio/L16;rate=8000\",\"audio\":\"%s\",\"encoding\":\"raw\"}}",
                        ASR_APIID, (unsigned int)status, temp_data);

    if(json_len < 0)
    {
        printf("[ASR] JSON拼接失败 status=%u\r\n", (unsigned int)status);
        json_data[0] = '\0';
        return;
    }

    if((uint32_t)json_len >= sizeof(json_data))
    {
        printf("[ASR] JSON被截断: need=%d, cap=%lu, status=%u, audio_b64_len=%lu\r\n",
               json_len,
               (unsigned long)sizeof(json_data),
               (unsigned int)status,
               (unsigned long)strlen(temp_data));
    }
}

// 语音识别处理
void audio_loop()
{
    uint32 now_ms = system_getval_ms();

    // 调试心跳：改为按时间戳打印，避免主循环去掉 delay 后刷屏
    if(asr_time_reached(now_ms, asr_last_heartbeat_ms + 1000U))
    {
        asr_last_heartbeat_ms = now_ms;
        printf("[ASR] 心跳: btn=%d start=%d link=%d send=%d need_net=%d fifo_used=%lu\r\n",
               gpio_get_level(ASR_BUTTON), audio_start_flag, audio_server_link_flag,
               audio_send_data_flag, audio_need_net_flag, (unsigned long)fifo_used(&adc_data_fifo));
    }

    // 启动阶段按节拍连接 WiFi，避免在初始化阶段阻塞
    if(!asr_wifi_ready)
    {
        if(!asr_wifi_connecting_shown)
        {
            asr_display_status("Connecting WiFi...");
            asr_wifi_connecting_shown = true;
            asr_next_boot_wifi_retry_ms = now_ms;
        }

        if(asr_time_reached(now_ms, asr_next_boot_wifi_retry_ms))
        {
            if(wifi_spi_init(ASR_WIFI_SSID, ASR_WIFI_PASSWORD))
            {
                asr_boot_wifi_retry_count++;
                printf("[ASR] wifi连接失败，重试 #%lu\r\n", (unsigned long)asr_boot_wifi_retry_count);
                asr_next_boot_wifi_retry_ms = now_ms + ASR_WIFI_BOOT_RETRY_MS;
                return;
            }

            printf("[ASR] wifi连接成功\r\n");
            printf("[ASR] 准备开启采样 PIT\r\n");
            pit_enable(ASR_PIT);
            printf("[ASR] PIT 已开启\r\n");
            asr_wifi_ready = true;
            asr_wifi_connecting_shown = false;
            asr_next_boot_wifi_retry_ms = 0;
            asr_boot_wifi_retry_count = 0;
            asr_result_reset();
            asr_display_status("Press P20.8 to start ASR");
            printf("按下P20.8按键开始连接服务器，连接完成后开始识别，开始识别后再次按下按键停止识别\r\n");
        }
        return;
    }

    // 仅在按键触发时进入识别流程（已移除自动启动）

    if(audio_start_flag && !audio_server_link_flag)
    {
        static uint32 connect_retry = 0;
        if(!asr_connect_status_shown)
        {
            asr_result_reset();
            asr_display_status("Connecting to server...");
            asr_connect_status_shown = true;
            asr_next_connect_retry_ms = now_ms;
        }

        if(asr_time_reached(now_ms, asr_next_connect_retry_ms))
        {
            printf("[ASR] 开始连接服务器，第 %lu 次尝试\r\n", (unsigned long)(++connect_retry));
            if(!websocket_client_connect())
            {
                printf("[ASR] 服务器连接失败，稍后重试...\r\n");
                asr_next_connect_retry_ms = now_ms + ASR_CONNECT_RETRY_MS;
                return;
            }

            audio_server_link_flag = 1;
            asr_connect_status_shown = false;
            asr_next_connect_retry_ms = 0;
            asr_finalize_phase = ASR_FINALIZE_IDLE;
            asr_next_finalize_action_ms = 0;
            asr_finalize_deadline_ms = 0;
            printf("[ASR] 服务器连接成功\r\n");
            printf("服务器连接成功，开始识别语音，最长识别时长为60秒，可手动按键停止\r\n");
            printf("语音识别结果:");
            asr_display_status("Capturing audio...");

            // 语音数据第一帧数据包
            audio_data_send(0);
            websocket_client_send((uint8_t*)json_data, strlen(json_data));

        #if ASR_CONNECT_TEST
            printf("连通性测试已关闭，进入真实语音流程。\r\n");
        #endif
        }
    }
    if(audio_start_flag && audio_server_link_flag && audio_send_data_flag)
    {
        audio_send_data_flag = 0;
        // 语音数据中间帧数据包
        audio_data_send(1);
        printf("[ASR] 发送中间帧，样本=%lu\r\n", (unsigned long)fifo_data_count);
        // 发送中间帧数据包
        websocket_client_send((uint8_t*)json_data, strlen(json_data));
    }

    if(audio_server_link_flag)
    {
        // 非阻塞轮询服务端结果，避免在发送关键路径中阻塞导致前段语音丢失
        asr_receive_nonblocking_drain(4);
    }

    // 连接阶段只接收命令不执行，避免阻塞式鸣笛影响后续命令接收
    if(!audio_server_link_flag)
    {
        asr_execute_one_queued_horn_cmd();
    }

    if(audio_need_net_flag && !audio_send_data_flag)
    {
        if(asr_finalize_phase == ASR_FINALIZE_IDLE)
        {
            asr_finalize_phase = ASR_FINALIZE_DRAIN_FIFO;
            asr_next_finalize_action_ms = now_ms;
        }

        if(asr_finalize_phase == ASR_FINALIZE_DRAIN_FIFO && asr_time_reached(now_ms, asr_next_finalize_action_ms))
        {
            if(fifo_used(&adc_data_fifo) > ASR_SEND_DATA_MAX_LENTH)
            {
                uint32 pacing_ms = 1;
                audio_data_send(1);
                printf("[ASR] 结束前补发中间帧，样本=%lu\r\n", (unsigned long)fifo_data_count);
                websocket_client_send((uint8_t*)json_data, strlen(json_data));
                pacing_ms = (uint32)((fifo_data_count * 1000UL) / 8000UL);
                if(pacing_ms == 0)
                {
                    pacing_ms = 1;
                }
                asr_next_finalize_action_ms = now_ms + pacing_ms;
                return;
            }

            audio_data_send(2);
            printf("[ASR] 发送结束帧，样本=%lu\r\n", (unsigned long)fifo_data_count);
            websocket_client_send((uint8_t*)json_data, strlen(json_data));
            printf("[ASR] 等待最终识别结果...\r\n");
            asr_finalize_phase = ASR_FINALIZE_WAIT_RESULT;
            asr_next_finalize_action_ms = now_ms;
            asr_finalize_deadline_ms = now_ms + ASR_FINAL_RESULT_WAIT_MS;
        }

        if(asr_finalize_phase == ASR_FINALIZE_WAIT_RESULT)
        {
            if(asr_time_reached(now_ms, asr_next_finalize_action_ms))
            {
                asr_receive_nonblocking_once();
                asr_next_finalize_action_ms = now_ms + ASR_FINAL_RESULT_POLL_MS;
            }

            if(asr_time_reached(now_ms, asr_finalize_deadline_ms))
            {
                // 关连接前再尽可能拉取并执行剩余命令，避免尾包命令丢失
                asr_receive_nonblocking_drain(8);
                while(asr_horn_cmd_q_count > 0)
                {
                    asr_execute_one_queued_horn_cmd();
                }

                printf("[ASR] 识别结果接收完成\r\n");

                if(asr_max_time > (60 * 8000))
                {
                    printf("\r\n语音识别达到最大时长60秒\r\n");
                    asr_max_time = 0;
                }
                websocket_client_close();
                audio_server_link_flag = 0;
                pit_disable(ASR_PIT);
                asr_finalize_phase = ASR_FINALIZE_REINIT_WIFI;
                asr_next_finalize_action_ms = now_ms;
            }
        }

        if(asr_finalize_phase == ASR_FINALIZE_REINIT_WIFI && asr_time_reached(now_ms, asr_next_finalize_action_ms))
        {
            if(wifi_spi_init(ASR_WIFI_SSID, ASR_WIFI_PASSWORD))
            {
                asr_next_finalize_action_ms = now_ms + ASR_WIFI_REINIT_RETRY_MS;
                return;
            }

            pit_enable(ASR_PIT);
            printf("\r\n语音识别完成，可再次按下按键开始识别\r\n");
            asr_display_status("Recognition finished. Press P20.8 to restart.");
            audio_get_count = -1;
            audio_need_net_flag = 0;
            asr_finalize_phase = ASR_FINALIZE_IDLE;
            asr_next_finalize_action_ms = 0;
            asr_finalize_deadline_ms = 0;
            asr_connect_status_shown = false;
        }
    }
}

void audio_stop_service(void)
{
    // 彻底停止语音服务，避免返回菜单后仍有后台动作
    audio_start_flag = 0;
    audio_need_net_flag = 0;
    audio_send_data_flag = 0;
    audio_server_link_flag = 0;
    audio_get_count = -1;
    asr_max_time = 0;

    asr_finalize_phase = ASR_FINALIZE_IDLE;
    asr_next_connect_retry_ms = 0;
    asr_next_finalize_action_ms = 0;
    asr_finalize_deadline_ms = 0;
    asr_connect_status_shown = false;
    asr_horn_cmd_q_head = 0;
    asr_horn_cmd_q_tail = 0;
    asr_horn_cmd_q_count = 0;

    // 停止采样中断，确保按键与ADC采样回调不再运行
    pit_disable(ASR_PIT);

    // 若连接仍在，主动关闭
    websocket_client_close();

    // 清空ASR屏幕显示，返回菜单时避免残留
    if(ASR_SCREEN_ENABLE)
    {
        ips200_clear();
    }
}

// 语音识别初始化
void audio_init()
{
    printf("等待初始化...\r\n");
    printf("[ASR] 初始化 ADC...\r\n");
    adc_init(RANDOM_NUM_ADC, ADC_12BIT);                                                            // 使用ADC生成随机数
    adc_init(ASR_AUDIO_ADC, ADC_12BIT);                                                             // 初始化麦克风ADC
    // 按键上下拉取决于有效电平
#if ASR_BUTTON_ACTIVE_HIGH
    gpio_init(ASR_BUTTON, GPI, GPIO_LOW, GPI_PULL_DOWN);                                            // 按下为高，默认下拉
#else
    gpio_init(ASR_BUTTON, GPI, GPIO_HIGH, GPI_PULL_UP);                                             // 按下为低，默认上拉
#endif

    printf("[ASR] 初始化 PIT/FIFO...\r\n");
    pit_us_init(ASR_PIT, 125);                                                                      // 开启麦克风接收定时器，频率为8K
    fifo_init(&adc_data_fifo, FIFO_DATA_16BIT, adc_get_data, (ASR_SEND_DATA_MAX_LENTH * ASR_FIFO_DEPTH_FACTOR)); // 初始化音频接收fifo

    pit_disable(ASR_PIT);                                                                           // 连接wifi前先关闭定时器
    printf("[ASR] WiFi 将在主循环中非阻塞连接\r\n");
    asr_display_status("Connecting WiFi...");
    asr_next_connect_retry_ms = 0;
    asr_next_finalize_action_ms = 0;
    asr_finalize_deadline_ms = 0;
    asr_last_heartbeat_ms = system_getval_ms();
    asr_connect_status_shown = false;
    asr_finalize_phase = ASR_FINALIZE_IDLE;
    asr_wifi_ready = false;
    asr_wifi_connecting_shown = true;
    asr_next_boot_wifi_retry_ms = asr_last_heartbeat_ms;
    asr_boot_wifi_retry_count = 0;
    asr_horn_cmd_q_head = 0;
    asr_horn_cmd_q_tail = 0;
    asr_horn_cmd_q_count = 0;
    printf("[ASR] init 完成，进入主循环\r\n");
}
