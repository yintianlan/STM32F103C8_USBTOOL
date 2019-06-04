#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include "stm32f1xx_hal.h"
#include "tinyprintf.h"
#include "FlyConfig.h"

#include "def.h"

#include "cmsis_os.h"

/* Flash最后地址 */
#define LAST_FLASH_ADDRESS		(0x0801FC00)

/* 定义喂狗接口       */
#define FeedDog()				do{IWDG->KR = 0x0000AAAAU;}while(0)
/* 定义喂狗接口       */

/* 进出临界区但不关闭中断*/
#define TaskSuspendSchedule		do{vTaskSuspendAll();}while(0)
#define TaskResumeSchedule		do{xTaskResumeAll();}while(0)
/* 进出临界区但不关闭中断*/

/*打开Logo*/
#if DEBUG_OUT_LEVEL
#define PRINTF_ENABLE			(1)
#endif

/* 定义log等级 */

/* 定义Log打印接口 */
#define FlyDebugPrint(level, fmt, args...)              do{\
                                                            if(level & DEBUG_OUT_LEVEL){\
                                                            	  TaskSuspendSchedule;\
                                                            	  tfp_printf("\r\n[%s]", #level);\
                                                                  tfp_printf(fmt, ##args);\
                                                                  TaskResumeSchedule;}\
                                                          }while(0)
/* 定义Log打印接口 */

/* 定义时间基准 */
#define 	T_1MS		(1)
#define 	T_10MS		(10)
#define 	T_100MS		(100)
#define 	T_1S		(1000)
#define 	T_1MM		(T_1S*60)
/* 定义时间基准 */

/* 定义唤醒方式 */
#define WKSRC_ACC_IN            (0x01 << 0)             /*外部ACC信号唤醒*/
#define WKSRC_CAN_IN            (0x01 << 1)             /*CAN总线唤醒*/
/* 定义唤醒方式 */

/* 定义唤醒后行为 */
#define WK_AND_RERUN			(0x01 << 0)				/* 唤醒后继续执行 */
/* 定义唤醒后行为 */

/* 系统复位标志 */
#define SYS_RESET_WHY_LPRS		(0x01 << 0)				/* 低功耗管理复位 */
#define SYS_RESET_WHY_WWDG		(0X01 << 1)				/* 窗口看门狗复位 */
#define SYS_RESET_WHY_FWDG		(0x01 << 2)				/* 独立看门狗复位 */
#define SYS_RESET_WHY_SWRS		(0x01 << 3)				/* 软件复位 */
#define SYS_RESET_WHY_PORR		(0x01 << 4)				/* 电源复位 */
#define SYS_RESET_WHY_EPRS		(0x01 << 5)				/* 外部引脚复位 */
/* 系统复位标志 */

/* 定义串口通道 */
typedef enum
{
  PORT_TO_HOST = 0,
  PORT_TO_EXT,
  PORT_TO_DEBUG,
}enumUartPort;
/* 定义串口通道 */

/* 定义串口通道对应的接口 */
#define USART_HOST              (USART1)
#define USART_EXT               (USART3)
#define USART_DEBUG             (USART2)
/* 定义串口通道对应的接口 */

/* 定义串口通讯数据帧 */
#define FRAME_TYPE_ACK          (0x01)
#define FRAME_TYPE_DATA         (0x02)

typedef enum
{
	BOOT_PWR_ON = 0,
	BOOT_WAKEUP_ON,
	BOOT_UPDATE_ON,
}enumBootState;

/*定义CAN波特率*/
#if defined(GD32F103xB)
typedef enum
{
	CAN_BAUDRATE_1MHZ = 3,
	CAN_BAUDRATE_500KHZ = 6,
	CAN_BAUDRATE_250KHZ = 12,
	CAN_BAUDRATE_125KHZ = 24,
	CAN_BAUDRATE_100KHZ = 30,
}enumCanBaudrate;
#elif defined(STM32F103C8)
typedef enum
{
	CAN_BAUDRATE_1MHZ = 2,
	CAN_BAUDRATE_500KHZ = 4,
	CAN_BAUDRATE_250KHZ = 8,
	CAN_BAUDRATE_125KHZ = 16,
	CAN_BAUDRATE_100KHZ = 20,
}enumCanBaudrate;
#endif
/*定义CAN波特率*/

/* 定义串口通讯数据帧 */
typedef struct
{
	/* 帧的类型（数据/应答） */
	unsigned char frameType;

	/* 数据长度 */
	unsigned char length;

	/* PDU */
	unsigned char data[];
}TxToHostFrameTypedef;
/* 定义串口通讯数据帧 */

/* 定义全局接口 */
typedef struct
{
    /* 车型初始化接口 */
  void (*McuInitProc)(void *);

  /* 车型Deinit接口 */
  void (*McuDeinitProc)(void *);

  /* 车型基本轮询处理任务 */
  void (*McuBasicTask)(void *);
  
  /* Host数据解包任务 */
  int (*HostRxDecode)(unsigned char *, unsigned short, void *);

  /* Host数据解包后的处理任务，由HostRxDecode回调接口 */
  void (*RxDecodeCallBackDef)(unsigned char *, unsigned short, void *);

  /* 接收到Host的应答后的回调接口 */
  void (*GotAckFromHost)(int);

  /* Host通信建立连接 */
  void (*ConnectEstablish)(int);

  /* 当前任务睡眠接口 */
  void (*Sleep)(int);

  /*External malloc interface*/
  void *(*Malloc)(unsigned int size);

  /*External Free interface*/
  void (*Free)(void *pv);

  /* 系统控制接口 */
  struct
  {
    /*系统休眠控制接口 */
    void (*Hibernate)(int, int, void*);

    /* 获取系统复位原因 */
	uint8 (*GetSysResetWhy)(void);
  }SystemControl;

  /* 底层接口 */
  struct
  {
    /* 外设Deinit接口 */
    void (*PeripDeinit)(void);

    /* 数据到Hsot接口 */
    void (*transmitToHost)(unsigned char*, unsigned short, unsigned short);
  }lowInterface;

  /* 系统版本信息 */
  struct
  {
    /* 版本号字符串 */
    unsigned char ver[32];

    /* 版本号长度  */
    unsigned char verSize;
  }Version;

}baseInfoTypedef;
/* 定义全局接口 */


/* 暴露外部接口 */
void FlyTickIncrease(void);
void InterfaceInit(void);
void FlyGoToUpdate(uint8 arg);
enumBootState GetBootState(void);
uint32_t FlyReadUserTimer(uint32_t *Timer);
void FLySystemErrReset(void);
void FlyResetUserTimer(uint32_t *Timer);
void FlyUartTransmitOne(enumUartPort port, uint8 data);
void FlyUartTransmit(enumUartPort port, uint8 *pdata, uint16 size);
int FLyGetSofVersionStr(unsigned char *pOut);
void FlySystemEnterSleep(int wkSrc, int method, void *pBaseInfo);

void LedToggle(void);
void LedBlink(void);
void UserPeripheralsDeinit(void);
void UserIrqStart(void);
void HanldeReceiveData(uint8_t* Buf, uint32_t Len);

/* 暴露外部接口 */


#endif
