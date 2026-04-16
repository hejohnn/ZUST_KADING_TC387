/*
 * Uart_rs232.h
 *
 *  Created on: 2026年4月11日
 *      Author: 17706
 */

#ifndef CODE_UART_RS232_UART_RS232_H_
#define CODE_UART_RS232_UART_RS232_H_

#include "MYHEADFILE.h"

/* 初始化：在 cpu0_main 中调用，确保中断使能前 UART 已就绪 */
void  UartRs232_Init(void);

/* 踏板发送（菜单 Static Func 调用） */
void  UartRs232_PedalRuntimeUpdate(void);
uint8 *UartRs232_GetPedalEnablePtr(void);
float *UartRs232_GetPedalPercentShowPtr(void);

/* TC264 数据接收：在 uart1_rx_isr 中调用 */
void  UartRs232_Tc264RxHandler(void);

/* TC264 数据开关与显示值指针（供菜单绑定） */
uint8 *UartRs232_GetTc264RxEnablePtr(void);
float *UartRs232_GetTc264DistanceFloatPtr(void);
float *UartRs232_GetTc264SpeedFloatPtr(void);

/* TC264 数据整数值（供其他模块使用） */
int32  UartRs232_GetTc264DistanceMm(void);
int32  UartRs232_GetTc264SpeedMm_s(void);

#endif /* CODE_UART_RS232_UART_RS232_H_ */
