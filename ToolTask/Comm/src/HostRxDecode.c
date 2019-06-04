#include "HostRxDecode.h"
#ifdef COMDECODEVW

/* USER CODE BEGIN Includes */
#include <string.h>
#include "def.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/


/* USER CODE BEGIN 0 */
/**
  * @brief  计算校验值.
  			计算：SUM（DataType,Length,Data0…DataN）^0Xff.
  * @param  address 数据首地址.
  * @param  offset 计算偏移量.
  * @param  length 从offset开始计算length个数据.
  * @retval 计算的结果.
  */
static uint8 CalculateCheckSum(uint32_t *address, uint8 offset, uint8 length)
{
	uint8 i = 0;
	uint8 tmp = 0;
	uint32_t addr = (uint32_t)address;
	addr += offset;

	while(i++ < length)
	{
		tmp += *(uint8 *)addr++;
	}

	tmp ^= 0xff;
	return tmp;
}
/**
  * @brief  从数据流中解析出数据帧并调用解析回调函数;
  			协议帧:0x2e--datatype--length--data--checksum.
  * @param  pData 待解析的数据流的首地址.
  * @param  length 数据流的长度.
  * @param  baseInfo 基本接口信息，包含回调函数.
  * @retval 解析的结果，0 未解析完成; 0xf0 校验计算错误; 0xf0 获取到了有效数据.
  */
int HostRxDecode(unsigned char * const pData, unsigned short length, void * const pBaseInfo)
{
	static uint8 ReceiveStatus = 0;
	static uint8 CmdBuf[256];

	static uint16 DataLen = 0;
	static uint16 CurIndex = 0;
	static u8 checksum, data;
        unsigned char * pdata = pData;

	for(; length > 0; length--)
	{
		data = *pdata++;
		
		switch(ReceiveStatus)
		{
			/* Get frame start */
			case 0x00:
				if(0x2e == data)
				{
					ReceiveStatus = 0x01;
				}
				else
				{
					((baseInfoTypedef *)pBaseInfo)->GotAckFromHost(data);
				}
			break;

			/* Get command type */
			case 0x01:
				/* The first data is cmd type */
				CmdBuf[0] = data;
				checksum = data;
				ReceiveStatus = 0x02;
			break;

			/* Get data length */
			case 0x02:
				if(data >= sizeof(CmdBuf))
				{
					ReceiveStatus = 0x00;
					break;
				}
				DataLen = data;
				CurIndex = 0;
				checksum += data;
				ReceiveStatus = 0x03;
			break;

			/* Get data and check data */
			case 0x03:
				if(CurIndex++ < DataLen)
				{
					CmdBuf[CurIndex] = data;
					checksum += data;
				}
				else
				{
					checksum ^= 0xff;
					if(checksum == data)
					{
						/*Establish connection*/
						((baseInfoTypedef *)pBaseInfo)->ConnectEstablish(pData[1]);
						
						/* Process command from host */
						((baseInfoTypedef *)pBaseInfo)->RxDecodeCallBackDef(CmdBuf, DataLen + 1, pBaseInfo);
						ReceiveStatus = 0x00;

						/* Return ask to host */
						return 0xff;
					}
					else
					{
						ReceiveStatus = 0x00;

						/* Return nack to host */
						return 0xf0;
					}
				}
			break;

			default:
				ReceiveStatus = 0x00;
			break;
		}
	}

	/*Default return*/
	return 0x00;
}
/* USER CODE END 0 */

#endif
