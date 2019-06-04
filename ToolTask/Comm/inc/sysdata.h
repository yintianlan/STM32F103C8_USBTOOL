#ifndef _SYSDATA_H_
#define _SYSDATA_H_

/****************软件信息*******************/
#include "FlyConfig.h"

/*车型ID定义，8字节，定义查看文档，定义在car.h中*/
#ifndef FLY_CAR_TYPE_ID
#define FLY_CAR_TYPE_ID		"F0F0F0F0"
#endif

/*软件版本，示例：1.0.0*/
#ifndef FLY_SW_VERSION
#define FLY_SW_VERSION		"1.0.0"
#endif

/****************软件信息*******************/

int BootloaderIdCheck(void);


#endif
