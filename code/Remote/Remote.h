/*
 * Remote.h
 *
 *  Created on: 2026年4月5日
 *      Author: 17706
 */

#ifndef CODE_REMOTE_REMOTE_H_
#define CODE_REMOTE_REMOTE_H_

#include "MYHEADFILE.h"

void Remote_Init(void);
void Remote_UpdateSteerBySbus(void);
uint8 *Remote_GetSteerMapEnablePtr(void);
int16 *Remote_GetSbusCh1Ptr(void);


#endif /* CODE_REMOTE_REMOTE_H_ */
