/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "slaveTool.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static BOOL rebootFlag = FALSE;
static uint32_t rebootTime;

static BOOL heartFlag = FALSE;
static uint32_t PingSendTimer;

extern IWDG_HandleTypeDef hiwdg;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
extern BaseType_t UartTransmitDataToHost(uint8_t * Buf, uint16_t Len);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
void ResetUserTimer(uint32_t *Timer)
{
	*Timer = HAL_GetTick();
}
/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
uint32_t ReadUserTimer(uint32_t *Timer)
{
	uint32_t tmp = HAL_GetTick();
	return (*Timer <= tmp) ? (tmp - *Timer) : (0xFFFFFFFF - tmp + *Timer);
}
/*****************************************************************************
**Name:		 	Delay1ms
**Function:
**Args:
**Return:
******************************************************************************/
void Delay1ms(volatile uint32_t nTime)
{
	HAL_Delay(nTime);
}

/*****************************************************************************
**Name:		 	MX_GPIO_Write
**Function:
**Args:
**Return:
******************************************************************************/
void MX_GPIO_Write(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
}
void MX_GPIO_Toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
}

/*****************************************************************************
**Name:		 	MX_IWDG_Refresh
**Function:	喂狗
**Args:
**Return:
******************************************************************************/
void MX_IWDG_Refresh(void)
{
	HAL_IWDG_Refresh(&hiwdg);
}
/*****************************************************************************
**Name:		 	SendHeartBeatToHost
**Function: 发送心跳函数
**Args:
**Return:
******************************************************************************/
void SendHeartBeatToHost(void)
{
	/*心跳*/
	uint8 sendBuf[] = {0x01, 0x01, 0xFF};
	static uint32_t sendTimer;

	if(ReadUserTimer(&sendTimer) >= T_100MS * 10)
	{
		UartTransmitDataToHost(sendBuf, sizeof(sendBuf));

		ResetUserTimer(&sendTimer);
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
	if(heartFlag == FALSE)
	{
		ResetUserTimer(&PingSendTimer);
	}
	else
	{
		if((heartFlag == TRUE) && (ReadUserTimer(&PingSendTimer) >= T_1S * 15))
		{
			heartFlag = FALSE;
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
		ResetUserTimer(&rebootSystemTimer);
	}
	else
	{
		if(ReadUserTimer(&rebootSystemTimer) >= rebootTime)
		{
			HAL_NVIC_SystemReset();
			ResetUserTimer(&rebootSystemTimer);
		}
	}
}

/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void SetRelayPro(uint8_t line, uint8_t column)
{
}

/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void SetRemotePro(uint8_t channel)
{
}

/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void SetVolCalibValue(uint32_t value)
{
}

/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void GetRelayState(void)
{
}

/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void GetRemoteCHState(void)
{
}


/*****************************************************************************
**Name:		 	
**Function:	 	
**Args:
**Return:
******************************************************************************/
void GetRemoteValue(void)
{
}


/**
  * @brief  解析host协议.
  * @param	Buf 一帧数据中数据部分的首地址.
  * @param	Len 该数据帧包含的数据长度.
  * @retval Null.
  */
void HostCmdProcess(uint8_t *Buf, uint16_t Len)
{
	uint8_t *Cmd;
	Cmd = Buf;
	
	dbgprintf("Process CMD %#02X\n", Cmd[0]);
	
	switch(Cmd[0])
	{
		case 0x00:
		{
			if((Cmd[1] == 0x01) && (Cmd[2] == 0x01))
			{
				/*Got ping*/
				uint8_t ack[] = {0x01, 0x01, 0x00};
				dbgprintf("Got ping...\n");
				UartTransmitDataToHost(ack, sizeof(ack));
			}
		}
		break;

		case 0x01:
		{
			if(Cmd[1] == 0xFF)
			{
				/*System reset*/
				uint8_t ack[] = {0x01, 0xFF, 0x00};

				dbgprintf("System reset...\n");
				UartTransmitDataToHost(ack, sizeof(ack));

				rebootFlag = TRUE;
				rebootTime = (Cmd[2] << 8) | Cmd[3];
			}
		}
		break;

		case 0x0A:
		{
			//设置方控的导通
			if(Cmd[1] == 0x01)
			{
				SetRelayPro(Cmd[2], Cmd[3]);
			}

			//选择AD输出通道
			if(Cmd[1] == 0x0D)
			{
				SetRemotePro(Cmd[2]);
			}

			//设置AD的校准值
			if(Cmd[1] == 0xAD)
			{
				uint32_t value;
				value = (Cmd[2] << 8) | Cmd[3];
				SetVolCalibValue(value);
			}
		}
		break;

		case 0x0B:
		{
			//获取当前的导通状态
			if(Cmd[1] == 0x02)
			{
				GetRelayState();
			}

			//获取当前的通道配置状态
			if(Cmd[1] == 0x03)
			{
				GetRemoteCHState();
			}

			//获取当前输出的电压值
			if(Cmd[1] == 0x04)
			{
				GetRemoteValue();
			}
		}
		break;

		case 0xAC:
		{
			//上位机心跳应答
			if((Cmd[1] == 0x01) && (Cmd[2] == 0x01) && (Cmd[3] == 0xFF))
			{
				heartFlag = TRUE;
				ResetUserTimer(&PingSendTimer);
			}
		}
		break;

		default:
		break;
	}
}
	
/**
  * @brief  基本任务.
  * @retval Null.
  */
void McuBasicTaskProc(void)
{
	/*重启设备*/
	RebootDevice();

	/*检测心跳是否丢失*/
	IsHeartBroken();
	
	/*发送心跳函数*/
	SendHeartBeatToHost();
}

///**
//  * @brief  配置逆初始化,系统调用.
//  * @retval Null.
//  */
//void McuDeInit(void)
//{
//}
//
///**
//  * @brief  初始化,系统调用.
//  * @retval Null.
//  */
//void McuInit(void)
//{
//}

/* USER CODE END 0 */

