#include <stdio.h>
#include <string.h>

#include "sysdata.h"
#include "FlyConfig.h"

#if RELEASE_VER

#define BOOTCONFIG_DATA_START_ADDR		(0x08002700)	//Boot配置区数据索引首地址
#define BOOTCONFIG_DATA_END_ADDR		(0x08002760)	//Boot配置区数据索引末地址
#define SYSCONFIG_DATA_START_ADDR		(0x08002800)	//System配置区数据索引首地址
#define SYSCONFIG_DATA_END_ADDR			(0x08002860)	//System配置区数据索引末地址


typedef struct
{
	char 	*pName;			//参数名指针
	int 	nameLength;		//参数名长度
	char 	*pData;			//数据指针
	int 	dataLength;		//数据长度
}BootloaderDataTypedef;


/***************************Bootloader配置存储区************************/

// BOOT_DATA_SECTION is [ from 0x08003000 to 0x08003100]
/*数据区 0x08002860 - 0x08002900*/
#pragma location="CONFIG_DATA_SECTION"

__root const char SYSTEM_IDENTIFIER_NAME[] @ "CONFIG_DATA_SECTION" = "SYSTEMID";
__root const char SYSTEM_IDENTIFIER[] @ "CONFIG_DATA_SECTION" = "H3V2R3N4261NPYODSJUDRQWFV";

__root const char SYSTEM_VERSION_NAME[] @ "CONFIG_DATA_SECTION" = "SYSTEMVER";
__root const char SYSTEM_VERSION[] @ "CONFIG_DATA_SECTION" = FLY_SW_VERSION;

/*Bootloader ID，System需要验证该ID才能启动*/
__root const char BOOTLOADER_IDENTIFIER_NAME[] @ "CONFIG_DATA_SECTION" = "BOOTID";
__root const char BOOTLOADER_IDENTIFIER[] @ "CONFIG_DATA_SECTION" = "JEDJZFQA8X7SXMUCIH0SBBB0I";

/*车型参数定义*/
__root const char SYSTEM_TYPE_NAME[] @ "CONFIG_DATA_SECTION" = "CARTYPE";
__root const char SYSTEM_TYPE[] @ "CONFIG_DATA_SECTION" = FLY_CAR_TYPE_ID;			//车型ID，8字符，定义查看文档


/*索引区 0x08002800 - 0x08002860, 存放数据的引用*/
#pragma location="CONFIG_TABLE_SECTION"

/*主程序存在标志*/
#if RELEASE_VER
__root const char SYSTEM_EXIST[] @ "CONFIG_TABLE_SECTION" = "000000000000000";
#else
__root const char SYSTEM_EXIST[] @ "CONFIG_TABLE_SECTION" = "0O413DNVC1XWY6G";
#endif

/*启动验证ID，Bootloader验证此ID引导System*/
__root const BootloaderDataTypedef SystemIdentifier @ "CONFIG_TABLE_SECTION"  = 
{
	.pName = (char *)SYSTEM_IDENTIFIER_NAME,
	.nameLength = sizeof(SYSTEM_IDENTIFIER_NAME),
	.pData = (char *)SYSTEM_IDENTIFIER,
	.dataLength = sizeof(SYSTEM_IDENTIFIER)
};

/*Bootloader验证ID*/
__root const BootloaderDataTypedef BootloaderIdentifier @ "CONFIG_TABLE_SECTION"  = 
{
	.pName = (char *)BOOTLOADER_IDENTIFIER_NAME,
	.nameLength = sizeof(BOOTLOADER_IDENTIFIER_NAME),
	.pData = (char *)BOOTLOADER_IDENTIFIER,
	.dataLength = sizeof(BOOTLOADER_IDENTIFIER)
};

/*System版本号*/
__root const BootloaderDataTypedef SystemVersion @ "CONFIG_TABLE_SECTION"  = 
{
	.pName = (char *)SYSTEM_VERSION_NAME,
	.nameLength = sizeof(SYSTEM_VERSION_NAME), 
	.pData = (char *)SYSTEM_VERSION,
	.dataLength = sizeof(SYSTEM_VERSION)
};

/*System车型参数*/
__root const BootloaderDataTypedef SystemType @ "CONFIG_TABLE_SECTION"  = 
{
	.pName = (char *)SYSTEM_TYPE_NAME,
	.nameLength = sizeof(SYSTEM_TYPE_NAME), 
	.pData = (char *)SYSTEM_TYPE,
	.dataLength = sizeof(SYSTEM_TYPE)
};

/**
  * @brief  验证Bootloader ID
  * @note   
  * @rmtoll 
  * @param  None
  * @retval None	ID匹配返回0，其它返回非0
  */
int BootloaderIdCheck(void)
{
	int bootAddr = BOOTCONFIG_DATA_START_ADDR;
	int gotItem = 0, checkOk = 0;
	BootloaderDataTypedef configTmp;

	for(; bootAddr < BOOTCONFIG_DATA_END_ADDR ; )
	{
		if(*(unsigned int *)bootAddr != 0xffffffff)
		{
			/*读取配置列表*/
			memcpy(&configTmp, (unsigned int *)bootAddr, 0x10);

			/*找到System ID参数项*/
			if(memcmp(configTmp.pName, BootloaderIdentifier.pName, configTmp.nameLength) == 0)
			{
				gotItem = 1;

				/*ID匹配*/
				if(memcmp(configTmp.pData, BootloaderIdentifier.pData, configTmp.dataLength) == 0)
				{
					checkOk = 1;
					break;
				}
			}
		}
		
		bootAddr += 0x10;
	}

	/*返回结果*/
	if((gotItem != 0) && (checkOk != 0))
	{
		return 0;
	}
	else
	{
		return -1;
	}

}

#endif
