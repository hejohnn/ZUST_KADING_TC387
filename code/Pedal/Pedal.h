/*
 * Pedal.h
 *
 *  Created on: 2026年4月11日
 *      Author: 17706
 */

#ifndef CODE_PEDAL_PEDAL_H_
#define CODE_PEDAL_PEDAL_H_

#include "MYHEADFILE.h"

void Pedal_Init(void);
void Pedal_Update(void);

uint16 Pedal_GetRaw(void);
uint16 Pedal_GetVoltageMv(void);
uint16 Pedal_GetPercent(void);
uint16 Pedal_GetPercentRaw(void);
uint16 Pedal_GetMinRaw(void);
uint16 Pedal_GetMaxRaw(void);
uint8 Pedal_IsSignalValid(void);




#endif /* CODE_PEDAL_PEDAL_H_ */
