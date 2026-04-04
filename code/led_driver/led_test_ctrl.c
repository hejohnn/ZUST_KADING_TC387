#include "led_test_ctrl.h"

#include "zf_driver_delay.h"
#include "zf_device_tld7002.h"
#include "zf_device_dot_matrix_screen.h"

static uint8 led_tld_inited = 0;
static uint8 led_dot_matrix_inited = 0;
static uint8 led_force_off_lock = 0;
static volatile uint8 led_need_dot_scan = 0;

// CH15用于TLD7002同步信号，不能被清零，否则点阵扫描中断会失效。
#define LED_TLD_SYNC_DUTY (5000)

static const uint16 led_all_off_bitmap[7] = {0, 0, 0, 0, 0, 0, 0};
static const uint16 led_all_on_bitmap[7]  = {0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF};

static const uint16 led_left_arrow_bitmap[7] =
{
    // 7x15自定义图像模板：每行低15位有效，bit14为最左列，bit0为最右列
    // 你可以直接修改下面7行数据来绘制自己的左箭头图案

    0x0001,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    // 0x0010,
    // 0x0018,
    // 0x3FFC,
    // 0x3FFE,
    // 0x3FFC,
    // 0x0018,
    // 0x0010,
};

static const uint16 led_right_arrow_bitmap[7] =
{
    0x0010,
    0x0018,
    0x3FFC,
    0x3FFE,
    0x3FFC,
    0x0018,
    0x0010,
};
static void led_test_tld_init_if_needed(void)
{
    if(!led_tld_inited)
    {
        tld7002_init();
        led_tld_inited = 1;
    }
}

static void led_test_dot_matrix_init_if_needed(void)
{
    if(!led_dot_matrix_inited)
    {
        dot_matrix_screen_init();
        led_dot_matrix_inited = 1;
    }
}

static void led_test_apply_all_off(void)
{
    uint8 i;

    tld7002_gpin0_set_level(1);
    for(i = 0; i < 15; i++)
    {
        tld7002_duty[i] = 0;
    }
    tld7002_duty[15] = LED_TLD_SYNC_DUTY;
    tld7002_set_duty(1);
}

void led_test_on(void)
{
    led_test_tld_init_if_needed();
    led_test_dot_matrix_init_if_needed();
    led_force_off_lock = 0;
    led_need_dot_scan = 0;

    tld7002_gpin0_set_level(1);
    dot_matrix_screen_reset_scan_phase();
    dot_matrix_screen_set_brightness(9000);
    dot_matrix_screen_show_bitmap_7x15(led_all_on_bitmap);
    led_need_dot_scan = 1;
}

void led_test_off(void)
{
    led_test_tld_init_if_needed();
    led_test_dot_matrix_init_if_needed();
    led_force_off_lock = 1;
    led_need_dot_scan = 0;
    dot_matrix_screen_reset_scan_phase();
    dot_matrix_screen_show_bitmap_7x15(led_all_off_bitmap);
    led_test_apply_all_off();
}

void led_test_left_arrow(void)
{
    led_test_dot_matrix_init_if_needed();

    led_force_off_lock = 0;
    led_need_dot_scan = 0;
    dot_matrix_screen_reset_scan_phase();
    dot_matrix_screen_set_brightness(9000);
    dot_matrix_screen_show_bitmap_7x15(led_left_arrow_bitmap);
    led_need_dot_scan = 1;
}

void led_test_right_arrow(void)
{
    led_test_dot_matrix_init_if_needed();

    led_force_off_lock = 0;
    led_need_dot_scan = 0;
    dot_matrix_screen_reset_scan_phase();
    dot_matrix_screen_set_brightness(9000);
    dot_matrix_screen_show_bitmap_7x15(led_right_arrow_bitmap);
    led_need_dot_scan = 1;
}

void led_test_show_custom_bitmap(const uint16 *bitmap_rows)
{
    if(0 == bitmap_rows)
    {
        return;
    }

    led_test_dot_matrix_init_if_needed();

    led_force_off_lock = 0;
    led_need_dot_scan = 0;
    dot_matrix_screen_reset_scan_phase();
    dot_matrix_screen_set_brightness(9000);
    dot_matrix_screen_show_bitmap_7x15(bitmap_rows);
    led_need_dot_scan = 1;
}

uint8 led_test_need_dot_scan(void)
{
    return led_need_dot_scan;
}

void led_test_task(void)
{
    static uint16 led_keep_off_div = 0;

    if(led_force_off_lock)
    {
        if(++led_keep_off_div >= 20)
        {
            led_keep_off_div = 0;
            led_test_apply_all_off();
        }
    }
    else
    {
        led_keep_off_div = 0;
    }
}
