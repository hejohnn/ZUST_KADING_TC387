/*
 * Pedal.h
 *
 *  Created on: 2026年4月11日
 *      Author: 17706
 */

#ifndef CODE_PEDAL_PEDAL_H_
#define CODE_PEDAL_PEDAL_H_

#include "MYHEADFILE.h"

void   Pedal_Init(void);
void   Pedal_Update(void);
uint16 Pedal_GetPercent(void);       /* 0..100, 已滤波 */
uint8  Pedal_IsSignalValid(void);    /* min/max 学习完成且 span >= 阈值 */

/* 调试用: 直接暴露 ADC 原始值 / 学习到的 min / max / signal_valid
 * 菜单 INT_SHOW / CONFIG_SHOW 使用, 方便定位硬件/软件问题 */
int16 *Pedal_GetMenuRawPtr(void);
int16 *Pedal_GetMenuMinPtr(void);
int16 *Pedal_GetMenuMaxPtr(void);
uint8 *Pedal_GetMenuValidPtr(void);

#endif /* CODE_PEDAL_PEDAL_H_ */
