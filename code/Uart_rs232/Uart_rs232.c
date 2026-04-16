/*
 * Uart_rs232.c
 *
 *  Created on: 2026年4月11日
 *      Author: 17706
 */

#include "Uart_rs232.h"
#include "Pedal.h"

#define UART_INDEX      (UART_1)
#define UART_BAUDRATE   (115200)
#define UART_TX_PIN     (UART1_TX_P33_12)
#define UART_RX_PIN     (UART1_RX_P33_13)

/* -------- 踏板UART发送 -------- */
static uint8  pedal_uart_enable  = 0;
static uint8  uart_inited        = 0;
static uint8  pedal_logic_inited = 0;
static float  pedal_percent_show = 0.0f;

/* -------- TC264编码器数据接收（状态机） -------- */
/* 协议格式: "D:<dist>,S:<speed>\r\n"，dist/speed 为带符号整数 */
typedef enum {
    SM_IDLE    = 0,  /* 等待 'D'         */
    SM_D_COLON,      /* 等待 ':'          */
    SM_DIST,         /* 解析 distance     */
    SM_COMMA_S,      /* 等待 'S'          */
    SM_S_COLON,      /* 等待 ':'          */
    SM_SPEED,        /* 解析 speed        */
} Tc264ParseState;

static uint8           tc264_rx_enable   = 0;
static Tc264ParseState tc264_parse_state = SM_IDLE;
static int32           tc264_dist_acc    = 0;
static int32           tc264_dist_sign   = 1;
static int32           tc264_speed_acc   = 0;
static int32           tc264_speed_sign  = 1;
static float           tc264_distance_f  = 0.0f;
static float           tc264_speed_f     = 0.0f;

static void UartRs232_EnsureUartInited(void)
{
    if(!uart_inited)
    {
        uart_init(UART_INDEX, UART_BAUDRATE, UART_TX_PIN, UART_RX_PIN);
        uart_inited = 1;
    }
}

/* 状态机: 逐字节解析 "D:<dist>,S:<speed>\r\n" */
static void Tc264_StateMachine_Feed(uint8 byte)
{
    switch(tc264_parse_state)
    {
        case SM_IDLE:
            if(byte == 'D')
                tc264_parse_state = SM_D_COLON;
            break;

        case SM_D_COLON:
            if(byte == ':') {
                tc264_dist_acc  = 0;
                tc264_dist_sign = 1;
                tc264_parse_state = SM_DIST;
            } else {
                tc264_parse_state = SM_IDLE;
            }
            break;

        case SM_DIST:
            if(byte == '-') {
                tc264_dist_sign = -1;
            } else if(byte >= '0' && byte <= '9') {
                tc264_dist_acc = tc264_dist_acc * 10 + (int32)(byte - '0');
            } else if(byte == ',') {
                tc264_parse_state = SM_COMMA_S;
            } else {
                tc264_parse_state = SM_IDLE;
            }
            break;

        case SM_COMMA_S:
            if(byte == 'S') {
                tc264_parse_state = SM_S_COLON;
            } else {
                tc264_parse_state = SM_IDLE;
            }
            break;

        case SM_S_COLON:
            if(byte == ':') {
                tc264_speed_acc  = 0;
                tc264_speed_sign = 1;
                tc264_parse_state = SM_SPEED;
            } else {
                tc264_parse_state = SM_IDLE;
            }
            break;

        case SM_SPEED:
            if(byte == '-') {
                tc264_speed_sign = -1;
            } else if(byte >= '0' && byte <= '9') {
                tc264_speed_acc = tc264_speed_acc * 10 + (int32)(byte - '0');
            } else if(byte == '\r' || byte == '\n') {
                /* 完整帧，提交数据 */
                tc264_distance_f = (float)(tc264_dist_sign  * tc264_dist_acc);
                tc264_speed_f    = (float)(tc264_speed_sign * tc264_speed_acc);
                tc264_parse_state = SM_IDLE;
            } else {
                tc264_parse_state = SM_IDLE;
            }
            break;

        default:
            tc264_parse_state = SM_IDLE;
            break;
    }
}

/* 供 UART1 RX 中断调用：读一个字节并喂给状态机 */
void UartRs232_Tc264RxHandler(void)
{
    uint8 byte = uart_read_byte(UART_INDEX);
    if(tc264_rx_enable)
        Tc264_StateMachine_Feed(byte);
}

static void UartRs232_SendPedalPercentFrame(uint16 percent)
{
    uint8 tx_buf[4];
    tx_buf[0] = 0xAA;
    tx_buf[1] = (uint8)(percent >> 8);
    tx_buf[2] = (uint8)(percent & 0xFF);
    tx_buf[3] = 0x55;
    uart_write_buffer(UART_INDEX, tx_buf, sizeof(tx_buf));
}

/* 在 cpu0_main 初始化阶段调用，确保 UART 在中断使能前完成初始化 */
void UartRs232_Init(void)
{
    UartRs232_EnsureUartInited();
}

void UartRs232_PedalRuntimeUpdate(void)
{
    UartRs232_EnsureUartInited();

    if(pedal_uart_enable)
    {
        if(!pedal_logic_inited)
        {
            Pedal_Init();
            pedal_logic_inited = 1;
        }

        Pedal_Update();

        {
            uint16 percent;
            percent = Pedal_GetPercent();
            pedal_percent_show = (float)percent;
            UartRs232_SendPedalPercentFrame(percent);
        }
    }
    else
    {
        pedal_logic_inited = 0;
    }
}

uint8 *UartRs232_GetPedalEnablePtr(void)
{
    return &pedal_uart_enable;
}

float *UartRs232_GetPedalPercentShowPtr(void)
{
    return &pedal_percent_show;
}

uint8 *UartRs232_GetTc264RxEnablePtr(void)
{
    return &tc264_rx_enable;
}

float *UartRs232_GetTc264DistanceFloatPtr(void)
{
    return &tc264_distance_f;
}

float *UartRs232_GetTc264SpeedFloatPtr(void)
{
    return &tc264_speed_f;
}

int32 UartRs232_GetTc264DistanceMm(void)
{
    return (int32)tc264_distance_f;
}

int32 UartRs232_GetTc264SpeedMm_s(void)
{
    return (int32)tc264_speed_f;
}
