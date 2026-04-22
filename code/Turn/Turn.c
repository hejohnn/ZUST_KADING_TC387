/*
 * Turn.c
 *
 *  Created on: 2026年4月3日
 *      Author: 17706
 */

#include "Turn.h"
#include "zf_device_pd1503.h"
#include "zf_driver_flash.h"

#include <math.h>

volatile float turn_target_angle_deg = 0.0f;
volatile float turn_current_angle_deg = 0.0f;

typedef struct
{
    volatile uint16 encoder_raw;        // SPI绝对编码器原始读数 (0~65535)
    volatile float  encoder_abs_deg;    // 编码器当前单圈绝对角度 (0~360°)
    volatile float  encoder_last_deg;   // 上次单圈角度，用于检测跨零
    volatile int32  encoder_turns;      // 编码器整圈计数 (跨越0°/360°时±1)
    int8  output_sign;                  // 当前输出方向: +1=正转, -1=反转, 0=停车
    uint16 startup_hold_cnt;
    uint8 motor_enable;
    uint8 encoder_valid;                // 编码器通信是否正常
    uint8 first_read_done;              // 首次读数标志
} Turn_RuntimeState;

typedef struct
{
    float target_angle_deg;
    float encoder_value;
    float angle_deg;
    uint8 motor_enable;
    int16 output_sign;      /* 驱动当前输出符号 +1/-1/0, 菜单 INT_SHOW */
    uint8 encoder_valid;    /* 编码器通信状态, 菜单 CONFIG_SHOW (Open/Close) */
} Turn_MenuState;

static PID_IncTypeDef turn_pid;

static pd1503_data_struct turn_encoder_data;  // PD1503编码器数据结构体

static Turn_RuntimeState turn_state = {
    .encoder_raw = 0,
    .encoder_abs_deg = 0.0f,
    .encoder_last_deg = 0.0f,
    .encoder_turns = 0,
    .output_sign = 0,
    .startup_hold_cnt = 0,
    .motor_enable = 0,
    .encoder_valid = 0,
    .first_read_done = 0,
};
static Turn_MenuState turn_menu_state = {
    .target_angle_deg = 0.0f,
    .encoder_value = 0.0f,
    .angle_deg = 0.0f,
    .motor_enable = 0,
    .output_sign = 0,
    .encoder_valid = 0,
};

//-------------------------------------------------------------------------------------------------------------------
// 单圈角度差值计算 (处理0/360°跨零)
// 返回值范围 [-180, +180)
//-------------------------------------------------------------------------------------------------------------------
static float Turn_AngleDiffDeg(float angle_deg, float center_deg)
{
    float diff = angle_deg - center_deg;
    while(diff > 180.0f)  diff -= 360.0f;
    while(diff < -180.0f) diff += 360.0f;
    return diff;
}

//-------------------------------------------------------------------------------------------------------------------
// 从Flash恢复圈数
//-------------------------------------------------------------------------------------------------------------------
static void Turn_LoadFromFlash(void)
{
    flash_read_page_to_buffer(Turn_FLASH_SECTOR, Turn_FLASH_PAGE);
    uint32 magic = flash_union_buffer[0].uint32_type;
    if(magic == Turn_FLASH_MAGIC)
    {
        turn_state.encoder_turns = flash_union_buffer[1].int32_type;
    }
    else
    {
        // Flash无有效数据，默认0圈
        turn_state.encoder_turns = 0;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 保存圈数到Flash
//-------------------------------------------------------------------------------------------------------------------
void Turn_SavePositionToFlash(void)
{
    flash_buffer_clear();
    flash_union_buffer[0].uint32_type = Turn_FLASH_MAGIC;
    flash_union_buffer[1].int32_type  = turn_state.encoder_turns;
    flash_erase_page(Turn_FLASH_SECTOR, Turn_FLASH_PAGE);
    flash_write_page_from_buffer(Turn_FLASH_SECTOR, Turn_FLASH_PAGE);
}

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
        // encoder_value 显示编码器多圈总角度(°)，angle_deg 显示车头实际角度(°)
        float total_deg = turn_state.encoder_turns * 360.0f + turn_state.encoder_abs_deg;
        turn_menu_state.encoder_value = total_deg - Turn_ENCODER_CENTER_DEG;
        turn_menu_state.angle_deg = turn_current_angle_deg;
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

        // 初始化 PD1503 SPI绝对编码器 (替代原来的GPT12增量编码器)
    if(pd1503_init() == 0)
    {
        turn_state.encoder_valid = 1;
    }
    else
    {
        turn_state.encoder_valid = 0;
    }

    PID_Inc_Init(&turn_pid, Turn_PID_KP, Turn_PID_KI, Turn_PID_KD);

        // 从Flash恢复圈数
        Turn_LoadFromFlash();

        turn_state.encoder_raw = 0;
        turn_state.encoder_abs_deg = 0.0f;
        turn_state.encoder_last_deg = 0.0f;
        turn_state.first_read_done = 0;
        turn_state.startup_hold_cnt = Turn_STARTUP_HOLD_CYCLES;
    turn_state.motor_enable = 0;
    turn_target_angle_deg = 0.0f;
    turn_current_angle_deg = 0.0f;

    turn_menu_state.target_angle_deg = 0.0f;
    turn_menu_state.encoder_value = 0.0f;
    turn_menu_state.angle_deg = 0.0f;
    turn_menu_state.motor_enable = 0;
    turn_menu_state.output_sign   = 0;
    turn_menu_state.encoder_valid = turn_state.encoder_valid;
}

void Turn_ControlTask(void)
{
    // 通过SPI读取PD1503绝对编码器角度
    uint8 read_ret = pd1503_read_angle(&turn_encoder_data);
    if(read_ret == 0)
    {
        float now_deg = turn_encoder_data.angle_deg * (float)Turn_ENCODER_SIGN;
        turn_state.encoder_raw = turn_encoder_data.angle_count;
        turn_state.encoder_valid = 1;

        if(!turn_state.first_read_done)
        {
            // 首次读数，仅记录，不检测跨零
            turn_state.encoder_abs_deg = now_deg;
            turn_state.encoder_last_deg = now_deg;
            turn_state.first_read_done = 1;
        }
        else
        {
            // 检测跨零：相邻两次采样差值超过180°认为跨越了0°/360°边界
            float delta = now_deg - turn_state.encoder_last_deg;
            if(delta > 180.0f)
            {
                // 从高角度跳到低角度 (例如 350°→10°)，反向跨零，圈数-1
                turn_state.encoder_turns--;
                Turn_SavePositionToFlash();
            }
            else if(delta < -180.0f)
            {
                // 从低角度跳到高角度 (例如 10°→350°)，正向跨零，圈数+1
                turn_state.encoder_turns++;
                Turn_SavePositionToFlash();
            }
            turn_state.encoder_abs_deg = now_deg;
            turn_state.encoder_last_deg = now_deg;
        }
    }
    else
    {
        // 通信失败，保持上次值不更新，不输出电机
        turn_state.encoder_valid = 0;
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, 0);
        turn_state.output_sign = 0;
        return;
    }

    // 上电抑制窗口：等待编码器读数稳定，不输出电机
    if(turn_state.startup_hold_cnt > 0)
    {
        turn_state.startup_hold_cnt--;
        turn_current_angle_deg = 0.0f;
        pwm_set_duty(Turn_PWM_FWD, 0);
        pwm_set_duty(Turn_PWM_REV, 0);
        turn_state.output_sign = 0;
        return;
    }

    // 编码器多圈总角度 = 圈数*360 + 当前单圈角度
    // 车头角度 = (总角度 - 中位角度) / 齿轮比
    float total_encoder_deg = turn_state.encoder_turns * 360.0f + turn_state.encoder_abs_deg;
    turn_current_angle_deg = (total_encoder_deg - Turn_ENCODER_CENTER_DEG) / Turn_GEAR_RATIO;

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
    // 将当前位置设为新的零点：重置圈数使当前总角度等于中位
    turn_state.encoder_turns = 0;
    Turn_SavePositionToFlash();
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
    // 返回编码器原始count值
    return (int32)turn_state.encoder_raw;
}

float Turn_GetEncoderTotalDeg(void)
{
    // 返回编码器多圈总角度 (相对中位)
    float total = turn_state.encoder_turns * 360.0f + turn_state.encoder_abs_deg;
    return total - Turn_ENCODER_CENTER_DEG;
}

int32 Turn_GetEncoderTurns(void)
{
    return turn_state.encoder_turns;
}

float Turn_GetCurrentAngleDeg(void)
{
    return turn_current_angle_deg;
}

uint16 Turn_GetEncoderRaw(void)
{
    return turn_state.encoder_raw;
}

float Turn_GetEncoderAbsAngleDeg(void)
{
    // 返回编码器当前单圈绝对角度 (0~360°)
    return turn_state.encoder_abs_deg;
}

uint8 Turn_GetEncoderValid(void)
{
    return turn_state.encoder_valid;
}

