/*
 * Turn.c
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 */
#include "Turn.h"

#include <math.h>



//ATOM3_CH1_P14_6

volatile float turn_target_angle_deg = 0.0f;
volatile float turn_current_angle_deg = 0.0f;

static PID_IncTypeDef turn_pid;
static volatile int32 turn_encoder_count = 0;
static volatile int32 turn_encoder_zero = 0;
static uint8 turn_dir_level = Turn_DIR_POSITIVE_LEVEL;
static uint16 turn_startup_hold_cnt = 0;
static uint8 turn_motor_enable = 0;

static uint32 Turn_GetDutyFromOutput(float output)
{
    float abs_output = fabsf(output);
    if(abs_output <= Turn_CTRL_OUTPUT_DEADBAND)
    {
        return 0;
    }

    if(abs_output >= (float)Turn_Duty_LIMIT)
    {
        return Turn_Duty_LIMIT;
    }

    return (uint32)abs_output;
}

static void Turn_ApplyMotorOutput(float output)
{
    uint32 duty = Turn_GetDutyFromOutput(output);
    uint8 target_dir_level = Turn_DIR_POSITIVE_LEVEL;

    if(duty == 0)
    {
        pwm_set_duty(Turn_PWM, 0);
        return;
    }

    if(output < 0.0f)
    {
        target_dir_level = (uint8)!Turn_DIR_POSITIVE_LEVEL;
    }

    if(target_dir_level != turn_dir_level)
    {
        turn_dir_level = target_dir_level;
        gpio_set_level(Turn_DIR_PIN, turn_dir_level);
    }

    pwm_set_duty(Turn_PWM, duty);
}

void Turn_Init(void)
{
    // 上电先把EN/PWM脚当GPIO拉到安全电平，避免复用切换前后误驱动
    gpio_init(Turn_PWM_SAFE_PIN, GPO, Turn_PWM_SAFE_LEVEL, GPO_PUSH_PULL);

    // 再切换到PWM外设并保持0占空比
    pwm_init(Turn_PWM, Turn_MOTOR_FREQ, 0);
    pwm_set_duty(Turn_PWM, 0);

    turn_dir_level = Turn_DIR_POSITIVE_LEVEL;
    gpio_init(Turn_DIR_PIN, GPO, turn_dir_level, GPO_PUSH_PULL);

    encoder_dir_init(Turn_ENCODER_INDEX, Turn_ENCODER_PULSE_PIN, Turn_ENCODER_DIR_INPUT_PIN);
    encoder_clear_count(Turn_ENCODER_INDEX);

    PID_Inc_Init(&turn_pid, Turn_PID_KP, Turn_PID_KI, Turn_PID_KD);

    turn_encoder_count = 0;
    turn_encoder_zero = 0;
    turn_startup_hold_cnt = Turn_STARTUP_HOLD_CYCLES;
    turn_motor_enable = 0;
    turn_target_angle_deg = 0.0f;
    turn_current_angle_deg = 0.0f;
}

void Turn_ControlTask(void)
{
    int16 delta_count = encoder_get_count(Turn_ENCODER_INDEX);
    encoder_clear_count(Turn_ENCODER_INDEX);

    turn_encoder_count += (int32)(Turn_ENCODER_SIGN * delta_count);
    turn_current_angle_deg = (float)(turn_encoder_count - turn_encoder_zero) / Turn_ENCODER_COUNT_PER_DEG;

    // 上电抑制窗口：先采样稳定零点，不输出电机
    if(turn_startup_hold_cnt > 0)
    {
        turn_startup_hold_cnt--;
        turn_encoder_zero = turn_encoder_count;
        turn_current_angle_deg = 0.0f;
        pwm_set_duty(Turn_PWM, 0);
        return;
    }

    if(!turn_motor_enable)
    {
        Turn_Stop();
        return;
    }

    Turn_ApplyMotorOutput(Positional_PID(&turn_pid, turn_target_angle_deg, turn_current_angle_deg, Turn_PID_I_MAX));
}

void Turn_SetTargetAngleDeg(float target_deg)
{
    turn_target_angle_deg = target_deg;
}

void Turn_SetCurrentAngleAsZero(void)
{
    turn_encoder_zero = turn_encoder_count;
    turn_current_angle_deg = 0.0f;
    turn_target_angle_deg = 0.0f;
    PID_clear(&turn_pid);
}

void Turn_Stop(void)
{
    PID_clear(&turn_pid);
    pwm_set_duty(Turn_PWM, 0);
}

void Turn_SetMotorEnable(uint8 enable)
{
    turn_motor_enable = (enable != 0) ? 1 : 0;
    if(!turn_motor_enable)
    {
        Turn_Stop();
    }
}

uint8 Turn_GetMotorEnable(void)
{
    return turn_motor_enable;
}

int32 Turn_GetEncoderCount(void)
{
    return (turn_encoder_count - turn_encoder_zero);
}

float Turn_GetCurrentAngleDeg(void)
{
    return turn_current_angle_deg;

}


