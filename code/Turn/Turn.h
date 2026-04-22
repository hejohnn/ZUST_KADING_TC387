/*
 * Turn.h
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 *  Modified:   转向编码器由GPT12增量编码器改为PD1503 SPI绝对编码器
 */

#ifndef CODE_TURN_TURN_H_
#define CODE_TURN_TURN_H_

#include "MYHEADFILE.h"
#include "zf_device_pd1503.h"

// ===================== 转向电机PWM配置 =====================
// 新驱动板: 两个EG2104驱动一个半桥, 双PWM互锁
//   正转: Turn_PWM_FWD 输出 PWM, Turn_PWM_REV 输出 0
//   反转: Turn_PWM_REV 输出 PWM, Turn_PWM_FWD 输出 0
#define Turn_PWM_FWD                     ATOM1_CH1_P14_6
#define Turn_PWM_FWD_SAFE_PIN            P14_6
#define Turn_PWM_REV                     ATOM0_CH0_P14_5
#define Turn_PWM_REV_SAFE_PIN            P14_5
#define Turn_PWM_SAFE_LEVEL              0
#define Turn_MOTOR_FREQ                  20000
#define Turn_Duty_LIMIT                  5000

// ===================== SPI绝对编码器(PD1503)配置 =====================
// SPI引脚配置见 zf_device_pd1503.h (SPI4: SCK=P22.3, MOSI=P22.0, MISO=P22.1, CS=P22.2)
#define Turn_ENCODER_SIGN                1              // 编码器方向: 1=正向, -1=反向
#define Turn_ENCODER_RESOLUTION          65536           // PD1503 16bit分辨率
#define Turn_ENCODER_CENTER_DEG          0.0f            // 编码器机械中位角度(°)
                                                        // 车头打正时读到的编码器绝对角度值
                                                        // 首次使用请上电读数后填入此处
#define Turn_GEAR_RATIO                  1.0f            // 齿轮减速比 (编码器度数/车头度数)
                                                        // 例: 车头转1°编码器转10°则填10.0f

// ===================== Flash断电记忆配置 =====================
#define Turn_FLASH_SECTOR                0              // Flash扇区号(固定0)
#define Turn_FLASH_PAGE                  0              // Flash页号(0-127可选)
#define Turn_FLASH_MAGIC                 0x5AA5C33C     // 魔数，用于校验Flash数据有效性

// ===================== PID 参数 =====================
#define Turn_PID_KP                      8.0f
#define Turn_PID_KI                      0.02f
#define Turn_PID_KD                      0.4f
#define Turn_PID_I_MAX                   500.0f

#define Turn_CTRL_OUTPUT_DEADBAND        30.0f
#define Turn_STARTUP_HOLD_CYCLES         120

// ===================== 菜单显示比例 =====================
// 菜单中 encoder_value 显示编码器累计角度(°), angle_deg 显示车头实际角度(°)
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
void Turn_MenuTargetAngleSync(void);
void Turn_MenuRuntimeUpdate(void);
void Turn_SetCurrentAngleAsZero(void);
void Turn_Stop(void);
void Turn_SetMotorEnable(uint8 enable);
uint8 Turn_GetMotorEnable(void);

int32 Turn_GetEncoderCount(void);
float Turn_GetCurrentAngleDeg(void);
float Turn_GetEncoderTotalDeg(void);        // 编码器多圈总角度(°)
uint16 Turn_GetEncoderRaw(void);
float Turn_GetEncoderAbsAngleDeg(void);     // 编码器单圈绝对角度 (0~360°)
int32 Turn_GetEncoderTurns(void);           // 编码器整圈数
uint8 Turn_GetEncoderValid(void);
void Turn_SavePositionToFlash(void);        // 手动保存位置到Flash
void Turn_ResetTurns_MenuCallback(void);    // 清零圈数(菜单按钮回调)

#endif /* CODE_TURN_TURN_H_ */
