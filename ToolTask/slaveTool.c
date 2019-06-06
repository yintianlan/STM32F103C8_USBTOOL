/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "slaveTool.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  RELAYState_ON = 0,     // (公共端——常闭)
  RELAYState_OFF,        // (公共端——常开)  
}RELAYState_TypeDef;

typedef struct
{
	uint8 RemoteChooseState;
	uint8 LineState;
	uint8 ColumnState;
}structRelayState;

structRelayState tRelayState;

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
void GetRelayState(void);

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

/**
  * 函数功能: 设置继电器模块的行状态
  * 输入参数：无
  * 返 回 值: 无
  * 说    明： 无
  */
void RELAYx_StateSet(uint8 Line, uint8 state)
{
	GPIO_PinState pingSet;
	if(state == 0x01)
	{
		pingSet = GPIO_PIN_SET;
	}
	else
	{
		pingSet = GPIO_PIN_RESET;
	}

	Line = Line + 1;
	switch(Line)
	{
		case 0x01:
			HAL_GPIO_WritePin(RELAY_H1_GPIO_Port, RELAY_H1_Pin, pingSet);
			break;

		case 0x02:
			HAL_GPIO_WritePin(RELAY_H2_GPIO_Port, RELAY_H2_Pin, pingSet);
			break;

		case 0x03:
			HAL_GPIO_WritePin(RELAY_H3_GPIO_Port, RELAY_H3_Pin, pingSet);
			break;

		case 0x04:
			HAL_GPIO_WritePin(RELAY_H4_GPIO_Port, RELAY_H4_Pin, pingSet);
			break;

		case 0x05:
			HAL_GPIO_WritePin(RELAY_H5_GPIO_Port, RELAY_H5_Pin, pingSet);
			break;

		case 0x06:
			HAL_GPIO_WritePin(RELAY_H6_GPIO_Port, RELAY_H6_Pin, pingSet);
			break;

		case 0x07:
			HAL_GPIO_WritePin(RELAY_H7_GPIO_Port, RELAY_H7_Pin, pingSet);
			break;

		case 0x08:
			HAL_GPIO_WritePin(RELAY_H8_GPIO_Port, RELAY_H8_Pin, pingSet);
			break;

		default:
			break;
	}
}

/**
  * 函数功能: 设置继电器模块的列状态
  * 输入参数：无
  * 返 回 值: 无
  * 说    明： 无
  */
void RELAYy_StateSet( uint8 Column, uint8 state)
{
	GPIO_PinState pingSet;
	if(state == 0x01)
	{
		pingSet = GPIO_PIN_SET;
	}
	else
	{
		pingSet = GPIO_PIN_RESET;
	}

	Column = Column + 1;
	switch(Column)
	{
		case 0x01:
			HAL_GPIO_WritePin(KC_L1_GPIO_Port, KC_L1_Pin, pingSet);
			break;

		case 0x02:
			HAL_GPIO_WritePin(KC_L2_GPIO_Port, KC_L2_Pin, pingSet);
			break;

		case 0x03:
			HAL_GPIO_WritePin(KC_L3_GPIO_Port, KC_L3_Pin, pingSet);
			break;

		case 0x04:
			HAL_GPIO_WritePin(KC_L4_GPIO_Port, KC_L4_Pin, pingSet);
			break;

		case 0x05:
			HAL_GPIO_WritePin(KC_L5_GPIO_Port, KC_L5_Pin, pingSet);
			break;

		case 0x06:
			HAL_GPIO_WritePin(KC_L6_GPIO_Port, KC_L6_Pin, pingSet);
			break;

		case 0x07:
			HAL_GPIO_WritePin(KC_L7_GPIO_Port, KC_L7_Pin, pingSet);
			break;

		case 0x08:
			HAL_GPIO_WritePin(KC_L8_GPIO_Port, KC_L8_Pin, pingSet);
			break;

		default:
			break;
	}
}

/**
  * 函数功能: 读取继电器的状态
  * 输入参数：无
  * 返 回 值: RELAYState_OFF：继电器(公共端——常开)
  *           RELAYState_ON： 继电器(公共端——常闭)
  * 说    明：对应于低电平有效继电器模块
  */
uint8_t RELAY_GetState(void)
{
	uint8_t relay_state = RELAYState_ON;
	static uint8_t line, column;

	/*读行*/
	HAL_GPIO_ReadPin(RELAY_H1_GPIO_Port, RELAY_H1_Pin) ? SET_BIT(line, 1<<0) : CLEAR_BIT(line, 1<<0);
	HAL_GPIO_ReadPin(RELAY_H2_GPIO_Port, RELAY_H2_Pin) ? SET_BIT(line, 1<<1) : CLEAR_BIT(line, 1<<1);
	HAL_GPIO_ReadPin(RELAY_H3_GPIO_Port, RELAY_H3_Pin) ? SET_BIT(line, 1<<2) : CLEAR_BIT(line, 1<<2);
	HAL_GPIO_ReadPin(RELAY_H4_GPIO_Port, RELAY_H4_Pin) ? SET_BIT(line, 1<<3) : CLEAR_BIT(line, 1<<3);
	HAL_GPIO_ReadPin(RELAY_H5_GPIO_Port, RELAY_H5_Pin) ? SET_BIT(line, 1<<4) : CLEAR_BIT(line, 1<<4);
	HAL_GPIO_ReadPin(RELAY_H6_GPIO_Port, RELAY_H6_Pin) ? SET_BIT(line, 1<<5) : CLEAR_BIT(line, 1<<5);
	HAL_GPIO_ReadPin(RELAY_H7_GPIO_Port, RELAY_H7_Pin) ? SET_BIT(line, 1<<6) : CLEAR_BIT(line, 1<<6);
	HAL_GPIO_ReadPin(RELAY_H8_GPIO_Port, RELAY_H8_Pin) ? SET_BIT(line, 1<<7) : CLEAR_BIT(line, 1<<7);

	/*读列*/
	HAL_GPIO_ReadPin(KC_L1_GPIO_Port, KC_L1_Pin) ? SET_BIT(column, 1<<0) : CLEAR_BIT(column, 1<<0);
	HAL_GPIO_ReadPin(KC_L2_GPIO_Port, KC_L2_Pin) ? SET_BIT(column, 1<<1) : CLEAR_BIT(column, 1<<1);
	HAL_GPIO_ReadPin(KC_L3_GPIO_Port, KC_L3_Pin) ? SET_BIT(column, 1<<2) : CLEAR_BIT(column, 1<<2);
	HAL_GPIO_ReadPin(KC_L4_GPIO_Port, KC_L4_Pin) ? SET_BIT(column, 1<<3) : CLEAR_BIT(column, 1<<3);
	HAL_GPIO_ReadPin(KC_L5_GPIO_Port, KC_L5_Pin) ? SET_BIT(column, 1<<4) : CLEAR_BIT(column, 1<<4);
	HAL_GPIO_ReadPin(KC_L6_GPIO_Port, KC_L6_Pin) ? SET_BIT(column, 1<<5) : CLEAR_BIT(column, 1<<5);
	HAL_GPIO_ReadPin(KC_L7_GPIO_Port, KC_L7_Pin) ? SET_BIT(column, 1<<6) : CLEAR_BIT(column, 1<<6);
	HAL_GPIO_ReadPin(KC_L8_GPIO_Port, KC_L8_Pin) ? SET_BIT(column, 1<<7) : CLEAR_BIT(column, 1<<7);

	
	if((line != 0) || (column != 0))
	{
		tRelayState.LineState = line;
		tRelayState.ColumnState = column;
		relay_state = RELAYState_ON;
	}
	else
	{
		tRelayState.LineState = 0;
		tRelayState.ColumnState = 0;
		relay_state = RELAYState_OFF;
	}

	return relay_state;
}


/*****************************************************************************
**Name:		 	SetRelayPro
**Function:	 	设置继电器导通
**Args:	line:每一bit代表方控的行
			column:每一bit代表方控的列
**Return:
******************************************************************************/
void SetRelayPro(uint8_t sLine, uint8_t sColumn)
{
	uint8 result; 

	for(int i = 0; i < 8; i++)
	{
		RELAYx_StateSet( i, sLine & (1<<i));
	}

	for(int j = 0; j < 8; j++)
	{
		RELAYy_StateSet(j, sColumn & (1<<j));
	}

	RELAY_GetState();//读取继电器的状态

	if((sLine == tRelayState.LineState) && (sColumn == tRelayState.ColumnState))
	{
		result = PASS;
	}
	else
	{
		result = FAIL;
	}
	
	uint8_t ack[] = {0x0A, 0x01, result};
	UartTransmitDataToHost(ack, sizeof(ack));
}

/*****************************************************************************
**Name:		 	SetRemotePro
**Function:	 	设置输出通道1-2
**Args:
**Return:
******************************************************************************/
void SetRemotePro(uint8_t channel)
{
	uint8 result; 
	if(channel == 0x01)
	{
		tRelayState.RemoteChooseState = REMOTE1;
		MX_GPIO_Write(REMOTE_CHOOSE_GPIO_Port, REMOTE_CHOOSE_Pin, GPIO_PIN_SET);
		result = HAL_GPIO_ReadPin(REMOTE_CHOOSE_GPIO_Port, REMOTE_CHOOSE_Pin) == GPIO_PIN_SET ? PASS : FAIL;
	}
	else
	{
		tRelayState.RemoteChooseState = REMOTE2;
		MX_GPIO_Write(REMOTE_CHOOSE_GPIO_Port, REMOTE_CHOOSE_Pin, GPIO_PIN_RESET);
		result = HAL_GPIO_ReadPin(REMOTE_CHOOSE_GPIO_Port, REMOTE_CHOOSE_Pin) == GPIO_PIN_RESET ? PASS : FAIL;
	}

	uint8_t ack[] = {0x0A, 0x0D, result};
	UartTransmitDataToHost(ack, sizeof(ack));
}

/*****************************************************************************
**Name:		 	SetVolCalibValue
**Function:	 	发送主控板上AD通道管脚的默认电压值
**Args:
**Return:
******************************************************************************/
void SetVolCalibValue(uint32_t value)
{
	uint8 result; 
	if((value != 0) && (value <= 3300))
	{
		tpDataInfo->SetVolValue = value;
		result = PASS;
	}
	else
	{
		result = FAIL;
	}

	uint8_t ack[] = {0x0A, 0xAD, result};
	UartTransmitDataToHost(ack, sizeof(ack));
}

/*****************************************************************************
**Name:		 	GetRelayState
**Function:	 	获取继电器行列导通状态
**Args:
**Return:
******************************************************************************/
void GetRelayState(void)
{
	RELAY_GetState();

	uint8_t ack[] = {0x0B, 0x02, tRelayState.LineState, tRelayState.ColumnState};
	UartTransmitDataToHost(ack, sizeof(ack));
}

/*****************************************************************************
**Name:		 	GetRemoteCHState
**Function:	 	请求当前AD配置通道
**Args:
**Return:
******************************************************************************/
void GetRemoteCHState(void)
{
	uint8 ChannalNum;

	ChannalNum = tRelayState.RemoteChooseState;

	uint8_t ack[] = {0x0B, 0x03, ChannalNum};
	UartTransmitDataToHost(ack, sizeof(ack));
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
				dbgprintf("Set relay io...\n");
				SetRelayPro(Cmd[2], Cmd[3]);
			}

			//选择AD输出通道
			if(Cmd[1] == 0x0D)
			{
				dbgprintf("Set adc channel...\n");
				SetRemotePro(Cmd[2]);
			}

			//设置AD的校准值
			if(Cmd[1] == 0xAD)
			{
				uint32_t value;
				value = (Cmd[2] << 8) | Cmd[3];
				dbgprintf("Set ping vdda...\n");
				SetVolCalibValue(value);
			}
		}
		break;

		case 0x0B:
		{
			//获取当前的导通状态
			if(Cmd[1] == 0x02)
			{
				dbgprintf("Got relay io...\n");
				GetRelayState();
			}

			//获取当前的通道配置状态
			if(Cmd[1] == 0x03)
			{
				dbgprintf("Got adc channel...\n");
				GetRemoteCHState();
			}

			//获取当前输出的电压值
			if(Cmd[1] == 0x04)
			{
				dbgprintf("Got adc value...\n");
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

/**
  * @brief  初始化,系统调用.
  * @retval Null.
  */
void McuInit(void)
{
	if(tpDataInfo->SetVolValue == 0x0000)
	{
		tpDataInfo->SetVolValue = 3300;
	}

	tpDataInfo->RemoteCh1[0] = REMOTE1;
	tpDataInfo->RemoteCh2[0] = REMOTE2;

	//开始方控校准
	AdcRemoteStartCalibrate();

	//clear the reset flags
	__HAL_RCC_CLEAR_RESET_FLAGS();
}

/* USER CODE END 0 */

