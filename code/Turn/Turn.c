/*
 * Turn.c
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 */
#include "Turn.h"

#include <math.h>

volatile float turn_target_angle_deg = 0.0f;
volatile float turn_current_angle_deg = 0.0f;

typedef struct
{
    volatile int32 encoder_count;
    volatile int32 encoder_zero;
    uint8 dir_level;
    uint16 startup_hold_cnt;
    uint8 motor_enable;
} Turn_RuntimeState;

typedef struct
{
    float target_angle_deg;
    float encoder_value;
    float angle_deg;
    uint8 motor_enable;
} Turn_MenuState;

static PID_IncTypeDef turn_pid;
static Turn_RuntimeState turn_state = {
    .encoder_count = 0,
    .encoder_zero = 0,
    .dir_level = Turn_DIR_POSITIVE_LEVEL,
    .startup_hold_cnt = 0,
    .motor_enable = 0,
};
static Turn_MenuState turn_menu_state = {
    .target_angle_deg = 0.0f,
    .encoder_value = 0.0f,
    .angle_deg = 0.0f,
    .motor_enable = 0,
};

static float Turn_EncToMenuAngleDeg(float enc_val)
{
    return enc_val * (Turn_MENU_ANGLE_FULL_SCALE / Turn_MENU_ENC_FULL_SCALE);
}

static float Turn_MenuAngleDegToEnc(float angle_val)
{
    return angle_val * (Turn_MENU_ENC_FULL_SCALE / Turn_MENU_ANGLE_FULL_SCALE);
}

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

    if(target_dir_level != turn_state.dir_level)
    {
        turn_state.dir_level = target_dir_level;
        gpio_set_level(Turn_DIR_PIN, turn_state.dir_level);
    }

    pwm_set_duty(Turn_PWM, duty);
}

void Turn_MenuTargetAngleSync(void)
{
    Turn_SetTargetAngleDeg(Turn_MenuAngleDegToEnc(turn_menu_state.target_angle_deg));
}

void Turn_MenuRuntimeUpdate(void)
{
    turn_menu_state.encoder_value = (float)Turn_GetEncoderCount();
    turn_menu_state.angle_deg = Turn_EncToMenuAngleDeg(turn_menu_state.encoder_value);

    /* 只在 enable 状态发生变化时才调用 SetMotorEnable，避免每周期触发 PID_clear */
    uint8 new_enable = (turn_menu_state.motor_enable != 0) ? 1U : 0U;
    if(new_enable != turn_state.motor_enable)
    {
        Turn_SetMotorEnable(new_enable);
    }
}

void Turn_Init(void)
{
    // 上电先把EN/PWM脚当GPIO拉到安全电平，避免复用切换前后误驱动
    gpio_init(Turn_PWM_SAFE_PIN, GPO, Turn_PWM_SAFE_LEVEL, GPO_PUSH_PULL);

    // 再切换到PWM外设并保持0占空比
    pwm_init(Turn_PWM, Turn_MOTOR_FREQ, 0);
    pwm_set_duty(Turn_PWM, 0);

    turn_state.dir_level = Turn_DIR_POSITIVE_LEVEL;
    gpio_init(Turn_DIR_PIN, GPO, turn_state.dir_level, GPO_PUSH_PULL);

    encoder_dir_init(Turn_ENCODER_INDEX, Turn_ENCODER_PULSE_PIN, Turn_ENCODER_DIR_INPUT_PIN);
    encoder_clear_count(Turn_ENCODER_INDEX);

    PID_Inc_Init(&turn_pid, Turn_PID_KP, Turn_PID_KI, Turn_PID_KD);

    turn_state.encoder_count = 0;
    turn_state.encoder_zero = 0;
    turn_state.startup_hold_cnt = Turn_STARTUP_HOLD_CYCLES;
    turn_state.motor_enable = 0;
    turn_target_angle_deg = 0.0f;
    turn_current_angle_deg = 0.0f;

    turn_menu_state.target_angle_deg = 0.0f;
    turn_menu_state.encoder_value = 0.0f;
    turn_menu_state.angle_deg = 0.0f;
    turn_menu_state.motor_enable = 0;
}

void Turn_ControlTask(void)
{
    int16 delta_count = encoder_get_count(Turn_ENCODER_INDEX);
    encoder_clear_count(Turn_ENCODER_INDEX);

    turn_state.encoder_count += (int32)(Turn_ENCODER_SIGN * delta_count);
    turn_current_angle_deg = (float)(turn_state.encoder_count - turn_state.encoder_zero) / Turn_ENCODER_COUNT_PER_DEG;

    // 上电抑制窗口：先采样稳定零点，不输出电机
    if(turn_state.startup_hold_cnt > 0)
    {
        turn_state.startup_hold_cnt--;
        turn_state.encoder_zero = turn_state.encoder_count;
        turn_current_angle_deg = 0.0f;
        pwm_set_duty(Turn_PWM, 0);
        return;
    }

    if(!turn_state.motor_enable)
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

void Turn_SetMenuTargetAngleDeg(float target_deg)
{
    turn_menu_state.target_angle_deg = target_deg;
    Turn_SetTargetAngleDeg(Turn_MenuAngleDegToEnc(turn_menu_state.target_angle_deg));
}

float *Turn_GetMenuTargetAngleDegPtr(void)
{
    return &turn_menu_state.target_angle_deg;
}

float *Turn_GetMenuEncoderValuePtr(void)
{
    return &turn_menu_state.encoder_value;
}

float *Turn_GetMenuAngleDegPtr(void)
{
    return &turn_menu_state.angle_deg;
}

uint8 *Turn_GetMenuMotorEnablePtr(void)
{
    return &turn_menu_state.motor_enable;
}

void Turn_SetCurrentAngleAsZero(void)
{
    turn_state.encoder_zero = turn_state.encoder_count;
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
    turn_state.motor_enable = (enable != 0) ? 1 : 0;
    if(!turn_state.motor_enable)
    {
        Turn_Stop();
    }
}

uint8 Turn_GetMotorEnable(void)
{
    return turn_state.motor_enable;
}

int32 Turn_GetEncoderCount(void)
{
    return (turn_state.encoder_count - turn_state.encoder_zero);
}

float Turn_GetCurrentAngleDeg(void)
{
    return turn_current_angle_deg;

}


