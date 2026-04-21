/*
 * Remote.c - SBUS 遥控器接收 + 失联保护
 *
 * 失联判定: 连续 REMOTE_LINK_TIMEOUT_MS 内没有"帧完成且 state=1"的有效事件.
 * 失联动作(边沿触发一次): 转向回中 + 关电机 + 短鸣.
 * 恢复不会自动重开电机, 必须用户在菜单里手动 Toggle_EN, 避免意外复位后瞬间暴冲.
 */

#include "Remote.h"

#include "zf_device_uart_receiver.h"

#define REMOTE_SBUS_CH1_MIN          (240U)
#define REMOTE_SBUS_CH1_MAX          (1800U)
#define REMOTE_SBUS_CH1_SCALE_RANGE  ((float)(REMOTE_SBUS_CH1_MAX - REMOTE_SBUS_CH1_MIN))

static uint8  remote_steer_map_enable = 0;
static uint16 remote_miss_ticks       = REMOTE_LINK_TIMEOUT_TICKS;  /* 启动默认视为未收到 */
int16 menu_remote_link_ok             = 0;

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

static void Remote_EnterFailsafe(void)
{
	/* 只在 OK -> LOST 边沿执行一次, 避免重复刷转向/鸣响 */
	if(!menu_remote_link_ok) return;

	menu_remote_link_ok = 0;
	Turn_SetMenuTargetAngleDeg(0.0f);
	Motor_SetEnable(0);
	Beep_Timer_ShortRing();
}

static void Remote_RestoreLink(void)
{
	if(menu_remote_link_ok) return;
	menu_remote_link_ok = 1;
	/* 不自动恢复电机使能, 等用户确认 */
}

void Remote_Init(void)
{
	uart_receiver_init();
	remote_miss_ticks   = REMOTE_LINK_TIMEOUT_TICKS;
	menu_remote_link_ok = 0;
}

uint8 *Remote_GetSteerMapEnablePtr(void)
{
	return &remote_steer_map_enable;
}

uint8 Remote_IsLinkOk(void)
{
	return menu_remote_link_ok ? 1U : 0U;
}

void Remote_UpdateSteerBySbus(void)
{
	static uint16 last_ch1_raw = 0xFFFF;
	uint8 fresh_valid = 0U;

	/* 消费一帧新数据: finsh_flag=1 且 state=1 才算"有效帧"
	 * state!=1 (接收机报失控) 的帧等同于无帧, 计入 miss 计数 */
	if(uart_receiver.finsh_flag == 1U)
	{
		uart_receiver.finsh_flag = 0U;
		if(uart_receiver.state == 1U)
		{
			fresh_valid = 1U;
		}
	}

	if(fresh_valid)
	{
		remote_miss_ticks = 0U;
		Remote_RestoreLink();

		if(last_ch1_raw != uart_receiver.channel[0])
		{
			last_ch1_raw = uart_receiver.channel[0];
			if(remote_steer_map_enable)
			{
				Turn_SetMenuTargetAngleDeg(Remote_MapSbusCh1ToTurnAngleDeg(last_ch1_raw));
			}
		}
	}
	else
	{
		if(remote_miss_ticks < REMOTE_LINK_TIMEOUT_TICKS)
		{
			remote_miss_ticks++;
		}
		if(remote_miss_ticks >= REMOTE_LINK_TIMEOUT_TICKS)
		{
			Remote_EnterFailsafe();
		}
	}
}
