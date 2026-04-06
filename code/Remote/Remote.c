/*
 * Remote.c
 *
 *  Created on: 2026年4月5日
 *      Author: 17706
 */

#include "Remote.h"

#include "zf_device_uart_receiver.h"

#define REMOTE_SBUS_CH1_MIN          (240U)
#define REMOTE_SBUS_CH1_MAX          (1800U)
#define REMOTE_SBUS_CH1_SCALE_RANGE  ((float)(REMOTE_SBUS_CH1_MAX - REMOTE_SBUS_CH1_MIN))

static uint8 remote_steer_map_enable = 0;

static float Remote_MapSbusCh1ToTurnAngleDeg(uint16 ch1_raw)
{
	uint16 ch1_limited = ch1_raw;

	if(ch1_limited < REMOTE_SBUS_CH1_MIN)
	{
		ch1_limited = REMOTE_SBUS_CH1_MIN;
	}
	else if(ch1_limited > REMOTE_SBUS_CH1_MAX)
	{
		ch1_limited = REMOTE_SBUS_CH1_MAX;
	}

	return ((float)(ch1_limited - REMOTE_SBUS_CH1_MIN) * Turn_MENU_ANGLE_FULL_SCALE) / REMOTE_SBUS_CH1_SCALE_RANGE;
}

void Remote_Init(void)
{
	uart_receiver_init();
}

uint8 *Remote_GetSteerMapEnablePtr(void)
{
	return &remote_steer_map_enable;
}

void Remote_UpdateSteerBySbus(void)
{
	static uint16 last_ch1_raw = 0xFFFF;

	if(1 != uart_receiver.finsh_flag)
	{
		return;
	}

	uart_receiver.finsh_flag = 0;

	if(1 != uart_receiver.state)
	{
		return;
	}

	if(last_ch1_raw != uart_receiver.channel[0])
	{
		last_ch1_raw = uart_receiver.channel[0];
		if(remote_steer_map_enable)
		{
			Turn_SetMenuTargetAngleDeg(Remote_MapSbusCh1ToTurnAngleDeg(last_ch1_raw));
		}
	}
}




