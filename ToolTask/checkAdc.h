#ifndef _CHECKADC_H_
#define _CHECKADC_H_

#ifndef		_checkAdc_GLOBAL_
    #define		checkAdc_GLOBAL_		extern
#else
    #define		checkAdc_GLOBAL_
#endif

#define ADC_CONVERTED_DATA_BUFFER_SIZE		(2)
#define REMOTE1 (1)
#define REMOTE2 (2)

/*ADC通道*/
typedef enum
{
	REMOTE_01 = 0,
	REMOTE_REF,
	MAX_CH = REMOTE_REF,
}AdcChannelTypedef;

/*存放ADC数据数组*/
typedef struct
{
	uint32_t Value[ADC_CONVERTED_DATA_BUFFER_SIZE];
}AdcValueTypedef;

extern AdcValueTypedef  tAdcValue;


checkAdc_GLOBAL_ void CheckStatus(void);
checkAdc_GLOBAL_ void AdcRemoteStartCalibrate(void);

#endif	/*_CHECKADC_H_*/
