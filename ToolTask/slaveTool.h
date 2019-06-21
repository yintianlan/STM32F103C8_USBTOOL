#ifndef _SLAVETOOL_H_
#define _SLAVETOOL_H_

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "def.h"

#include "checkAdc.h"


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

#define PASS (0)
#define FAIL (1)

/* Chip parameters ------------------------------------------------------------*/
#define 	FLASH_BASE_ADDR							(0x08000000)
#define 	FLASH_USER_START_ADDR					(0x08002800)
#define 	SYSTEM_BASE_ADDR						(FLASH_USER_START_ADDR + 0x100)
#define 	SRAM_SIZE								(0x5000)		/*芯片SRAM大小，STM32F103C8 Sram = 20k*/
#define		FLASH_SIZE								(0x10000)		/*芯片flash大小，STM32F103C8  64k*/
#define 	SRAM_BASE_ADDR							(0x20000000)
#define 	PARAMETER_START_ADDR					(SRAM_BASE_ADDR + SRAM_SIZE - 0x100)
/* ---------------------------------------------------------------------------- */

/*系统保留的参数分区，不被编译器分配*/
typedef struct
{
	unsigned short SetVolValue;
	unsigned char SetRemoteCh;
	unsigned char RemoteCh1[3];
	unsigned char RemoteCh2[3];
}structSysData;

/* 继电器工作状态 */
typedef enum
{
  RELAYState_ON = 0,     // (公共端——常闭)
  RELAYState_OFF,        // (公共端——常开)  
}RELAYState_TypeDef;

/* 继电器矩阵状态 */
typedef struct
{
	uint8	RemoteChooseState;
	uint8	LineState;
	uint8	ColumnState;
	uint16_t	BatteryVoltage;
}structRelayState;

/*外部引用*/
extern structSysData *const tpDataInfo;
extern structRelayState tRelayState;


/* 函数接口 */
void ResetUserTimer(uint32_t *Timer);
uint32_t ReadUserTimer(uint32_t *Timer);
void Delay1ms(volatile uint32_t nTime);												  
void MX_GPIO_Write(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void MX_GPIO_Toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void MX_IWDG_Refresh(void);

void McuInit(void);
void McuDeInit(void);
void McuBasicTaskProc(void);
void HostCmdProcess(uint8_t *Buf, uint16_t Len);


#endif	/*_SLAVE_TOOL_H_*/
