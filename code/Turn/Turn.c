/*
 * Turn.c
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 *  Modified:   转向编码器改回 GPT12 增量编码器 (EG2104 双PWM驱动保留)
 */

#include "Turn.h"

#include <math.h>

volatile float turn_target_angle_deg = 0.0f;
volatile float turn_current_angle_deg = 0.0f;

typedef struct
{
    volatile int32 encoder_count;   // 累计编码器计数值
    volatile int32 encoder_zero;    // 零点计数值 (上电抑制窗口末尾取样)
    int8   output_sign;             // 当前输出方向: +1=正转, -1=反转, 0=停车
    uint16 startup_hold_cnt;
    uint8  motor_enable;
    uint8  encoder_valid;           // 增量编码器硬件常有效, 始终为 1
} Turn_RuntimeState;

typedef struct
{
    float target_angle_deg;
    float encoder_value;
    float angle_deg;
    uint8 motor_enable;
    int16 output_sign;      /* 驱动当前输出符号 +1/-1/0, 菜单 INT_SHOW */
    uint8 encoder_valid;    /* 编码器状态, 菜单 CONFIG_SHOW (Open/Close) */
} Turn_MenuState;

static PID_IncTypeDef turn_pid;

static Turn_RuntimeState turn_state = {
    .encoder_count = 0,
    .encoder_zero = 0,
    .output_sign = 0,
    .startup_hold_cnt = 0,
    .motor_enable = 0,
    .encoder_valid = 1,
};

static Turn_MenuState turn_menu_state = {
    .target_angle_deg = 0.0f,
    .encoder_value = 0.0f,
    .angle_deg = 0.0f,
    .motor_enable = 0,
    .output_sign = 0,
    .encoder_valid = 1,
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

    if(duty == 0)
    {
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, 0);
        turn_state.output_sign = 0;
        return;
    }

    int8 target_sign = (output >= 0.0f) ? (int8)1 : (int8)-1;

    /* 方向切换时先把两路都拉到 0, 下个周期再给新方向占空比, 避免瞬间两路同时 PWM */
    if(target_sign != turn_state.output_sign && turn_state.output_sign != 0)
    {
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, 0);
        turn_state.output_sign = 0;
        return;
    }

    if(target_sign > 0)
    {
        pwm_set_duty(Turn_PWM_REV, 0);
        pwm_set_duty(Turn_PWM_FWD, duty);
    }
    else
    {
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, duty);
    }
    turn_state.output_sign = target_sign;
}

void Turn_MenuTargetAngleSync(void)
{
    Turn_SetTargetAngleDeg(Turn_MenuAngleDegToEnc(turn_menu_state.target_angle_deg));
}

void Turn_MenuRuntimeUpdate(void)
{
    // encoder_value 显示(相对零点)累计编码器度数(°), angle_deg 显示车头实际角度(°)
    turn_menu_state.encoder_value =
        (float)(turn_state.encoder_count - turn_state.encoder_zero) / Turn_ENCODER_COUNT_PER_DEG;
    turn_menu_state.angle_deg     = turn_current_angle_deg;
    turn_menu_state.output_sign   = (int16)turn_state.output_sign;
    turn_menu_state.encoder_valid = turn_state.encoder_valid;

    /* 只在 enable 状态发生变化时才调用 SetMotorEnable，避免每周期触发 PID_clear */
    uint8 new_enable = (turn_menu_state.motor_enable != 0) ? 1U : 0U;
    if(new_enable != turn_state.motor_enable)
    {
        Turn_SetMotorEnable(new_enable);
    }
}

void Turn_Init(void)
{
    // 上电先把两路PWM脚当GPIO拉到安全电平，避免复用切换前后误驱动
    gpio_init(Turn_PWM_FWD_SAFE_PIN, GPO, Turn_PWM_SAFE_LEVEL, GPO_PUSH_PULL);
    gpio_init(Turn_PWM_REV_SAFE_PIN, GPO, Turn_PWM_SAFE_LEVEL, GPO_PUSH_PULL);

    // 再切换到PWM外设并保持0占空比
    pwm_init(Turn_PWM_FWD, Turn_MOTOR_FREQ, 0);
    pwm_init(Turn_PWM_REV, Turn_MOTOR_FREQ, 0);
    pwm_set_duty(Turn_PWM_FWD, 0);
    pwm_set_duty(Turn_PWM_REV, 0);

    turn_state.output_sign = 0;

    // 初始化增量式编码器 (方向判别模式: CH1=计数脉冲, CH2=方向电平)
    encoder_dir_init(Turn_ENCODER_INDEX, Turn_ENCODER_PULSE_PIN, Turn_ENCODER_DIR_INPUT_PIN);
    encoder_clear_count(Turn_ENCODER_INDEX);

    PID_Inc_Init(&turn_pid, Turn_PID_KP, Turn_PID_KI, Turn_PID_KD);

    turn_state.encoder_count    = 0;
    turn_state.encoder_zero     = 0;
    turn_state.startup_hold_cnt = Turn_STARTUP_HOLD_CYCLES;
    turn_state.motor_enable     = 0;
    turn_state.encoder_valid    = 1;
    turn_target_angle_deg  = 0.0f;
    turn_current_angle_deg = 0.0f;

    turn_menu_state.target_angle_deg = 0.0f;
    turn_menu_state.encoder_value    = 0.0f;
    turn_menu_state.angle_deg        = 0.0f;
    turn_menu_state.motor_enable     = 0;
    turn_menu_state.output_sign      = 0;
    turn_menu_state.encoder_valid    = 1;
}

void Turn_ControlTask(void)
{
    int16 delta_count = encoder_get_count(Turn_ENCODER_INDEX);
    encoder_clear_count(Turn_ENCODER_INDEX);

    turn_state.encoder_count += (int32)(Turn_ENCODER_SIGN * delta_count);

    // 车头角度 = (编码器累计计数 - 零点) / 每度计数 / 齿轮比
    turn_current_angle_deg = (float)(turn_state.encoder_count - turn_state.encoder_zero)
                              / Turn_ENCODER_COUNT_PER_DEG / Turn_GEAR_RATIO;

    // 上电抑制窗口: 等待编码器稳定, 把当前位置取样为零点, 不输出电机
    if(turn_state.startup_hold_cnt > 0)
    {
        turn_state.startup_hold_cnt--;
        turn_state.encoder_zero = turn_state.encoder_count;
        turn_current_angle_deg = 0.0f;
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, 0);
        turn_state.output_sign = 0;
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

int16 *Turn_GetMenuOutputSignPtr(void)
{
    return &turn_menu_state.output_sign;
}

uint8 *Turn_GetMenuEncoderValidPtr(void)
{
    return &turn_menu_state.encoder_valid;
}

void Turn_SetCurrentAngleAsZero(void)
{
    turn_state.encoder_zero = turn_state.encoder_count;
    turn_current_angle_deg = 0.0f;
    turn_target_angle_deg = 0.0f;
    PID_clear(&turn_pid);
}

void Turn_ResetTurns_MenuCallback(void)
{
    Turn_SetCurrentAngleAsZero();
    Beep_ShortRing();
}

void Turn_Stop(void)
{
    PID_clear(&turn_pid);
    pwm_set_duty(Turn_PWM_FWD, 0);
    pwm_set_duty(Turn_PWM_REV, 0);
    turn_state.output_sign = 0;
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

uint8 Turn_GetEncoderValid(void)
{
    return turn_state.encoder_valid;
}
