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

#endif /* CODE_PEDAL_PEDAL_H_ */
