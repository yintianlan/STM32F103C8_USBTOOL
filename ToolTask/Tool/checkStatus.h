#ifndef _CHECKSTATUS_H_
#define _CHECKSTATUS_H_

#ifndef		_checkStatus_GLOBAL_
    #define		checkStatus_GLOBAL_		extern
#else
    #define		checkStatus_GLOBAL_
#endif

#define ADC_CONVERTED_DATA_BUFFER_SIZE		(3)


typedef enum
{
	REMOTE_01 = 0,
	REMOTE_REF,
	MAX_CH = REMOTE_REF,
}AdcChannelTypedef;


typedef struct
{
	uint32_t Value[ADC_CONVERTED_DATA_BUFFER_SIZE];
}AdcValueTypedef;



checkStatus_GLOBAL_ void CheckStatus(void);
checkStatus_GLOBAL_ void AdcRemoteStartCalibrate(void);

#endif

