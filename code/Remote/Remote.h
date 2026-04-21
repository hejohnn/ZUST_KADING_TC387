/*
 * Remote.h
 *
 *  Created on: 2026年4月5日
 *      Author: 17706
 */

#ifndef CODE_REMOTE_REMOTE_H_
#define CODE_REMOTE_REMOTE_H_

#include "MYHEADFILE.h"

/* 与 CCU60_CH1 的 PIT 周期保持一致, 见 cpu0_main.c */
#define REMOTE_TICK_MS              (2U)
#define REMOTE_LINK_TIMEOUT_MS      (100U)
#define REMOTE_LINK_TIMEOUT_TICKS   (REMOTE_LINK_TIMEOUT_MS / REMOTE_TICK_MS)

extern int16 menu_remote_link_ok;       /* 菜单只读: 1=链路正常, 0=失联 */

void   Remote_Init(void);
void   Remote_UpdateSteerBySbus(void);
uint8 *Remote_GetSteerMapEnablePtr(void);
uint8  Remote_IsLinkOk(void);


#endif /* CODE_REMOTE_REMOTE_H_ */
