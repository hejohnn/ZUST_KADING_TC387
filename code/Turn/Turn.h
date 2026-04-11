/*
 * Turn.h
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 */

#ifndef CODE_TURN_TURN_H_
#define CODE_TURN_TURN_H_

#include "MYHEADFILE.h"

#define Turn_PWM  ATOM1_CH1_P14_6
#define Turn_PWM_SAFE_PIN                P14_6
#define Turn_PWM_SAFE_LEVEL              0
#define Turn_MOTOR_FREQ  20000
#define Turn_Duty_LIMIT  5000

#define Turn_DIR_PIN                     P14_5
#define Turn_DIR_POSITIVE_LEVEL          1

#define Turn_ENCODER_INDEX               TIM5_ENCODER
#define Turn_ENCODER_PULSE_PIN           TIM5_ENCODER_CH1_P10_3
#define Turn_ENCODER_DIR_INPUT_PIN       TIM5_ENCODER_CH2_P10_1

#define Turn_ENCODER_COUNT_PER_DEG       1.0f
#define Turn_ENCODER_SIGN                1

#define Turn_PID_KP                      8.0f
#define Turn_PID_KI                      0.02f
#define Turn_PID_KD                      0.4f
#define Turn_PID_I_MAX                   500.0f

#define Turn_CTRL_OUTPUT_DEADBAND        30.0f
#define Turn_STARTUP_HOLD_CYCLES         120

#define Turn_MENU_ENC_FULL_SCALE         45000.0f
#define Turn_MENU_ANGLE_FULL_SCALE       60.0f


//extern uint16 STEER_MID;

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



#endif /* CODE_TURN_TURN_H_ */
