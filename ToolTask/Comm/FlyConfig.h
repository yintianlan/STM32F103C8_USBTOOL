#ifndef _FLYCONFIG_H_
#define _FLYCONFIG_H_

#include "Interface.h"


/* 定义log等级 */
#define DEBUG_FORBID				(0)
#define DEBUG_ERROR					(0x01 << 0)
#define DEBUG_WARNG					(0x01 << 1)
#define DEBUG_MSG01					(0x01 << 2)
#define DEBUG_MSG02					(0x01 << 3)
#define DEBUG_MSG04					(0x01 << 4)
/* 定义log等级 */

/* Heap trace调试 */
#if RELEASE_VER
#define HEAP_TRACE_DEBUG			(0)
#else
#define HEAP_TRACE_DEBUG			(1)
#endif
/* Heap trace调试 */

#if RELEASE_VER
#define HAVE_BOOTLOADER				(1)
#define IWDG_ENABLE					(1)
#else
#define HAVE_BOOTLOADER				(0)
#define IWDG_ENABLE					(0)
#endif

/* 选择打印等级 */
#if RELEASE_VER
#define DEBUG_OUT_LEVEL				(DEBUG_FORBID)
#else
#define DEBUG_OUT_LEVEL				(0x0f)
#endif
/* 选择打印等级 */

/* 数据发送应答配置 */
#define		HOST_NEED_ACK			(1)				/* 串口发送给主机是否需要等待应答 */
#define		ACK_WAIT_MS				(100)			/* 应答等待时间(ms) */
#define		ACK_REPEAT_CT			(3)				/* 最大重发次数 */
/* 数据发送应答配置 */

/* CAR ID BEGIN Defines*/
#define 	CAR_001					(0x01)			/**/
/* CAR ID END Defines*/

/* CAR SELECT BEGIN Defines*/
#define 	CAR_SEL					CAR_001
/* CAR SELECT END Defines*/

/* CAR PARAMETER BEGIN Defines*/
#if CAR_SEL==CAR_001
	#define 	MAGOTAN2018
	#define 	COMDECODEVW
	#define 	VerCarTypeStr		"VW01"		/* 车型代号 */
	#define 	VerSoftwareStr		"1.0.2"		/* 该车型的软件版本 */
#else
	#define		VerCarTypeStr		"****"
#endif
/* CAR PARAMETER END Defines*/


#define  DEBUG_DG_SEVERE (1)

#define FLY_SW_VERSION 		VerSoftwareStr
#define FLY_CAR_TYPE_ID 	VerCarTypeStr

#endif
