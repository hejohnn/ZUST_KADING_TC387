/*********************************************************************************************************************
* 后轮编码器应用模块 - 双轮 (左/右), 基于 TC387 硬件 GPT12
*
* 硬件接线:
*     左轮: A = P21.7 (TIM5_CH1) , B = P21.6 (TIM5_CH2)
*     右轮: A = P33.7 (TIM2_CH1) , B = P33.6 (TIM2_CH2)
*
* 参数:
*     编码器每圈 1000 线 (硬件四倍频后 4000 计数)
*     左轮: 每圈行进 5 cm = 50 mm
*     右轮: 每圈行进 20 cm = 200 mm
*
* 实现:
*     使用 TC387 硬件 GPT12 (T2/T5) 做四倍频正交解码
*     每 ENCODER_PIT_PERIOD_MS 在 CCU61_CH0 PIT 中断里累计左右计数, 每 SPEED_SAMPLE_MS 算一次速度
*     menu_*_left/right 用于菜单里通过 MENU_ITEM_INT_SHOW 显示
*
* 注意 (方案二):
*     T5 与 T6 同处 GPT2 块, 共用预分频器/内部时钟。频繁读取 T5 会扰动 T6 旋转编码器。
*     PIT 周期由 10ms 拉长到 50ms, 同时 pit_handler 内的 T5/T2 读取使用 interrupt_global_disable/enable
*     原子保护, 避免 2ms 旋转编码器 ISR 在 T5 读+清序列中插入。
********************************************************************************************************************/

#ifndef __ENCODER_APP_H__
#define __ENCODER_APP_H__

#include "MYHEADFILE.h"

/* ---------------------- 编码器物理参数 ---------------------- */
#define ENCODER_PPR                  4000        // 编码器每圈计数 (硬件四倍频原始值, 1000线*4)
#define DISTANCE_PER_REV_MM_LEFT     200         // 左轮每圈行进距离, 单位 mm (20cm, 与右轮一致)
#define DISTANCE_PER_REV_MM_RIGHT    200         // 右轮每圈行进距离, 单位 mm (20cm)
#define SPEED_SAMPLE_MS         100         // 速度采样周期, 单位 ms (方案二: 拉长到 PIT 周期的整数倍)
#define ENCODER_PIT_PERIOD_MS   50          // PIT 中断周期, 单位 ms (方案二: 10 -> 50, 减少对 T6 的扰动)

/* ---------------------- 菜单显示用的缓存值 (int16, 给 MENU_ITEM_INT_SHOW 使用) ---------------------- */
extern int16 menu_distance_left_cm;       // 左轮累计距离, 单位 cm
extern int16 menu_speed_left_cm_s;        // 左轮当前速度, 单位 cm/s
extern int16 menu_distance_right_cm;      // 右轮累计距离, 单位 cm
extern int16 menu_speed_right_cm_s;       // 右轮当前速度, 单位 cm/s

/* ---------------------- 对外接口 ---------------------- */
void   encoder_app_init(void);
void   encoder_app_pit_handler(void);

int32  encoder_app_get_count_left   (void);
int32  encoder_app_get_count_right  (void);
int32  encoder_app_get_distance_mm_left (void);
int32  encoder_app_get_distance_mm_right(void);
int32  encoder_app_get_speed_mm_s_left  (void);
int32  encoder_app_get_speed_mm_s_right (void);
uint8  encoder_app_speed_updated   (void);
void   encoder_app_reset           (void);

#endif /* __ENCODER_APP_H__ */
