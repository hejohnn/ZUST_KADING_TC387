/*
 * Motor.h - 双 EG2104 半桥 双轮直流电机驱动 (踏板线性控制)
 *
 * 硬件:
 *     左轮: 正转 PWM = P33.12 (ATOM2_CH4), 反转 PWM = P33.13 (ATOM2_CH5)
 *     右轮: 正转 PWM = P33.5  (ATOM2_CH1), 反转 PWM = P33.11 (ATOM1_CH2)
 *     驱动方式: 两路 PWM 互锁
 *               正转: FWD 脚输出 PWM, REV 脚输出 0
 *               反转: REV 脚输出 PWM, FWD 脚输出 0
 *
 * 调用顺序:
 *     Pedal_Init();
 *     Motor_Init();
 *     // 周期任务 (建议 10ms 以上)
 *     Pedal_Update();
 *     Motor_UpdateFromPedal();
 */

#ifndef CODE_MOTOR_MOTOR_H_
#define CODE_MOTOR_MOTOR_H_

#include "MYHEADFILE.h"

/* ------------------ PWM 引脚 ------------------ */
#define MOTOR_LEFT_PWM_FWD          ATOM2_CH4_P33_12
#define MOTOR_LEFT_PWM_FWD_SAFE_PIN P33_12
#define MOTOR_LEFT_PWM_REV          ATOM2_CH5_P33_13
#define MOTOR_LEFT_PWM_REV_SAFE_PIN P33_13

#define MOTOR_RIGHT_PWM_FWD          ATOM2_CH1_P33_5
#define MOTOR_RIGHT_PWM_FWD_SAFE_PIN P33_5
#define MOTOR_RIGHT_PWM_REV          ATOM1_CH2_P33_11
#define MOTOR_RIGHT_PWM_REV_SAFE_PIN P33_11

/* ------------------ PWM 参数 ------------------ */
#define MOTOR_PWM_FREQ              20000           /* 20kHz, 超出人耳听阈 */
#define MOTOR_DUTY_LIMIT            5000            /* 上限 50% (PWM_DUTY_MAX=10000), 调试期保守值 */

/* ------------------ 踏板映射 ------------------ */
#define MOTOR_PEDAL_DEADBAND_PCT    2               /* 踏板百分比死区 */

/* ------------------ 菜单显示缓存 ------------------ */
extern int16 menu_motor_enable;        /* 菜单可写, 1=允许电机输出, 0=强制停车 */
extern int16 menu_motor_direction;     /* 菜单可写, 1=前进, -1=后退 */
extern int16 menu_motor_duty_max;      /* 菜单可写, 踏板映射上限 (0..MOTOR_DUTY_LIMIT) */
extern int16 menu_motor_pedal_pct;     /* 当前踏板百分比 */
extern int16 menu_motor_left_duty;     /* 左轮当前占空比 (0..MOTOR_DUTY_LIMIT) */
extern int16 menu_motor_right_duty;    /* 右轮当前占空比 */

/* ------------------ API ------------------ */
void  Motor_Init(void);
void  Motor_Stop(void);
void  Motor_SetEnable(uint8 enable);
uint8 Motor_GetEnable(void);
void  Motor_ToggleEnable(void);          /* 菜单按键切换使能 */
void  Motor_ToggleDirection(void);       /* 菜单按键切换前进/后退 (自动先停车) */

/* 直接设置某轮输出, 入参带符号: 正=前进, 负=后退, 绝对值=占空比(已限幅) */
void  Motor_SetLeftOutput (int32 output);
void  Motor_SetRightOutput(int32 output);

/* 把当前踏板百分比线性映射到两轮占空比, 周期调用 */
void  Motor_UpdateFromPedal(void);

#endif /* CODE_MOTOR_MOTOR_H_ */
