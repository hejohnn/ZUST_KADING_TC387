/*
 * Motor.c - DRV8701E 双轮直流电机驱动 (踏板线性控制), 详见 Motor.h
 */

#include "Motor.h"

int16 menu_motor_enable     = 0;
int16 menu_motor_direction  = 1;                    /* 1=前进, -1=后退 */
int16 menu_motor_duty_max   = MOTOR_DUTY_LIMIT;     /* 踏板映射上限, 运行时可改 */
int16 menu_motor_pedal_pct  = 0;
int16 menu_motor_left_duty  = 0;
int16 menu_motor_right_duty = 0;

static uint8 left_dir_level      = MOTOR_LEFT_DIR_FORWARD;
static uint8 right_dir_level     = MOTOR_RIGHT_DIR_FORWARD;

#define MOTOR_IS_ENABLED()  (menu_motor_enable != 0)

static uint32 motor_clamp_duty(int32 abs_duty)
{
    if(abs_duty <= 0)                       return 0;
    if(abs_duty >= MOTOR_DUTY_LIMIT)        return (uint32)MOTOR_DUTY_LIMIT;
    return (uint32)abs_duty;
}

void Motor_Init(void)
{
    /* 1. 上电先把 PWM 引脚以 GPIO 拉低, 防止外设切换瞬间误驱动 */
    gpio_init(MOTOR_LEFT_PWM_SAFE_PIN,  GPO, 0, GPO_PUSH_PULL);
    gpio_init(MOTOR_RIGHT_PWM_SAFE_PIN, GPO, 0, GPO_PUSH_PULL);

    /* 2. 方向引脚先拉到前进电平 */
    left_dir_level  = MOTOR_LEFT_DIR_FORWARD;
    right_dir_level = MOTOR_RIGHT_DIR_FORWARD;
    gpio_init(MOTOR_LEFT_DIR_PIN,  GPO, left_dir_level,  GPO_PUSH_PULL);
    gpio_init(MOTOR_RIGHT_DIR_PIN, GPO, right_dir_level, GPO_PUSH_PULL);

    /* 3. 切换 PWM 外设, 占空比保持 0 */
    pwm_init(MOTOR_LEFT_PWM,  MOTOR_PWM_FREQ, 0);
    pwm_init(MOTOR_RIGHT_PWM, MOTOR_PWM_FREQ, 0);
    pwm_set_duty(MOTOR_LEFT_PWM,  0);
    pwm_set_duty(MOTOR_RIGHT_PWM, 0);

    menu_motor_enable       = 0;
    menu_motor_pedal_pct    = 0;
    menu_motor_left_duty    = 0;
    menu_motor_right_duty   = 0;
}

void Motor_Stop(void)
{
    pwm_set_duty(MOTOR_LEFT_PWM,  0);
    pwm_set_duty(MOTOR_RIGHT_PWM, 0);
    menu_motor_left_duty  = 0;
    menu_motor_right_duty = 0;
}

void Motor_SetEnable(uint8 enable)
{
    menu_motor_enable = (enable != 0) ? 1 : 0;
    if(!menu_motor_enable)
    {
        Motor_Stop();
    }
}

uint8 Motor_GetEnable(void)
{
    return MOTOR_IS_ENABLED() ? 1U : 0U;
}

void Motor_ToggleEnable(void)
{
    Motor_SetEnable(MOTOR_IS_ENABLED() ? 0U : 1U);
    if(MOTOR_IS_ENABLED()) Beep_Timer_ShortRing();
}

void Motor_ToggleDirection(void)
{
    /* 切换方向前强制停车, 下一个周期按新方向重新加速 */
    Motor_Stop();
    menu_motor_direction = (menu_motor_direction > 0) ? (int16)-1 : (int16)1;
    Beep_Timer_ShortRing();
}

void Motor_SetLeftOutput(int32 output)
{
    uint32 duty;
    uint8  target_dir;

    if(output >= 0)
    {
        target_dir = MOTOR_LEFT_DIR_FORWARD;
        duty = motor_clamp_duty(output);
    }
    else
    {
        target_dir = (uint8)!MOTOR_LEFT_DIR_FORWARD;
        duty = motor_clamp_duty(-output);
    }

    if(duty == 0)
    {
        pwm_set_duty(MOTOR_LEFT_PWM, 0);
        menu_motor_left_duty = 0;
        return;
    }

    if(target_dir != left_dir_level)
    {
        /* 切向前先停, 避免 H 桥瞬间反向直通 */
        pwm_set_duty(MOTOR_LEFT_PWM, 0);
        left_dir_level = target_dir;
        gpio_set_level(MOTOR_LEFT_DIR_PIN, left_dir_level);
    }

    pwm_set_duty(MOTOR_LEFT_PWM, duty);
    menu_motor_left_duty = (int16)((output >= 0) ? (int32)duty : -(int32)duty);
}

void Motor_SetRightOutput(int32 output)
{
    uint32 duty;
    uint8  target_dir;

    if(output >= 0)
    {
        target_dir = MOTOR_RIGHT_DIR_FORWARD;
        duty = motor_clamp_duty(output);
    }
    else
    {
        target_dir = (uint8)!MOTOR_RIGHT_DIR_FORWARD;
        duty = motor_clamp_duty(-output);
    }

    if(duty == 0)
    {
        pwm_set_duty(MOTOR_RIGHT_PWM, 0);
        menu_motor_right_duty = 0;
        return;
    }

    if(target_dir != right_dir_level)
    {
        pwm_set_duty(MOTOR_RIGHT_PWM, 0);
        right_dir_level = target_dir;
        gpio_set_level(MOTOR_RIGHT_DIR_PIN, right_dir_level);
    }

    pwm_set_duty(MOTOR_RIGHT_PWM, duty);
    menu_motor_right_duty = (int16)((output >= 0) ? (int32)duty : -(int32)duty);
}

void Motor_UpdateFromPedal(void)
{
    /* 信号无效或未启用 -> 强制停车 */
    if(!MOTOR_IS_ENABLED() || !Pedal_IsSignalValid())
    {
        Motor_Stop();
        menu_motor_pedal_pct = 0;
        return;
    }

    /* 3. 取踏板百分比, 应用死区 */
    uint16 pct = Pedal_GetPercent();
    if(pct > 100U) pct = 100U;
    menu_motor_pedal_pct = (int16)pct;

    if(pct < MOTOR_PEDAL_DEADBAND_PCT)
    {
        Motor_Stop();
        return;
    }

    /* 4. 线性映射到 0..menu_motor_duty_max (上限 MOTOR_DUTY_LIMIT), 按方向符号输出 */
    int32 duty_max = menu_motor_duty_max;
    if(duty_max < 0)                        duty_max = 0;
    if(duty_max > MOTOR_DUTY_LIMIT)         duty_max = MOTOR_DUTY_LIMIT;

    int32 duty = ((int32)pct * duty_max) / 100;
    if(menu_motor_direction < 0) duty = -duty;

    Motor_SetLeftOutput (duty);
    Motor_SetRightOutput(duty);
}
