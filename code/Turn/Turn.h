/*
 * Turn.h
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 *  Modified:   转向驱动改为 DRV8701E (PWM + DIR), 编码器仍为 GPT12 增量编码器
 */

#ifndef CODE_TURN_TURN_H_
#define CODE_TURN_TURN_H_

#include "MYHEADFILE.h"

// ===================== 转向电机PWM配置 =====================
// 驱动板: DRV8701E, PWM + DIR 模式
//   速度: Turn_PWM_PIN 输出 PWM (P14.6)
//   方向: Turn_DIR_PIN 输出电平 (P14.5)  高=正转, 低=反转
#define Turn_PWM_PIN                     ATOM0_CH2_P14_3
#define Turn_PWM_SAFE_PIN                P14_3
#define Turn_DIR_PIN                     P14_5
#define Turn_DIR_FORWARD_LEVEL           1
#define Turn_DIR_REVERSE_LEVEL           0
#define Turn_PWM_SAFE_LEVEL              0
#define Turn_MOTOR_FREQ                  20000
#define Turn_Duty_LIMIT                  5000

// ===================== 增量式编码器(GPT12)配置 =====================
// 当前工程定时器占用: TIM2=后右, TIM5=后左, TIM6=旋转编码器
// 空闲定时器: TIM3 / TIM4
// 这里使用 TIM4, 引脚 P02.8 / P00.9 (参考"推荐IO分配"中第3路编码器槽位)
#define Turn_ENCODER_INDEX               TIM4_ENCODER
#define Turn_ENCODER_PULSE_PIN           TIM4_ENCODER_CH1_P02_8
#define Turn_ENCODER_DIR_INPUT_PIN       TIM4_ENCODER_CH2_P00_9
//#define Turn_ENCODER_INDEX               TIM5_ENCODER
//#define Turn_ENCODER_PULSE_PIN           TIM5_ENCODER_CH1_P10_3
//#define Turn_ENCODER_DIR_INPUT_PIN       TIM5_ENCODER_CH2_P10_1

#define Turn_ENCODER_COUNT_PER_DEG       1.0f            // 编码器计数值 / 编码器旋转角度(°)
#define Turn_ENCODER_SIGN                1               // 编码器方向: 1=正向, -1=反向
#define Turn_GEAR_RATIO                  1.0f            // 齿轮减速比 (编码器度数/车头度数)
                                                         // 例: 车头转1°编码器转10°则填10.0f

// ===================== PID 参数 =====================
#define Turn_PID_KP                      8.0f
#define Turn_PID_KI                      0.02f
#define Turn_PID_KD                      0.4f
#define Turn_PID_I_MAX                   500.0f

#define Turn_CTRL_OUTPUT_DEADBAND        30.0f
#define Turn_STARTUP_HOLD_CYCLES         120

// ===================== 菜单显示比例 =====================
// 菜单中 encoder_value 显示(相对零点)累计编码器度数(°), angle_deg 显示车头实际角度(°)
#define Turn_MENU_ENC_FULL_SCALE         360.0f
#define Turn_MENU_ANGLE_FULL_SCALE       360.0f

extern volatile float turn_target_angle_deg;
extern volatile float turn_current_angle_deg;

void Turn_Init(void);
void Turn_ControlTask(void);
void Turn_Page_Init(void);
void Turn_SetTargetAngleDeg(float target_deg);
void Turn_SetMenuTargetAngleDeg(float target_deg);
float *Turn_GetMenuTargetAngleDegPtr(void);
float *Turn_GetMenuEncoderValuePtr(void);
float *Turn_GetMenuAngleDegPtr(void);
uint8 *Turn_GetMenuMotorEnablePtr(void);
int16 *Turn_GetMenuOutputSignPtr(void);
uint8 *Turn_GetMenuEncoderValidPtr(void);
void Turn_MenuTargetAngleSync(void);
void Turn_MenuRuntimeUpdate(void);
void Turn_SetCurrentAngleAsZero(void);
void Turn_ResetTurns_MenuCallback(void);    // 清零当前位置(菜单按钮回调)
void Turn_Stop(void);
void Turn_SetMotorEnable(uint8 enable);
uint8 Turn_GetMotorEnable(void);

int32 Turn_GetEncoderCount(void);
float Turn_GetCurrentAngleDeg(void);
uint8 Turn_GetEncoderValid(void);

#endif /* CODE_TURN_TURN_H_ */
