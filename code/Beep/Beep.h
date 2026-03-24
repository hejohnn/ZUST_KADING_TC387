#ifndef __BEEP_H__
#define __BEEP_H__

#include "MYHEADFILE.h"

// 蜂鸣器控制引脚（当前工程按该引脚输出控制）
#define Beep P33_10

// 蜂鸣器电平极性配置：0 表示低电平响，1 表示高电平响
#define BEEP_ACTIVE_LEVEL   1
#define BEEP_INACTIVE_LEVEL (1 - BEEP_ACTIVE_LEVEL)

// 旋钮触发蜂鸣标志位（1 表示当前处于一次短鸣周期）
extern uint8 g_switch_encoder_ring_flag;
// 按键触发蜂鸣标志位（1 表示当前处于一次短鸣周期）
extern uint8 g_key_ring_flag;
// 蜂鸣器总开关：open_status 允许鸣叫，close_status 禁止鸣叫
extern uint8 beep_enable_flag;

#define    SHORT_RING_TIME          3

// 拉响蜂鸣器（按当前硬件逻辑：低电平为响）
void Beep_Start(void);
// 初始化蜂鸣器引脚及相关定时器资源
void Beep_Init(void);
// 关闭蜂鸣器（按当前硬件逻辑：高电平为停）
void Beep_Stop(void);
// 阻塞式短鸣
void Beep_ShortRing(void);
// 阻塞式中鸣
void Beep_MediumRing(void);
// 阻塞式长鸣
void Beep_LongRing(void);
// 阻塞式自定义时长鸣叫（单位：ms）
void Beep_Ring(uint16_t Time);
// 旋钮事件触发的短鸣起始函数
void Beep_Timer_ShortRing(void);
// 旋钮事件触发的短鸣停止函数
void Beep_Timer_ShortRing_Stop(void);
// 按键事件触发的短鸣起始函数
void Key_Timer_ShortRing(void);
// 按键事件触发的短鸣停止函数
void Key_Timer_ShortRing_Stop(void);

#endif
