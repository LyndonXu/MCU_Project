#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f10x_conf.h"
#include "user_conf.h"
#include "io_buf_ctrl.h"
#include "app_port.h"
#include "pwm.h"


#include "key_led.h"
#include "adc_ctrl.h"
#include "code_switch.h"
#include "key_led_ctrl.h"

#include "buzzer.h"

#include "user_init.h"
#include "user_api.h"

#include "key_led_table.h"

#include "protocol.h"
#include "message.h"
#include "message_2.h"
#include "flash_ctrl.h"
#include "extern_io_ctrl.h"

#include "wm8776.h"

int main()
{
	u32 u32SyncCount = 0;
	u32 u32RedressTime;


	KeyBufInit();
	GlobalStateInit();
	
	
	PeripheralPinClkEnable();
	OpenSpecialGPIO();
	Timer2InitTo1us();

	ReadSaveData();
	MessageUartInit();

#if 0	
	KeyLedInit();
	ChangeAllLedState(false);
	RockPushRodInit();
	CodeSwitchInit();
	BuzzerInit();
	
	
	PWMCtrlInit();
#endif

	SysTickInit();
	ChangeEncodeState();
	
	/* WM8776 & other */
	WM8776Init();
	{
		u32RedressTime = g_u32SysTickCnt;
		while(SysTimeDiff(u32RedressTime, g_u32SysTickCnt) < 2000);/* 延时2S */		
	}
	AudioModeCtrlInit();
	AudioVolumeInit();
	AudioSrcInit();
	FantasyPowerInit();
	
	do
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	}while(0);


#if 0
	/* 打开所有LED */

	ChangeAllLedState(true);
	u32SyncCount = 0;
	while (u32SyncCount < 5)
	{
		u8 u8Buf[PROTOCOL_YNA_ENCODE_LENGTH];
		u8 *pBuf = NULL;
		void *pMsgIn = NULL; 

		u32RedressTime = g_u32SysTickCnt;

		pBuf = u8Buf;
		memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

		pBuf[_YNA_Sync] = 0xAA;
		pBuf[_YNA_Mix] = 0x07;
		pBuf[_YNA_Cmd] = 0xC0;

		YNAGetCheckSum(pBuf);
		CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
		
		pMsgIn = MessageUartFlush(false);
		if (pMsgIn != NULL)
		{
			MessageUartRelease(pMsgIn);
			break;
		}
		u32SyncCount++;		
		while(SysTimeDiff(u32RedressTime, g_u32SysTickCnt) < 1000);/* 延时1S */
	}
#endif	

	ChangeAllLedState(false);
	GlobalStateInit();
	do 
	{
		void *pFIFO = KeyBufGetBuf();
		if (pFIFO == NULL)
		{
			break;
		}
		KeyBufGetEnd(pFIFO);
	}while(1);

	LoadPowerOffMemoryToDevice();
	
	u32RedressTime = g_u32SysTickCnt;
	while (1)
	{
		void *pMsgIn = MessageUartFlush(false);
		void *pKeyIn = KeyBufGetBuf();

		if (pKeyIn != NULL)
		{
			KeyProcess(pKeyIn);
		}	
		if (pMsgIn != NULL)
		{
			if (BaseCmdProcess(pMsgIn, &c_stUartIOTCB) != 0)			
			{
				PCEchoProcess(pMsgIn);
			}
		}
		
		
		KeyBufGetEnd(pKeyIn);				
		MessageUartRelease(pMsgIn);

		PowerOffMemoryFlush();
		AudioVolumeGradientFlush();		
		FlushExternVolumeCmd();
		
		if (SysTimeDiff(u32RedressTime, g_u32SysTickCnt) > 500)
		{
			static bool boToggle = false;
			u32RedressTime = g_u32SysTickCnt;

			GPIO_WriteBit(GPIOA, GPIO_Pin_0, boToggle ? Bit_RESET : Bit_SET);

			GPIO_WriteBit(GPIOA, GPIO_Pin_1, boToggle ? Bit_RESET : Bit_SET);
			boToggle = !boToggle;

		}
	}
}
