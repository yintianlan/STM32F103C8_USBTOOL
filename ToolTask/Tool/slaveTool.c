/*****************************************************************************
1.(主Node Task)

******************************************************************************/
#include "slaveTool.h"


static BOOL rebootFlag = FALSE;
static uint32_t rebootTime;

static BOOL heartFlag = FALSE;
static uint32_t PingSendTimer;

/*****************************************************************************
**Name: 		NodeTransmit
**Function:	数据上行通道,结点只通过CAN与网关通讯，不用串口
**Args:
**Return:
******************************************************************************/
void UsbTransmit(uint8 *arg, uint32_t size,  void * const pBaseInfo)
{

}

/*****************************************************************************
**Name:		 	HeartBeatCheck
**Function:	心跳应答函数
**Args:
**Return:
******************************************************************************/
void HeartBeatCheck(unsigned char * const pCAN_RxData, void * const pBaseInfo)
{
	if ((pCAN_RxData[1] == 0x01) && (pCAN_RxData[2] == 0x01) )
	{
		/* code */
		heartFlag = TRUE;
		uint8 sendBuf[] = {0xAC, 0x10, 0x01, 0x01};
		UsbTransmit(sendBuf, sizeof(sendBuf), pBaseInfo);
		FlyResetUserTimer(&PingSendTimer);
	}
	else
	{
		/* code */
	}
}

/*****************************************************************************
**Name:		 	IsHeartBroken
**Function:	检测心跳是否丢失，如果心跳超时，则重新请求
**Args:
**Return:
******************************************************************************/
void IsHeartBroken(void)
{
	if(0)
	{
		FlyResetUserTimer(&PingSendTimer);
	}
	else
	{
		if((heartFlag == TRUE) && (FlyReadUserTimer(&PingSendTimer) >= T_1S * 20))
		{
			heartFlag = FALSE;
			FlyResetUserTimer(&PingSendTimer);
		}
	}
}


/*****************************************************************************
**Name:		 	RebootDevice
**Function:	此函数需放在base任务中运行
**Args:
**Return:
******************************************************************************/
void RebootDevice()
{
	static uint32_t rebootSystemTimer;

	if(rebootFlag == FALSE)
	{
		FlyResetUserTimer(&rebootSystemTimer);
	}
	else
	{
		if(FlyReadUserTimer(&rebootSystemTimer) >= rebootTime)
		{
			HAL_NVIC_SystemReset();
			FlyResetUserTimer(&rebootSystemTimer);
		}
	}
}

/**
  * @brief  从数据流中解析出数据帧并调用解析回调函数;
  			协议帧:0xff--0x55--length--datatype--data--checksum.
  * @param  pData 待解析的数据流的首地址.
  * @param  length 数据流的长度.
  * @param  baseInfo 基本接口信息，包含回调函数.
  * @retval 解析的结果，0 未解析完成; 0xf0 校验计算错误; 0xf0 获取到了有效数据.
  */
int HostRxDecode(unsigned char * const pData, unsigned short length, void * const pBaseInfo)
{

	/*Default return*/
	return 0x00;
}


/**
  * @brief  解析host协议.
  * @param	pData 一帧数据中数据部分的首地址.
  * @param	length 该数据帧包含的数据长度.
  * @param	pBaseInfo 全局基本信息.
  * @retval Null.
  */
void HostCmdProc(unsigned char * const pData, unsigned short length, void * const pBaseInfo)
{
	switch(pData[0])
	{
		case 0x00:
			break;

		default:
			{
			}
			break;
	}

	/*Return ack to host; Ack = 0xAC + original protocol */
	if(pData[0] != 0xAC)
	{
		uint16 count;
		uint8 checksum;
		uint8 sendBuf[4] = {0xff, 0x55, length, 0xAC};

		checksum = length;
		checksum += 0xAC;
		for(count = 0; count < length - 2; count++)
		{
			checksum += pData[count];
		}

		FlyUartTransmit(PORT_TO_HOST, sendBuf, 4);
		FlyUartTransmit(PORT_TO_HOST, pData, length - 2);
		FlyUartTransmit(PORT_TO_HOST, &checksum, 1);
	}
}


/**
  * @brief  基本任务.
  * @param	pBaseInfo 全局基本信息.
  * @retval Null.
  */
void McuBasicTaskProc(void * const pBaseInfo)
{
	/*重启设备*/
	RebootDevice();

	/*检测心跳是否丢失*/
	IsHeartBroken();
}

/**
  * @brief  配置逆初始化,系统调用.
  * @retval Null.
  */
void McuDeInit(void * const pBaseInfo)
{

}


/**
  * @brief  车辆相关初始化,系统调用.
  * @param	pBaseInfo 全局基本信息.
  * @retval Null.
  */
void McuInit(void * const pBaseInfo)
{
	

	/*Init bus*/

	/*Set Filter*/

}

