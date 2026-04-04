#ifndef _LED_TEST_CTRL_H_
#define _LED_TEST_CTRL_H_

#include "zf_common_typedef.h"

void led_test_on(void);
void led_test_off(void);
void led_test_left_arrow(void);
void led_test_right_arrow(void);
void led_test_show_custom_bitmap(const uint16 *bitmap_rows);
void led_test_task(void);
uint8 led_test_need_dot_scan(void);

#endif