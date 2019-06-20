#define	_checkStatus_GLOBAL_
#include "slaveTool.h"

/* Private variables ---------------------------------------------------------*/
AdcValueTypedef  tAdcValue;
static uint32_t PowerOnTimer;
extern BaseType_t UartTransmitDataToHost(uint8_t * Buf, uint16_t Len);

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* Voltage define */
#define V10mVolt				(10)
#define V100mVolt				(100)
#define	V1Volt					(V100mVolt * 10)

/* Send Remote key to host */
#define SEND_REMOTE_KEY_TO_HOST(Ch, Val)		do{uint8 buf[5] = {0x00, 0xAD, (u8)Ch, (u8)(Val >> 8), (u8)Val};\
													UartTransmitDataToHost(buf,sizeof(buf));}while(0)

/* Send battery voltage */
#define SEND_BAT_VOLTAGE_TO_HOST 	do{uint8 buf[4] = {0x00,0x02,(BatteryVoltage >> 8)&0xff, BatteryVoltage&0xff};\
													UartTransmitDataToHost(buf,4);}while(0)


#define REMOTE_KEY_DELAY_TIME_PRESS			(T_1MS * 10)
#define REMOTE_KEY_DELAY_TIME_RELEASE		(T_1MS * 10)
#define REMOTE_KEY_THRESHOLD_VAL			(V10mVolt * 5)
#define REMOTE_KEY_CH_AD_TMP_SIZE			(10)
#define REMOTE_SIGNAL_PERIOD_TIME			(T_1MS * 1)
#define REMOTE_KEY_NO_KEY		        (V100mVolt * 33)	//方控释放值

typedef struct
{
	uint8	 chId;
	uint8    chState;
	uint32_t valueNow;			//当前计算的值
	uint32_t valueBuf[REMOTE_KEY_CH_AD_TMP_SIZE];	//多次采用容器
	uint32_t signalPeriod;		//单次采样时间
	uint32_t delayTimer;		//消抖计时器
	uint32_t thresholdTime;		//消抖延时
	uint32_t thresholdVal;		//跳动阈值电压
	uint32_t defaultVoltage;	//无按键按下时的电压
}RemoteKeyChannelTypedef;

typedef enum
{
	AD_SYNC_INITIALIZE = 0,
	AD_SYNC_CALIBRATION,
	AD_SYNC_WORKING,
}AdSyncStepDef;


/*1路按键通道*/
RemoteKeyChannelTypedef tRemoteKeyCh = 
{
	.chId = 0,
	.chState = 0,
	.delayTimer = REMOTE_SIGNAL_PERIOD_TIME,
	.thresholdTime = REMOTE_KEY_DELAY_TIME_RELEASE,
	.thresholdVal = REMOTE_KEY_THRESHOLD_VAL,
	.valueNow = V100mVolt * 33,
	.defaultVoltage = V100mVolt * 33,
};

//RemoteKeyChannelTypedef tRemoteKeyCh1 = 
//{
//	.chId = 0,
//	.chState = 0,
//	.delayTimer = REMOTE_SIGNAL_PERIOD_TIME,
//	.thresholdTime = REMOTE_KEY_DELAY_TIME_RELEASE,
//	.thresholdVal = REMOTE_KEY_THRESHOLD_VAL,
//	.valueNow = V100mVolt * 33,
//	.defaultVoltage = V100mVolt * 33,
//};
//RemoteKeyChannelTypedef tRemoteKeyCh2 = 
//{
//	.chId = 1,
//	.chState = 0,
//	.delayTimer = REMOTE_SIGNAL_PERIOD_TIME,
//	.thresholdTime = REMOTE_KEY_DELAY_TIME_RELEASE,
//	.thresholdVal = REMOTE_KEY_THRESHOLD_VAL,	
//	.valueNow = V100mVolt * 33,
//	.defaultVoltage = V100mVolt * 33,
//};
//

static AdSyncStepDef adRemoteSyncStep = AD_SYNC_INITIALIZE;

uint16_t		BatteryVoltage;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/


/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
/*****************************************************************************
**Name:		 	AdcRemoteStartCalibrate
**Function:	 	开始方控校准
**Args:
**Return:
******************************************************************************/
void AdcRemoteStartCalibrate(void)
{
	adRemoteSyncStep = AD_SYNC_INITIALIZE;
	ResetUserTimer(&PowerOnTimer);
}

/*****************************************************************************
**Name:		 	GetAdcValue
**Function:
**Args:
**Return:
******************************************************************************/
uint32_t GetAdcValue(AdcChannelTypedef channel)
{
	if(channel > MAX_CH)
	{
		return 0;
	}
	return tAdcValue.Value[channel];
}

/*****************************************************************************
**Name:		 	获取remote电压值
**Function:
**Args:
**Return:
******************************************************************************/
uint32_t GetChannelVol(uint8 chId)
{
	uint32_t vddaValue;

	/* 获取VDDA实际的电压电平 */
	vddaValue = tpDataInfo->SetVolValue;

	if( vddaValue <= 0 && vddaValue >=3600)
	{
		vddaValue = 3300;
	}

	/* 方控通道AD值 */
	return GetAdcValue((AdcChannelTypedef)chId) * vddaValue/ 4095;;
}

/*****************************************************************************
**Name:		 	CheckBatteryVol
**Function:	实时检测电池电压
**Args:
**Return:
******************************************************************************/
void CheckBatteryVol(void)
{
	static uint32_t AutoSendTimer;

	BatteryVoltage = GetChannelVol(tRemoteKeyCh.chId);

	if(ReadUserTimer(&AutoSendTimer) > T_1S * 1)
	{
		SEND_BAT_VOLTAGE_TO_HOST; 
		ResetUserTimer(&AutoSendTimer);
	}
}

/*****************************************************************************
**Name:		 	GetRemoteChannelStableVol
**Function:	 	获取当前通道的稳定值
*说明：采样8组数据，
**Args:
**Return:
******************************************************************************/
void GetRemoteChannelStableVol(RemoteKeyChannelTypedef *remoteChx)
{
	uint8 signalIndex = 0;
	uint32_t checkTimer;

	ResetUserTimer(&checkTimer);

	if ( REMOTE_KEY_CH_AD_TMP_SIZE < 1)
	{
		remoteChx->valueNow = 0;
		return;
	}

	/*获取连续的采样值*/
	while(signalIndex < REMOTE_KEY_CH_AD_TMP_SIZE)
	{
		if(ReadUserTimer(&checkTimer) >= remoteChx->signalPeriod)
		{
			remoteChx->valueBuf[signalIndex++] = GetChannelVol(remoteChx->chId);
			ResetUserTimer(&checkTimer);
		}
	}

	if ( REMOTE_KEY_CH_AD_TMP_SIZE <= 2)
	{
		remoteChx->valueNow = remoteChx->valueBuf[0];
		return;
	}

	/*从大到小排序*/
	{
		uint8 nextIndex = 0;
		uint8 endIndex = REMOTE_KEY_CH_AD_TMP_SIZE - 1;
		uint32_t tmp;
		uint8 condition = 1;

		while(condition)
		{
			for(signalIndex = 0; signalIndex < REMOTE_KEY_CH_AD_TMP_SIZE; signalIndex++)
			{
				nextIndex = signalIndex + 1;
				
				if(nextIndex <= endIndex)
				{
					if(remoteChx->valueBuf[signalIndex] < remoteChx->valueBuf[nextIndex])
					{
						tmp = remoteChx->valueBuf[signalIndex];
						remoteChx->valueBuf[signalIndex] = remoteChx->valueBuf[nextIndex];
						remoteChx->valueBuf[nextIndex] = tmp;
					}
				}
				else
				{
					endIndex -= 1;

					/*Break outer loop*/
					if(endIndex == 0)
					{
						condition = 0;
					}
					
					break;
				}
			}
		}
	}

	/*去头去尾取平均*/
	remoteChx->valueNow = 0;
	for(signalIndex = 1; signalIndex < REMOTE_KEY_CH_AD_TMP_SIZE - 1; signalIndex++)
	{
		remoteChx->valueNow += (remoteChx->valueBuf[signalIndex] * 100)/(REMOTE_KEY_CH_AD_TMP_SIZE - 2);
	}

	remoteChx->valueNow /= 100;

	return;
}

/*****************************************************************************
**Name:		 	GetRemoteChannelDefaultVol
**Function:	 	获取方控通道的默认电压
*说明：读取通道remoteChx的电压值，经过延时消除抖动后发送给上层。
**Args:
**Return:
******************************************************************************/
void GetRemoteChannelDefaultVol(RemoteKeyChannelTypedef *remoteChx)
{
	GetRemoteChannelStableVol(remoteChx);
	remoteChx->defaultVoltage = remoteChx->valueNow;
}
/*****************************************************************************
**Name:		 	SubVoltage
**Function:	 	获取电压差值
*说明：
**Args:val1：大值
**Args:val2：小值
**Return:		逻辑最小值
******************************************************************************/
uint32_t SubVoltage(uint32_t val1, uint32_t val2)
{
	return val1 > val2 ? val1 - val2 : 0;
}
/*****************************************************************************
**Name:		 	AddVoltage
**Function:	 	获取电压和
*说明：
**Args:val1
**Args:val2
**Return:		逻辑最大值
******************************************************************************/
uint32_t AddVoltage(uint32_t val1, uint32_t val2)
{
	uint32_t res = 0;

	res = val1 + val2;
	res = res < val1 ? 0xffffffff : res;
	res = res < val2 ? 0xffffffff : res;

	return res;
}
/*****************************************************************************
**Name:		 	GetRemoteChannelVol
**Function:	 	方控按键电压值读取并发送
*说明：			通道电压值下跌，将通道电压值发给上层；通道电压值上升，发3300给上层。
**Args:
**Return:
******************************************************************************/
void GetRemoteChannelVol(RemoteKeyChannelTypedef *remoteChx)
{
	uint32_t valTmp;
	
	valTmp = GetChannelVol(remoteChx->chId);

	switch(remoteChx->chState)
	{
		/* Wait press or release */
		case 0x00:
			{
				if(valTmp <= SubVoltage(remoteChx->defaultVoltage, remoteChx->thresholdVal))
				{
					remoteChx->chState = 0x10;
					ResetUserTimer(&remoteChx->delayTimer);
				}
				else if(valTmp > AddVoltage(remoteChx->defaultVoltage, remoteChx->thresholdVal))
				{
					remoteChx->chState = 0x40;
					ResetUserTimer(&(remoteChx->delayTimer));
				}
			}
			break;

		/* Pressed */
		case 0x10:
			{
				if(valTmp >= SubVoltage(remoteChx->defaultVoltage, remoteChx->thresholdVal))
				{
					remoteChx->chState = 0x00;
					ResetUserTimer(&remoteChx->delayTimer);
				}
				else
				if(ReadUserTimer(&remoteChx->delayTimer) >= REMOTE_KEY_DELAY_TIME_PRESS)
				{
					GetRemoteChannelStableVol(remoteChx);
					/*Send to host*/
					SEND_REMOTE_KEY_TO_HOST(remoteChx->chId, remoteChx->valueNow);
					remoteChx->defaultVoltage = remoteChx->valueNow;

					remoteChx->chState = 0x00;
					ResetUserTimer(&(remoteChx->delayTimer));
				}
			}
			break;

		/* Released */
		case 0x40:
			{
				if(ReadUserTimer(&remoteChx->delayTimer) >= REMOTE_KEY_DELAY_TIME_RELEASE)
				{
					GetRemoteChannelStableVol(remoteChx);
					/*Send to host*/
					SEND_REMOTE_KEY_TO_HOST(remoteChx->chId, REMOTE_KEY_NO_KEY);
					remoteChx->defaultVoltage = remoteChx->valueNow;
					
					remoteChx->chState = 0x00;
					ResetUserTimer(&(remoteChx->delayTimer));
				}
			}
			break;

		default:
			remoteChx->chState = 0;
			break;
	}
}

/*****************************************************************************
**Name:		 	CheckRemoteKey
**Function:	 	方控按键检测
**Args:
**Return:
******************************************************************************/
void CheckRemoteKey(void)
{
	switch(adRemoteSyncStep)
	{
		/*等待开机稳定*/
		case AD_SYNC_INITIALIZE:
			if(ReadUserTimer(&PowerOnTimer) >= T_100MS * 20)
			{
				adRemoteSyncStep = AD_SYNC_CALIBRATION;
				ResetUserTimer(&PowerOnTimer);
			}
			break;

		/*检测按键默认电压*/
		case AD_SYNC_CALIBRATION:
			if(ReadUserTimer(&PowerOnTimer) < T_100MS * 1)
			{
				GetRemoteChannelDefaultVol(&tRemoteKeyCh);
			}
			else
			{
				SEND_REMOTE_KEY_TO_HOST(tRemoteKeyCh.chId, REMOTE_KEY_NO_KEY);

				adRemoteSyncStep = AD_SYNC_WORKING;
				ResetUserTimer(&PowerOnTimer);
			}
			break;

		/*按键处理*/
		case AD_SYNC_WORKING:
			{
					GetRemoteChannelVol(&tRemoteKeyCh);
			}
			break;

		default:
			adRemoteSyncStep = AD_SYNC_INITIALIZE;
			ResetUserTimer(&PowerOnTimer);
			break;
	}

}

/*****************************************************************************
**Name:		 	CheckStatus
**Function:
**Args:
**Return:
******************************************************************************/
void CheckStatus(void)
{
	//以按键方式检测
	CheckRemoteKey();

	//实时检测ADC
	CheckBatteryVol();
}

