/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：SN74HC595D.c
* 摘要: 74HC595D 控制程序
* 版本：0.0.1
* 作者：许龙杰
* 日期：2018年10月28日
*******************************************************************************/
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f10x_conf.h"
#include "user_conf.h"
#include "user_api.h"
#include "SN74HC595D.h"

/* shift regsiter clk */
#define SRCLK_PIN			GPIO_Pin_9
#define SRCLK_PORT			GPIOC

/* regsiter clk */
#define RCLK_PIN			GPIO_Pin_8
#define RCLK_PORT			GPIOC

/* enable pin */
#define OE_PIN				GPIO_Pin_15
#define OE_PORT				GPIOA

/* serial input pin */
#define SER_PIN				GPIO_Pin_8
#define SER_PORT			GPIOA


#define IO_CNT			8


#define IO_BYTE			((IO_CNT + 7) / 8)


static u8 s_u8ExternIO[IO_BYTE] = {0};
static u8 s_u8ExternIOBackup[IO_BYTE] = {0};

void SN74HC595DIOInit(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_InitStructure.GPIO_Pin = SER_PIN;
	GPIO_Init(SER_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(SER_PORT, OE_PIN, Bit_RESET);

	GPIO_InitStructure.GPIO_Pin = SRCLK_PIN;
	GPIO_Init(SRCLK_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(SRCLK_PORT, SRCLK_PIN, Bit_SET);

	GPIO_InitStructure.GPIO_Pin = RCLK_PIN;
	GPIO_Init(RCLK_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(RCLK_PORT, RCLK_PIN, Bit_SET);

	GPIO_InitStructure.GPIO_Pin = OE_PIN;
	GPIO_Init(OE_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(OE_PORT, OE_PIN, Bit_RESET);


}

void SN74HC595DIOClear(void)
{
	memset(s_u8ExternIO, 0, sizeof(s_u8ExternIO));
	memset(s_u8ExternIOBackup, 0xFF, sizeof(s_u8ExternIOBackup));

	SN74HC595DIOFlush();
}


void SN74HC595DIOMemClear(void)
{
	memset(s_u8ExternIO, 0, sizeof(s_u8ExternIO));
}

void SN74HC595DIOCtrl(u8 u8Index, BitAction emAction)
{
	u32 u32Hight, u32Low;
	if (u8Index >= IO_CNT)
	{
		return;
	}
	u32Hight = u8Index >> 3;
	u32Low = u8Index & 0x07;
	if (emAction == Bit_RESET)
	{
		s_u8ExternIO[u32Hight] &= (~(1 << u32Low));
	}
	else
	{
		s_u8ExternIO[u32Hight] |= (1 << u32Low);
	}
	
}

void SN74HC595DIOFlush(void)
{
	if (memcmp(s_u8ExternIOBackup, s_u8ExternIO, IO_BYTE) != 0)
	{
		s32 i;
		u32 u32Hight, u32Low;
		BitAction emAction;
		
		for (i = IO_CNT - 1; i >= 0; i--)
		{
			u32Hight = i >> 3;
			u32Low = i & 0x07;
			if (((s_u8ExternIO[u32Hight] >> u32Low) & 0x01) != 0)
			{
				emAction = Bit_RESET;
			}
			else
			{
				emAction = Bit_SET;
			}
			
			GPIO_WriteBit(SER_PORT, SER_PIN, emAction);
			GPIO_WriteBit(SRCLK_PORT, SRCLK_PIN, Bit_RESET);
			GPIO_WriteBit(SRCLK_PORT, SRCLK_PIN, Bit_SET);
		}

		GPIO_WriteBit(RCLK_PORT, RCLK_PIN, Bit_RESET);
		GPIO_WriteBit(RCLK_PORT, RCLK_PIN, Bit_SET);
	
		GPIO_WriteBit(SRCLK_PORT, SRCLK_PIN, Bit_SET);
		GPIO_WriteBit(RCLK_PORT, RCLK_PIN, Bit_SET);
		
		memcpy(s_u8ExternIOBackup, s_u8ExternIO, IO_BYTE);
	}
}

