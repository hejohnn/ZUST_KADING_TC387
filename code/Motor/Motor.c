/*
 * Motor.c - 双 EG2104 半桥 双轮直流电机驱动 (踏板线性控制), 详见 Motor.h
 */

#include "Motor.h"

int16 menu_motor_enable     = 0;
int16 menu_motor_direction  = 1;                    /* 1=前进, -1=后退 */
int16 menu_motor_duty_max   = MOTOR_DUTY_LIMIT;     /* 踏板映射上限, 运行时可改 */
int16 menu_motor_pedal_pct  = 0;
int16 menu_motor_left_duty  = 0;
int16 menu_motor_right_duty = 0;

static int8 left_output_sign  = 0;                  /* +1=正转, -1=反转, 0=停车 */
static int8 right_output_sign = 0;

#define MOTOR_IS_ENABLED()  (menu_motor_enable != 0)

static uint32 motor_clamp_duty(int32 abs_duty)
{
    if(abs_duty <= 0)                       return 0;
    if(abs_duty >= MOTOR_DUTY_LIMIT)        return (uint32)MOTOR_DUTY_LIMIT;
    return (uint32)abs_duty;
}

void Motor_Init(void)
{
    /* 1. 上电先把 4 路 PWM 引脚以 GPIO 拉低, 防止外设切换瞬间误驱动 */
    gpio_init(MOTOR_LEFT_PWM_FWD_SAFE_PIN,  GPO, 0, GPO_PUSH_PULL);
    gpio_init(MOTOR_LEFT_PWM_REV_SAFE_PIN,  GPO, 0, GPO_PUSH_PULL);
    gpio_init(MOTOR_RIGHT_PWM_FWD_SAFE_PIN, GPO, 0, GPO_PUSH_PULL);
    gpio_init(MOTOR_RIGHT_PWM_REV_SAFE_PIN, GPO, 0, GPO_PUSH_PULL);

    /* 2. 切换 PWM 外设, 占空比保持 0 */
    pwm_init(MOTOR_LEFT_PWM_FWD,  MOTOR_PWM_FREQ, 0);
    pwm_init(MOTOR_LEFT_PWM_REV,  MOTOR_PWM_FREQ, 0);
    pwm_init(MOTOR_RIGHT_PWM_FWD, MOTOR_PWM_FREQ, 0);
    pwm_init(MOTOR_RIGHT_PWM_REV, MOTOR_PWM_FREQ, 0);
    pwm_set_duty(MOTOR_LEFT_PWM_FWD,  0);
    pwm_set_duty(MOTOR_LEFT_PWM_REV,  0);
    pwm_set_duty(MOTOR_RIGHT_PWM_FWD, 0);
    pwm_set_duty(MOTOR_RIGHT_PWM_REV, 0);

    left_output_sign  = 0;
    right_output_sign = 0;

    menu_motor_enable       = 0;
    menu_motor_pedal_pct    = 0;
    menu_motor_left_duty    = 0;
    menu_motor_right_duty   = 0;
}

void Motor_Stop(void)
{
    pwm_set_duty(MOTOR_LEFT_PWM_FWD,  0);
    pwm_set_duty(MOTOR_LEFT_PWM_REV,  0);
    pwm_set_duty(MOTOR_RIGHT_PWM_FWD, 0);
    pwm_set_duty(MOTOR_RIGHT_PWM_REV, 0);
    left_output_sign      = 0;
    right_output_sign     = 0;
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
    int8   target_sign = (output > 0) ? (int8)1 : ((output < 0) ? (int8)-1 : (int8)0);
    uint32 duty        = motor_clamp_duty((output >= 0) ? output : -output);

    if(duty == 0)
    {
        pwm_set_duty(MOTOR_LEFT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_LEFT_PWM_REV, 0);
        left_output_sign     = 0;
        menu_motor_left_duty = 0;
        return;
    }

    /* 方向切换: 本周期先两路归零, 下一周期再给新方向 PWM, 避免半桥瞬间反向直通 */
    if(target_sign != left_output_sign && left_output_sign != 0)
    {
        pwm_set_duty(MOTOR_LEFT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_LEFT_PWM_REV, 0);
        left_output_sign     = 0;
        menu_motor_left_duty = 0;
        return;
    }

    if(target_sign > 0)
    {
        pwm_set_duty(MOTOR_LEFT_PWM_REV, 0);
        pwm_set_duty(MOTOR_LEFT_PWM_FWD, duty);
    }
    else
    {
        pwm_set_duty(MOTOR_LEFT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_LEFT_PWM_REV, duty);
    }
    left_output_sign     = target_sign;
    menu_motor_left_duty = (int16)((target_sign > 0) ? (int32)duty : -(int32)duty);
}

void Motor_SetRightOutput(int32 output)
{
    int8   target_sign = (output > 0) ? (int8)1 : ((output < 0) ? (int8)-1 : (int8)0);
    uint32 duty        = motor_clamp_duty((output >= 0) ? output : -output);

    if(duty == 0)
    {
        pwm_set_duty(MOTOR_RIGHT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_RIGHT_PWM_REV, 0);
        right_output_sign     = 0;
        menu_motor_right_duty = 0;
        return;
    }

    if(target_sign != right_output_sign && right_output_sign != 0)
    {
        pwm_set_duty(MOTOR_RIGHT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_RIGHT_PWM_REV, 0);
        right_output_sign     = 0;
        menu_motor_right_duty = 0;
        return;
    }

    if(target_sign > 0)
    {
        pwm_set_duty(MOTOR_RIGHT_PWM_REV, 0);
        pwm_set_duty(MOTOR_RIGHT_PWM_FWD, duty);
    }
    else
    {
        pwm_set_duty(MOTOR_RIGHT_PWM_FWD, 0);
        pwm_set_duty(MOTOR_RIGHT_PWM_REV, duty);
    }
    right_output_sign     = target_sign;
    menu_motor_right_duty = (int16)((target_sign > 0) ? (int32)duty : -(int32)duty);
}

void Motor_UpdateFromPedal(void)
{
    /* 1. 无论是否使能, 都刷新踏板百分比到菜单, 方便上电前预览踏板信号 */
    uint16 pct = Pedal_IsSignalValid() ? Pedal_GetPercent() : 0U;
    if(pct > 100U) pct = 100U;
    menu_motor_pedal_pct = (int16)pct;

    /* 2. 菜单侧可能把 DutyMax 改到越界值, 这里回写一次 clamp, 让显示与实际生效值一致 */
    if(menu_motor_duty_max < 0)                        menu_motor_duty_max = 0;
    if(menu_motor_duty_max > MOTOR_DUTY_LIMIT)         menu_motor_duty_max = MOTOR_DUTY_LIMIT;

    /* 3. 未使能 / 信号无效 / 踏板在死区 -> 停车, 不下发 duty */
    if(!MOTOR_IS_ENABLED() || !Pedal_IsSignalValid() || pct < MOTOR_PEDAL_DEADBAND_PCT)
    {
        Motor_Stop();
        return;
    }

    /* 4. 线性映射到 0..menu_motor_duty_max, 按方向符号输出 */
    int32 duty = ((int32)pct * menu_motor_duty_max) / 100;
    if(menu_motor_direction < 0) duty = -duty;

    Motor_SetLeftOutput (duty);
    Motor_SetRightOutput(duty);
}
