/*********************************************************************************************************************
* 后轮编码器应用模块 - 双轮 (左/右), 详见 encoder_app.h 头文说明
* 方案二: 把 T5/T2 读+清放进 interrupt_global_disable/enable 原子区, PIT 周期拉到 50ms
********************************************************************************************************************/

#include "encoder_app.h"
#include "IfxGpt12.h"

/* ---------------------- 模块内部状态 (左/右各保持一份) ---------------------- */
static volatile int32  total_count_left  = 0;
static volatile int32  total_count_right = 0;
static volatile int32  last_count_left   = 0;
static volatile int32  last_count_right  = 0;
static volatile int32  speed_mm_left     = 0;
static volatile int32  speed_mm_right    = 0;
static volatile uint32 ms_tick           = 0;
static volatile uint8  speed_flag        = 0;

/* ---------------------- 菜单显示用的缓存值 ---------------------- */
int16 menu_distance_left_cm  = 0;
int16 menu_speed_left_cm_s   = 0;
int16 menu_distance_right_cm = 0;
int16 menu_speed_right_cm_s  = 0;

/* ---------------------- 初始化 ---------------------- */
void encoder_app_init(void)
{
    // 左轮: TIM5, A=P21.7, B=P21.6
    encoder_quad_init(TIM5_ENCODER, TIM5_ENCODER_CH1_P21_7, TIM5_ENCODER_CH2_P21_6);
    // 右轮: TIM2, A=P33.7, B=P33.6
    encoder_quad_init(TIM2_ENCODER, TIM2_ENCODER_CH1_P33_7, TIM2_ENCODER_CH2_P33_6);

    total_count_left   = 0;
    total_count_right  = 0;
    last_count_left    = 0;
    last_count_right   = 0;
    speed_mm_left      = 0;
    speed_mm_right     = 0;
    ms_tick            = 0;
    speed_flag         = 0;

    menu_distance_left_cm  = 0;
    menu_speed_left_cm_s   = 0;
    menu_distance_right_cm = 0;
    menu_speed_right_cm_s  = 0;
}

/* ---------------------- PIT 中断服务 (50ms, 由 cc61_pit_ch0_isr 调用) ---------------------- */
void encoder_app_pit_handler(void)
{
    int16 dl, dr;
    int32 dtl, dtr;

    // 直接读取硬件寄存器原始值 (四倍频, 不经过库函数 /4 处理)
    // 用 interrupt_global_disable/enable 保护读+清序列, 避免 2ms 旋转编码器 ISR 插入扰动
    interrupt_global_disable();
    dl = (int16)IfxGpt12_T5_getTimerValue(&MODULE_GPT120);
    IfxGpt12_T5_setTimerValue(&MODULE_GPT120, 0);
    dr = (int16)IfxGpt12_T2_getTimerValue(&MODULE_GPT120);
    IfxGpt12_T2_setTimerValue(&MODULE_GPT120, 0);
    interrupt_global_enable(0);

    // 左轮 (T5) 实测计数为右轮 (T2) 的 1/4, 在累计时 ×4 对齐到同样的 4000/圈
    // 左轮接线方向与右轮相反, 取负使正向行驶时两轮计数同号
    total_count_left  += -(int32)dl * 4;
    total_count_right += dr;

    ms_tick += ENCODER_PIT_PERIOD_MS;
    if (ms_tick >= SPEED_SAMPLE_MS)
    {
        ms_tick = 0;

        dtl = total_count_left  - last_count_left;
        dtr = total_count_right - last_count_right;
        last_count_left  = total_count_left;
        last_count_right = total_count_right;

        // 速度 (mm/s) = delta * DISTANCE_PER_REV_MM * 1000 / ENCODER_PPR / SPEED_SAMPLE_MS
        speed_mm_left  = dtl * (int32)DISTANCE_PER_REV_MM_LEFT  * 1000
                            / (int32)ENCODER_PPR / (int32)SPEED_SAMPLE_MS;
        speed_mm_right = dtr * (int32)DISTANCE_PER_REV_MM_RIGHT * 1000
                            / (int32)ENCODER_PPR / (int32)SPEED_SAMPLE_MS;
        speed_flag = 1;

        menu_speed_left_cm_s  = (int16)(speed_mm_left  / 10);
        menu_speed_right_cm_s = (int16)(speed_mm_right / 10);
    }

    // 距离每个 PIT 周期都更新一次, 菜单刷新感平滑
    menu_distance_left_cm  = (int16)(total_count_left  * (int32)DISTANCE_PER_REV_MM_LEFT
                                / (int32)ENCODER_PPR / 10);
    menu_distance_right_cm = (int16)(total_count_right * (int32)DISTANCE_PER_REV_MM_RIGHT
                                / (int32)ENCODER_PPR / 10);
}

/* ---------------------- 对外接口 ---------------------- */
int32 encoder_app_get_count_left  (void) { return total_count_left;  }
int32 encoder_app_get_count_right (void) { return total_count_right; }

int32 encoder_app_get_distance_mm_left  (void)
{
    return total_count_left  * (int32)DISTANCE_PER_REV_MM_LEFT  / (int32)ENCODER_PPR;
}
int32 encoder_app_get_distance_mm_right (void)
{
    return total_count_right * (int32)DISTANCE_PER_REV_MM_RIGHT / (int32)ENCODER_PPR;
}

int32 encoder_app_get_speed_mm_s_left  (void) { return speed_mm_left;  }
int32 encoder_app_get_speed_mm_s_right (void) { return speed_mm_right; }

uint8 encoder_app_speed_updated(void)
{
    if (speed_flag)
    {
        speed_flag = 0;
        return 1;
    }
    return 0;
}

void encoder_app_reset(void)
{
    total_count_left   = 0;
    total_count_right  = 0;
    last_count_left    = 0;
    last_count_right   = 0;
    speed_mm_left      = 0;
    speed_mm_right     = 0;
    ms_tick            = 0;

    menu_distance_left_cm  = 0;
    menu_speed_left_cm_s   = 0;
    menu_distance_right_cm = 0;
    menu_speed_right_cm_s  = 0;

    encoder_clear_count(TIM5_ENCODER);
    encoder_clear_count(TIM2_ENCODER);
}
