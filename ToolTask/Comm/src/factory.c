#include <stdio.h>
#include <string.h>

#include "factory.h"
#include "interface.h"

/**
  ******************************************************************************
  * @file    factory.c
  * @author  hq
  * @brief   工厂调试模块
  ==============================================================================+


**/
extern void FactoryIoControl(uint8, uint8);
extern BOOL HcanTransmit(CanTxMsgTypeDef *data);

typedef struct
{
	uint8 comRxState;

}tFactoryInfoTypedef;

typedef struct
{
	uint8	FrameLen;
	uint8	FrameBuf[32];
	uint8	FrameCheckSum;
	uint8	FrameLenMax;
	uint8	FramStatus;
}structHostCommInfo;


structHostCommInfo	tHostCommInfo;
BOOL bIsFactoryState = FALSE;

/*****************************************************************************
**Name: 		EnterFactoryState
**Function:		进入工厂模式
**Args:
**Return:
******************************************************************************/
void EnterFactoryState(void)
{
	bIsFactoryState = TRUE;

	if(bIsFactoryState == TRUE)
	{
		FlyCanFilterAllId();	//接收所有CAN_ID
	}

	FlyDebugPrint(DEBUG_MSG01, "Enter factory mode");
}
/*****************************************************************************
**Name: 		ExitFactoryState
**Function:		退出工厂模式
**Args:
**Return:
******************************************************************************/
void ExitFactoryState(void)
{
	bIsFactoryState = FALSE;

	FlyDebugPrint(DEBUG_MSG01, "Exit factory mode");

	HAL_NVIC_SystemReset();
}
/*****************************************************************************
**Name: 		FactoryGetState
**Function:		获取工厂模式状态
**Args:
**Return:
******************************************************************************/
BOOL FactoryGetState(void)
{
	return bIsFactoryState;
}
/*****************************************************************************
**Name: 		FactoryTransmit
**Function:
**Args:
**Return:
******************************************************************************/
void FactoryTransmit(uint8 *arg, uint16 size)
{
	if(bIsFactoryState)
	{
		uint16 count;
		uint8 checksum;

		uint8 sendBuf[3] = {0xff, 0x55, size + 1};

		checksum = size + 1;

		for(count = 0; count < size; count++)
		{
			checksum += arg[count];
		}

		FlyUartTransmit(PORT_TO_HOST, sendBuf, 3);
		FlyUartTransmit(PORT_TO_HOST, arg, size);
		FlyUartTransmit(PORT_TO_HOST, &checksum, 1);
	}
}
/*****************************************************************************
**Name:		 	FactorySendHearBeatToHost
**Function:
**Args:
**Return:
******************************************************************************/
void FactorySendHeartBeatToHost(void)
{
	/*心跳*/
	static uint8 sendBuf[] = {0x22, 0x00};
	static uint32_t timer;

	if(FlyReadUserTimer(&timer) >= T_100MS * 5)
	{
		FactoryTransmit(sendBuf, sizeof(sendBuf));

		FlyResetUserTimer(&timer);
	}
}
/*****************************************************************************
**Name:		 	FactoryReturnVersionToHost
**Function:		发送版本号
**Args:
**Return:
******************************************************************************/
void FactoryReturnVersionToHost(void)
{
	uint8 sendBuf[64] = {0x00, 0x01};
	uint8 size = 2;

	size += FLyGetSofVersionStr(sendBuf + 2);

	FactoryTransmit(sendBuf, size);
}
/*****************************************************************************
**Name:		 	FactoryTaskProc
**Function:
**Args:
**Return:
******************************************************************************/
void FactoryTaskProc(void)
{
	/*发送心跳*/
//	FactorySendHeartBeatToHost();
	FeedDog();
}
/*****************************************************************************
**Name:		 	DealHostCmd
**Function:
**Args:
**Return:
******************************************************************************/
void FactoryDealHostCmd(uint8 *pdata, uint16_t len)
{
	switch(pdata[0])
	{
		/*Return version*/
		case 0x00:
			{
				if(bIsFactoryState != TRUE)return;

				switch(pdata[1])
				{
					/*return version*/
					case 0x01:
					{
						FactoryReturnVersionToHost();
					}
					break;
				}
			}
			break;

		/*IO控制*/
		case 0x02:
			{
				if(bIsFactoryState != TRUE)return;

				FactoryIoControl(pdata[1], pdata[2]);
			}
			break;

		/*Enter or exit factory mode*/
		case 0xff:
			{
				uint8 sendBuf[] = {0xff, 0x22, 0x00};
//				FactoryTransmit(sendBuf, sizeof(sendBuf));

				/*Enter factory mode*/
				if((pdata[1] == 0x22) && (pdata[2] == 0x33))
				{
					EnterFactoryState();
					FactoryTransmit(sendBuf, sizeof(sendBuf));
				}
				else if((pdata[1] == 0x22) && (pdata[2] == 0x44))	/*Exit factory mode*/
				{
					FactoryTransmit(sendBuf, sizeof(sendBuf));
					ExitFactoryState();
				}
			}
			break;

		/*Update related protocol*/
		case 0x37:
			{
				if(bIsFactoryState != TRUE)return;

				/*Enter bootloader for update*/
				if((pdata[1] == 0xfa))
				{
					extern void UserPeripheralsDeinit(void);
					uint8 arg = pdata[2];
					uint8 ack[] = {0xfa, 0xff};

					/*波特率参数*/
					/*
						1:115200
						2:57600
						3:56000
						4:38400
						5:19200
						6:14400
					*/
					if((arg == 0) || (arg > 6))
					{
						ack[1] = 0xff;
						FactoryTransmit(ack, sizeof(ack));

						return;
					}
					ack[1] = 0x00;
					FactoryTransmit(ack, sizeof(ack));

					HAL_RCC_DeInit();
					UserPeripheralsDeinit();
					FlyGoToUpdate(arg);
				}
			}
			break;

		default:
			break;
	}
}
/*****************************************************************************
**Name:		 	FactoryComRxProc
**Function:
**Args:
**Return:
******************************************************************************/
void FactoryComRxProc(uint8 * data)
{
	switch(tHostCommInfo.FramStatus)
	{
		case 0:
				if(0xff == *data)
				{
					tHostCommInfo.FramStatus = 1;
				}
				break;

		case 1:
				if(0x55 == *data)
				{
					tHostCommInfo.FramStatus = 2;
					memset(&tHostCommInfo.FrameBuf, 0x00, sizeof(tHostCommInfo.FrameBuf));
				}
				else
				{
					tHostCommInfo.FramStatus = 0;
				}
				break;

		case 2:
				if(0xff != *data)
				{
					tHostCommInfo.FrameCheckSum = tHostCommInfo.FrameLenMax = *data;
					tHostCommInfo.FrameLen = 0;
					tHostCommInfo.FramStatus = 3;
				}
				else
				{
					tHostCommInfo.FramStatus = 0;
				}
				break;

		case 3:
				if(tHostCommInfo.FrameLen < tHostCommInfo.FrameLenMax - 1)
				{
					tHostCommInfo.FrameBuf[tHostCommInfo.FrameLen] = *data;
					tHostCommInfo.FrameCheckSum += *data;
					tHostCommInfo.FrameLen++;
				}
				else
				{
					if(*data == tHostCommInfo.FrameCheckSum)
					{
						tHostCommInfo.FrameBuf[tHostCommInfo.FrameLen] = 0;
						tHostCommInfo.FrameBuf[tHostCommInfo.FrameLen + 1] = 0;

						/* Deal command at here */
						FactoryDealHostCmd(tHostCommInfo.FrameBuf, tHostCommInfo.FrameLen);
					}
					else
					{
					}
					tHostCommInfo.FramStatus = 0;
					tHostCommInfo.FrameLen = 0;
					tHostCommInfo.FrameCheckSum = 0;
					tHostCommInfo.FrameLenMax = 0;
				}
				break;

		default:
				tHostCommInfo.FramStatus = 0;
				break;
	}
}

/*****************************************************************************
**Name:		 	FactoryCanDecode
**Function:
**Args:
**Return:
******************************************************************************/
void FactoryCanDecode(CanRxMsgTypeDef *pCAN_RxData)
{
	if(pCAN_RxData != NULL)
	{
		CanTxMsgTypeDef canTxBuf =
		{
			.StdId = pCAN_RxData->StdId,
			.ExtId = pCAN_RxData->ExtId,
			.IDE = pCAN_RxData->IDE,
			.RTR = CAN_RTR_DATA,
			.DLC = pCAN_RxData->DLC,
		};

		memcpy(canTxBuf.Data, pCAN_RxData->Data, 8);

		HcanTransmit(&canTxBuf);
	}
}

