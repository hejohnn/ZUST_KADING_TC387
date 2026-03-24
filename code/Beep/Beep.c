#include "Beep.h"

// 旋钮事件蜂鸣状态标志
uint8 g_switch_encoder_ring_flag = 0;
// 按键事件蜂鸣状态标志
uint8 g_key_ring_flag = 0;
// 蜂鸣器总开关（默认开启）
uint8 beep_enable_flag = open_status;

void Beep_Init(void)
{
    // 使用统一的蜂鸣器引脚，并在初始化时保持关闭状态
    gpio_init(Beep, GPO, BEEP_INACTIVE_LEVEL, GPO_PUSH_PULL);
    system_delay_init();
    pit_ms_init(CCU61_CH1, SHORT_RING_TIME);            //蜂鸣器中断
    pit_close(CCU61_CH1);
}


void Beep_Start(void)
{
    // 仅在总开关允许时拉响
    if(beep_enable_flag == open_status)
        gpio_set_level(Beep, BEEP_ACTIVE_LEVEL);
}

void Beep_Stop(void)
{
    gpio_set_level(Beep, BEEP_INACTIVE_LEVEL);
}

void Beep_ShortRing(void)
{
    // 阻塞式短鸣
    Beep_Start();
    system_delay_ms(SHORT_RING_TIME);
//    system_delay_us(200);
    Beep_Stop();
}

void Beep_MediumRing(void)
{
    // 阻塞式中鸣（100ms）
    Beep_Start();
    system_delay_ms(100);
    Beep_Stop();
}

void Beep_LongRing(void)
{
    // 阻塞式长鸣（1000ms）
    Beep_Start();
    system_delay_ms(1000);
    Beep_Stop();
}

void Beep_Ring(uint16_t Time)
{
    // 阻塞式自定义鸣叫时长
    Beep_Start();
    system_delay_ms(Time);
    Beep_Stop();
}
/***********************************************
* @brief : 定时器开关蜂鸣器函数
* @param : void
* @return: void
* @date  : 2024年11月6日12:24:04
* @author: SJX
************************************************/
void Beep_Timer_ShortRing(void)
{
    // 置位旋钮蜂鸣标志，具体停止动作由外部周期任务调用 Stop 接口完成
    g_switch_encoder_ring_flag = 1;
    Beep_Start();
//    pit_start(CCU61_CH1);
//    pit_enable(CCU61_CH1);
}

void Beep_Timer_ShortRing_Stop(void)
{
    // 仅在对应标志有效时关鸣，避免误关其它来源的蜂鸣
    if(g_switch_encoder_ring_flag == 1)
    {
        g_switch_encoder_ring_flag = 0;
        Beep_Stop();
    }
}

/***********************************************
* @brief : 定时器开关蜂鸣器函数
* @param : void
* @return: void
* @date  : 2024年11月6日12:24:04
* @author: SJX
************************************************/
void Key_Timer_ShortRing(void)
{
    // 置位按键蜂鸣标志，具体停止动作由外部周期任务调用 Stop 接口完成
    g_key_ring_flag = 1;
    Beep_Start();
//    pit_start(CCU61_CH1);
//    pit_enable(CCU61_CH1);
}

void Key_Timer_ShortRing_Stop(void)
{
    // 仅在对应标志有效时关鸣，避免误关其它来源的蜂鸣
    if(g_key_ring_flag == 1)
    {
        g_key_ring_flag = 0;
        Beep_Stop();
    }
}


