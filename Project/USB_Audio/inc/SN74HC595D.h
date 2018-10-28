#ifndef SN74HC595D_H_
#define SN74HC595D_H_
#include "stm32f10x_conf.h"

void SN74HC595DIOInit(void);
void SN74HC595DIOClear(void);
void SN74HC595DIOMemClear(void);
void SN74HC595DIOCtrl(u8 u8Index, BitAction emAction);
void SN74HC595DIOFlush(void);


#endif

