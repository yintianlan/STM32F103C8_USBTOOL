#ifndef _SLAVETOOL_H_
#define _SLAVETOOL_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "def.h"
#include "FreeRTOS.h"
#include "task.h"

#if DEBUG
/* 定义Log打印接口 */
#define dbgprintf(fmt, ...)                do{\
                                                printf(fmt, ##__VA_ARGS__);\
                                                }while(0)
#else
#define dbgprintf(fmt, ...)                do{}while(0)
#endif

/* 定义时间基准 */
#define 	T_1MS		(1)
#define 	T_10MS		(10)
#define 	T_100MS		(100)
#define 	T_1S		(1000)
#define 	T_1MM		(T_1S*60)
/* 定义时间基准 */

/* 函数接口 */												  
void ResetUserTimer(uint32_t *Timer);
uint32_t ReadUserTimer(uint32_t *Timer);
void Delay1ms(volatile uint32_t nTime);												  
												  
void McuInit(void);
void McuDeInit(void);
void McuBasicTaskProc(void);
void HostCmdProcess(uint8_t *Buf, uint16_t Len);


#endif	/*_SLAVE_TOOL_H_*/
