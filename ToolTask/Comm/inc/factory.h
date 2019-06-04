#ifndef _FACTORY_H_
#define _FACTORY_H_

#include "def.h"
#include "interface.h"

void EnterFactoryState();
void ExitFactoryState();
BOOL FactoryGetState();
void FactoryTaskProc(void);
void FactoryComRxProc(uint8 * data);
void FactoryTransmit(uint8 *, uint16);


#endif
