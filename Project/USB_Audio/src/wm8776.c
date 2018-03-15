/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：wm8776.c
* 摘要: WM8776控制源文件
* 版本：0.0.1
* 作者：许龙杰
* 日期：2017年11月29日
*******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "I2C.h"
#include "wm8776.h"
#include "protocol.h"
#include "flash_ctrl.h"

#define EXTERN_VOLUME_CTRL_VOLUME			0x10
#define EXTERN_VOLUME_CTRL_MUTE				0x20
#define EXTERN_VOLUME_CTRL_CHANNEL1			0x01
#define EXTERN_VOLUME_CTRL_CHANNEL2			0x02
#define EXTERN_VOLUME_CTRL_CHANNEL_ALL		0x03

typedef struct _tagStExternVolumeCmd
{
	uint8_t u8Data;
	uint8_t u8Cmd;
}StExternVolumeCmd;

uint8_t const c_u8ExternVolumeChanelToDevice[TOTAL_EXTERN_VOLUME_CHANNEL] =
{
	/* _Channel_AIN_1 ~ 5, _Channel_InnerSpeaker, _Channel_NormalOut */
	5, 4, 3, 2, 0, 1, 6
};

uint8_t const c_u8ExternVolumeChanelToGlobleChannel[TOTAL_EXTERN_VOLUME_CHANNEL] =
{
	/* _Channel_AIN_1 ~ 5, _Channel_InnerSpeaker, _Channel_NormalOut */
	_Channel_AIN_1,
	_Channel_AIN_2,
	_Channel_AIN_3,
	_Channel_AIN_4,
	_Channel_AIN_5,
	_Channel_InnerSpeaker,
	_Channel_NormalOut,
};


static StExternVolumeCmd s_stExternVolumeCmd[TOTAL_EXTERN_VOLUME_CHANNEL] = { 0 };


#define MSG_SPI_SHDN_PIN 				GPIO_Pin_6
#define MSG_SPI_SHDN_PORT	 			GPIOC

#define MSG_SPI_RS_PIN 					GPIO_Pin_7
#define MSG_SPI_RS_PORT	 				GPIOC


#define MSG_SPI_CS_PIN 				GPIO_Pin_12
#define MSG_SPI_SCK_PIN 			GPIO_Pin_13
#define MSG_SPI_MISO_PIN			GPIO_Pin_14
#define MSG_SPI_MOSI_PIN			GPIO_Pin_15
#define MSG_SPI_PORT				GPIOB
#define MSG_SPI						SPI2

#define ENABLE_MSG_SPI()			RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);


static uint16_t s_u16WMReg[_WM_Reg_Reserved] = 
{
	0x00F9, 0x00F9, 0x00F9,			/* Header Phone, enable zero cross */
	0x00FF, 0x00FF, 0x00FF,			/* DAC */
	0x0000,
	0x0091, 0x0000, 0x0000,			/* DAC， DAC ctrl 1 enable zero cross and timeout disable */
	0x0022, 0x0022, 0x0022,			/* Interface */
	0x0008,							/* Power */
	0x01CF, 0x01CF,					/* ADC, enable zero cross */
	0x007B, 0x0000, 0x0032,			/* ALC */
	0x0000, 						/* Noise */
	0x00A6,							/* Limiter */
	0x0001,							/* ADC Mux */
	0x0001,							/* Output Mux */
	0x0000,							/* reset */
	
};


static EmAudioCtrlMode s_emAudioCtrlMode[TOTAL_MODE_CTRL] = 
{
	_Audio_Ctrl_Mode_Normal, 	/* AIN 1 */
	_Audio_Ctrl_Mode_Normal, 	/* AIN 2 */
	_Audio_Ctrl_Mode_Normal,	/* AIN 3 */
	_Audio_Ctrl_Mode_Normal,	/* AIN 4 */	
	_Audio_Ctrl_Mode_Normal,	/* AIN 5 */ 
	_Audio_Ctrl_Mode_Normal,	/* Ain Mux */
	_Audio_Ctrl_Mode_Normal,	/* Digital PC */
	_Audio_Ctrl_Mode_Normal,	/* Header Phone */
	_Audio_Ctrl_Mode_Normal,	/* Inner Speaker */
	_Audio_Ctrl_Mode_Normal,	/* Normal Out */

};



static StVolume s_stVolume[TOTAL_VOLUME_CHANNEL] = 
{
	{0x80, 0x80},			/* AIN 1 */
	{0x80, 0x80},     		/* AIN 2 */
	{0x80, 0x80},     		/* AIN 3 */
	{0x80, 0x80},     		/* AIN 4 */
	{0x80, 0x80},     		/* AIN 5 */
	/* reg:[0,255], use [0, 207], real:[0,255], 80% is 204(0xCC), Ain Mux */
	{0xCF, 0xCF},					
	/* reg:[0,255], use [0, 255], real:[0,255], 90% is 230(0xE6), Digital PC */
	{0xE6, 0xE6}, 					
	/* reg:[0,127], use [0, 225], real:[0,255], 90% is 230(0xE6), Header Phone */
	{0xE6, 0xE6},  		
	{0x80, 0x80},				/* Inner Speaker */
	{0x80, 0x80},				/* Normal Out */
};


static StVolume s_stVolumeGradient[TOTAL_VOLUME_CHANNEL] = 
{
	{0x80, 0x80},			/* AIN 1 */
	{0x80, 0x80},     		/* AIN 2 */
	{0x80, 0x80},     		/* AIN 3 */
	{0x80, 0x80},     		/* AIN 4 */
	{0x80, 0x80},     		/* AIN 5 */
	/* reg:[0,255], use [0, 207], real:[0,255], 80% is 204(0xCC), Ain Mux */
	{0xCF, 0xCF},					
	/* reg:[0,255], use [0, 255], real:[0,255], 90% is 230(0xE6), Digital PC */
	{0xE6, 0xE6}, 					
	/* reg:[0,127], use [0, 225], real:[0,255], 90% is 230(0xE6), Header Phone */
	{0xE6, 0xE6},  		
	{0x80, 0x80},				/* Inner Speaker */
	{0x80, 0x80},				/* Normal Out */
};



static void ExternInterfaceInit(void);



s32 WM8776Write(u8 u8Cmd, u16 u16Data)
{
	u8 u8Addr = (0x34 >> 1);
	u8 u8Data;
	u16Data &= 0x01FF;
	
	u8Data = u16Data;
	u8Cmd = (u8Cmd << 1) | (u16Data >> 8);
#if 1	
	{
		u8 u8EchoBase[8] = {0};
		u8EchoBase[_YNA_Sync] = 0xAA;
		u8EchoBase[_YNA_Mix] = 0x06;
		u8EchoBase[_YNA_Cmd] = 0x50;
		u8EchoBase[_YNA_Data1] = u8Addr;
		u8EchoBase[_YNA_Data2] = u8Cmd;
		u8EchoBase[_YNA_Data3] = u8Data;

		YNAGetCheckSum(u8EchoBase);
		CopyToUartMessage(u8EchoBase, 8);
	}
#endif	
	//return 0;
	return I2CWrite(u8Addr, u8Cmd, u8Data);
}

void WM8776Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_4, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);

	I2CInit();
	//WM8776Write(_WM_Reg_OutputMux, 0x07);
	//s_u16WMReg[_WM_Reg_OutputMux] = 0x07;
	
	ExternInterfaceInit();

}

int WM8776EnableAINChannel(u8 u8Channel)
{
	int s32Rlst = 0;
	u16 u16Reg = s_u16WMReg[_WM_Reg_ADCMuxCtrl];
	u8Channel &= 0x1F;
	
	u16Reg &= 0xC0;
	u16Reg |= u8Channel;
	
	s32Rlst = WM8776Write(_WM_Reg_ADCMuxCtrl, u16Reg);
	if (s32Rlst == 0)
	{
		s_u16WMReg[_WM_Reg_ADCMuxCtrl] = u16Reg;
	}
	
	return s32Rlst;
}

int WM8776EnableOutputChannel(u8 u8Channel)
{
	int s32Rlst = 0;
	u8Channel &= 0x07;
	
	s32Rlst = WM8776Write(_WM_Reg_OutputMux, u8Channel);
	if (s32Rlst == 0)
	{
		s_u16WMReg[_WM_Reg_OutputMux] = u8Channel;
	}
	
	return s32Rlst;
}

u8 WM8776GetAINChannelEnableState()
{
	return s_u16WMReg[_WM_Reg_ADCMuxCtrl] & 0x1F;
}
u8 WM8776GetOutputChannelEnableState()
{
	return s_u16WMReg[_WM_Reg_OutputMux] & 0x07;
}


static uint8_t s_u8ExternVolumeCmdState = 0;

static StExternVolumeCmd s_stExternVolumeCmdBak[TOTAL_EXTERN_VOLUME_CHANNEL] = { 0 };


void FlushExternVolumeCmd(void)
{
	if (s_u8ExternVolumeCmdState != 0)
	{
		/* <TODO>: send cmd to SPI */
		uint16_t *pCmd = (uint16_t *)s_stExternVolumeCmd;
		uint16_t *pCmdBak = (uint16_t *)s_stExternVolumeCmdBak;
		uint32_t i;
		
		GPIO_WriteBit(MSG_SPI_PORT, MSG_SPI_CS_PIN, Bit_RESET);
		
		memset(s_stExternVolumeCmdBak, 0, sizeof(s_stExternVolumeCmdBak));

		while (SPI_I2S_GetFlagStatus(MSG_SPI, SPI_I2S_FLAG_TXE) == RESET);
		for (i = 0; i < TOTAL_EXTERN_VOLUME_CHANNEL; i++)
		{
			/* Send SPI data */
			SPI_I2S_SendData(MSG_SPI, pCmd[TOTAL_EXTERN_VOLUME_CHANNEL - 1 - i]);

			while (SPI_I2S_GetFlagStatus(MSG_SPI, SPI_I2S_FLAG_TXE) == RESET);

			/* Wait for SPI data reception */
			while (SPI_I2S_GetFlagStatus(MSG_SPI, SPI_I2S_FLAG_RXNE) == RESET);
			/* Read SPI received data */
			pCmdBak[TOTAL_EXTERN_VOLUME_CHANNEL - 1 - i] = 
				SPI_I2S_ReceiveData(MSG_SPI);
			
		}
		
		GPIO_WriteBit(MSG_SPI_PORT, MSG_SPI_CS_PIN, Bit_SET);
		
		
		memset(s_stExternVolumeCmd, 0, sizeof(s_stExternVolumeCmd));
		s_u8ExternVolumeCmdState = 0;
	}
}


int32_t ExternVolumeSetMode(uint8_t u8Index, EmAudioCtrlMode emMode)
{
	uint8_t u8GlobleChannel;
	uint8_t u8DeviceIndex;
	EmAudioCtrlMode emOldMode;
	
	if (u8Index >= TOTAL_EXTERN_VOLUME_CHANNEL)
	{
		return -1;
	}
	
	if (emMode > _Audio_Ctrl_Mode_ShieldLeftAndRight)
	{
		return -1;
	}
	
	u8GlobleChannel = c_u8ExternVolumeChanelToGlobleChannel[u8Index];
	
	emOldMode = s_emAudioCtrlMode[u8GlobleChannel];
	if (emOldMode == emMode)
	{
		return 1;
	}

	if ((emOldMode == _Audio_Ctrl_Mode_ShieldLeft && emMode == _Audio_Ctrl_Mode_ShieldRight)
		|| (emMode == _Audio_Ctrl_Mode_ShieldLeft && emOldMode == _Audio_Ctrl_Mode_ShieldRight))
	{
		return -1;
	}
	
	u8DeviceIndex = c_u8ExternVolumeChanelToDevice[u8Index];
	
	if (emMode == _Audio_Ctrl_Mode_Normal)
	{
		s_stExternVolumeCmd[u8DeviceIndex].u8Cmd = EXTERN_VOLUME_CTRL_VOLUME | 
							EXTERN_VOLUME_CTRL_CHANNEL_ALL;
		s_stExternVolumeCmd[u8DeviceIndex].u8Data = s_stVolume[u8GlobleChannel].u8Channel1;
	}
	else
	{
		EmAudioCtrlMode emCorrectMode = emMode;
#if 0		
		if (emCorrectMode == _Audio_Ctrl_Mode_ShieldLeft)
		{
			emCorrectMode = _Audio_Ctrl_Mode_ShieldRight;
		}
		else if (emCorrectMode == _Audio_Ctrl_Mode_ShieldRight)
		{
			emCorrectMode = _Audio_Ctrl_Mode_ShieldLeft;		
		}
#endif		
		s_stExternVolumeCmd[u8DeviceIndex].u8Cmd = EXTERN_VOLUME_CTRL_MUTE | (((u32)emCorrectMode) & 0x0F);		
	}
	s_emAudioCtrlMode[u8GlobleChannel] = emMode;
	s_u8ExternVolumeCmdState = 1;
	return 0;
	
}

int32_t ExternVolumeSetVolume(uint8_t u8Index, uint8_t u8Volume)
{
	uint8_t u8GlobleChannel;
	uint8_t u8DeviceIndex;
	EmAudioCtrlMode emCurMode;
	uint8_t u8OldVolume;
	
	if (u8Index >= TOTAL_EXTERN_VOLUME_CHANNEL)
	{
		return -1;
	}
	

	u8GlobleChannel = c_u8ExternVolumeChanelToGlobleChannel[u8Index];
	
	u8OldVolume = s_stVolume[u8GlobleChannel].u8Channel1;
	if (u8OldVolume == u8Volume)
	{
		return 1;
	}
	
	emCurMode = s_emAudioCtrlMode[u8GlobleChannel];
	if (emCurMode == _Audio_Ctrl_Mode_ShieldLeftAndRight)
	{
		s_stVolume[u8GlobleChannel].u8Channel1 = 
		s_stVolume[u8GlobleChannel].u8Channel2 = u8Volume;
		return 1;
	}

	u8DeviceIndex = c_u8ExternVolumeChanelToDevice[u8Index];

	s_stExternVolumeCmd[u8DeviceIndex].u8Data = u8Volume;
	if (emCurMode == _Audio_Ctrl_Mode_ShieldLeft)
	{
		s_stExternVolumeCmd[u8DeviceIndex].u8Cmd = EXTERN_VOLUME_CTRL_VOLUME | 
							EXTERN_VOLUME_CTRL_CHANNEL2;
	}
	else if (emCurMode == _Audio_Ctrl_Mode_ShieldRight)
	{
		s_stExternVolumeCmd[u8DeviceIndex].u8Cmd = EXTERN_VOLUME_CTRL_VOLUME | 
							EXTERN_VOLUME_CTRL_CHANNEL1;	
	}
	else
	{
		s_stExternVolumeCmd[u8DeviceIndex].u8Cmd = EXTERN_VOLUME_CTRL_VOLUME | 
							EXTERN_VOLUME_CTRL_CHANNEL_ALL;	
	}
	
	s_stVolume[u8GlobleChannel].u8Channel1 = 
	s_stVolume[u8GlobleChannel].u8Channel2 = u8Volume;
	s_u8ExternVolumeCmdState = 1;
	
	return 0;
	
}



static void ExternInterfaceInit(void)
{
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	ENABLE_MSG_SPI();

	SPI_StructInit(&SPI_InitStructure);
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	//SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(MSG_SPI, &SPI_InitStructure);

	SPI_Cmd(MSG_SPI, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = MSG_SPI_SCK_PIN | MSG_SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(MSG_SPI_PORT, &GPIO_InitStructure);
	
	
	GPIO_InitStructure.GPIO_Pin = MSG_SPI_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(MSG_SPI_PORT, &GPIO_InitStructure);


	GPIO_InitStructure.GPIO_Pin = MSG_SPI_SHDN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(MSG_SPI_SHDN_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(MSG_SPI_SHDN_PORT, MSG_SPI_SHDN_PIN, Bit_SET);
		

	GPIO_InitStructure.GPIO_Pin = MSG_SPI_RS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(MSG_SPI_RS_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = MSG_SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(MSG_SPI_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(MSG_SPI_PORT, MSG_SPI_CS_PIN, Bit_SET);


	GPIO_WriteBit(MSG_SPI_RS_PORT, MSG_SPI_RS_PIN, Bit_RESET);
	
	{
		u32 u32RedressTime = g_u32SysTickCnt;
		while(SysTimeDiff(u32RedressTime, g_u32SysTickCnt) < 100);		
	}

	GPIO_WriteBit(MSG_SPI_RS_PORT, MSG_SPI_RS_PIN, Bit_SET);

}





static void AudioModePinInit(void)
{
}

int GetAudioCtrlMode(u32 u32Channel, EmAudioCtrlMode *pMode)
{
	if ((u32Channel >= TOTAL_MODE_CTRL) || (pMode == NULL))
	{
		return -1;
	}
	
	*pMode = s_emAudioCtrlMode[u32Channel];
	
	return 0;
}

void GetAllAudioCtrlMode(EmAudioCtrlMode emAudioCtrlMode[TOTAL_MODE_CTRL])
{
	int s32Size = sizeof(EmAudioCtrlMode) * TOTAL_MODE_CTRL;
	memcpy(emAudioCtrlMode, s_emAudioCtrlMode, s32Size);
}
void SetAllAudioCtrlMode(EmAudioCtrlMode emAudioCtrlMode[TOTAL_MODE_CTRL])
{
	int i;
	for (i = 0; i < TOTAL_MODE_CTRL; i++)
	{
		SetAudioCtrlMode(i, emAudioCtrlMode[i]);
	}
}


typedef int(*PFUN_SetAudioCtrlMode)(EmAudioCtrlMode emMode, bool boIsForce);


int SetAudioCtrlModeAinMux(EmAudioCtrlMode emMode, bool boIsForce)
{
#if 1
	uint16_t u16Reg = s_u16WMReg[_WM_Reg_ADCMuxCtrl];
	if (emMode > _Audio_Ctrl_Mode_ShieldLeftAndRight)
	{
		return -1;
	}
	if (!boIsForce)
	{
		if (emMode == s_emAudioCtrlMode[_Channel_AIN_Mux])
		{
			return 0;
		}
	}
	
	u16Reg &= 0x003F;
	switch(emMode)
	{
		case _Audio_Ctrl_Mode_Normal:
		{
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeft:
		{	
			u16Reg |= 0x0080;
			break;
		}
		case _Audio_Ctrl_Mode_ShieldRight:
		{
			u16Reg |= 0x0040;
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeftAndRight:
		{
			u16Reg |= 0x00C0;
			break;
		}
		default:
			return -1;
	}
	
	if (WM8776Write(_WM_Reg_ADCMuxCtrl, u16Reg) != 0)
	{
		return -1;
	}
	s_u16WMReg[_WM_Reg_ADCMuxCtrl] = u16Reg;
#else
	
	uint16_t u16Reg = s_u16WMReg[_WM_Reg_PWRDownCtrl];
	if (emMode >= _Audio_Ctrl_Mode_LeftToRight)
	{
		return -1;
	}
	if (emMode == s_emAudioCtrlMode[_Channel_AIN_Mux])
	{
		return 0;
	}
	u16Reg &= 0x00BD;
	
	switch(emMode)
	{
		case _Audio_Ctrl_Mode_Normal:
		{
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeftAndRight:
		{
			u16Reg |= 0x0042;
			break;
		}
		default:
			return -1;
	}
	
	if (WM8776Write(_WM_Reg_PWRDownCtrl, u16Reg) != 0)
	{
		return -1;
	}
	s_u16WMReg[_WM_Reg_PWRDownCtrl] = u16Reg;	
#endif	
	s_emAudioCtrlMode[_Channel_AIN_Mux] = emMode;
	return 0;
}

int SetAudioCtrlModeDigitalPC(EmAudioCtrlMode emMode, bool boIsForce)
{
	uint16_t u16Reg = s_u16WMReg[_WM_Reg_DACCtrl1];
	if (emMode >= _Audio_Ctrl_Mode_Reserved)
	{
		return -1;
	}
	if (!boIsForce)
	{
		if (emMode == s_emAudioCtrlMode[_Channel_PC])
		{
			return 0;
		}
	}
	u16Reg &= 0x000F;
	switch(emMode)
	{
		case _Audio_Ctrl_Mode_Normal:
		{
			u16Reg |= 0x0090;
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeft:
		{	
			u16Reg |= 0x0080;
			break;
		}
		case _Audio_Ctrl_Mode_ShieldRight:
		{
			u16Reg |= 0x0010;
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeftAndRight:
		{
			break;
		}
		case _Audio_Ctrl_Mode_LeftToRight:
		{
			u16Reg |= 0x00D0;
			break;
		}
		case _Audio_Ctrl_Mode_RightToLeft:
		{
			u16Reg |= 0x00B0;
			break;
		}
		case _Audio_Ctrl_Mode_Mux:
		{
			u16Reg |= 0x00F0;
			break;
		}
		default:
			return -1;
	}
	
	if (WM8776Write(_WM_Reg_DACCtrl1, u16Reg) != 0)
	{
		return -1;
	}
	s_u16WMReg[_WM_Reg_DACCtrl1] = u16Reg;
	
	s_emAudioCtrlMode[_Channel_PC] = emMode;
	return 0;
}

int SetAudioCtrlModeHeaderPhone(EmAudioCtrlMode emMode, bool boIsForce)
{
	uint16_t u16Reg = s_u16WMReg[_WM_Reg_PWRDownCtrl];
	if (emMode >= _Audio_Ctrl_Mode_LeftToRight)
	{
		return -1;
	}
	if (!boIsForce)
	{
		if (emMode == s_emAudioCtrlMode[_Channel_HeaderPhone])
		{
			return 0;
		}		
	}
	u16Reg &= 0x00F7;
	switch(emMode)
	{
		case _Audio_Ctrl_Mode_Normal:
		{
			break;
		}
		case _Audio_Ctrl_Mode_ShieldLeftAndRight:
		{
			u16Reg |= 0x0008;
			break;
		}

		default:
			return -1;
	}
	
	if (WM8776Write(_WM_Reg_PWRDownCtrl, u16Reg) != 0)
	{
		return -1;
	}
	s_u16WMReg[_WM_Reg_PWRDownCtrl] = u16Reg;
	
	s_emAudioCtrlMode[_Channel_HeaderPhone] = emMode;
	return 0;
}

int SetAudioModeExtern(u8 u8Index, EmAudioCtrlMode emMode, bool boIsForce)
{
	u8 u8RealChannel;
	if (u8Index >= TOTAL_EXTERN_MODE_CTRL)
	{
		return -1;
	}
	
	u8RealChannel = c_u8ExternVolumeChanelToGlobleChannel[u8Index];
	
	if (boIsForce || (emMode != s_emAudioCtrlMode[u8RealChannel]))
	{
		ExternVolumeSetMode(u8Index, emMode);
	}
	
	s_emAudioCtrlMode[u8RealChannel] = emMode;
	
	return 0;
}

int SetAudioModeExternAIN1(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(0, emMode, boIsForce);
}

int SetAudioModeExternAIN2(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(1, emMode, boIsForce);
}

int SetAudioModeExternAIN3(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(2, emMode, boIsForce);
}

int SetAudioModeExternAIN4(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(3, emMode, boIsForce);
}

int SetAudioModeExternAIN5(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(4, emMode, boIsForce);
}

int SetAudioModeExternInnerSpeaker(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(5, emMode, boIsForce);
}

int SetAudioModeExternNormalOut(EmAudioCtrlMode emMode, bool boIsForce)
{
	return SetAudioModeExtern(6, emMode, boIsForce);
}


const PFUN_SetAudioCtrlMode c_pFunSetAudioCtrlMode[TOTAL_MODE_CTRL] = 
{
	SetAudioModeExternAIN1, 	/* AIN 1 */
	SetAudioModeExternAIN2, 	/* AIN 2 */
	SetAudioModeExternAIN3,		/* AIN 3 */
	SetAudioModeExternAIN4,		/* AIN 4 */	
	SetAudioModeExternAIN5,		/* AIN 5 */ 
	SetAudioCtrlModeAinMux,			/* Ain Mux */
	SetAudioCtrlModeDigitalPC,		/* Digital PC */
	SetAudioCtrlModeHeaderPhone,	/* Header Phone */
	SetAudioModeExternInnerSpeaker,	/* Inner Speaker */
	SetAudioModeExternNormalOut,	/* Normal Out */
};

int SetAudioCtrlModeInner(u32 u32Channel, EmAudioCtrlMode emMode, bool boIsForce)
{
	if ((u32Channel >= TOTAL_MODE_CTRL) || (emMode >= _Audio_Ctrl_Mode_Reserved))
	{
		return -1;
	}
	if (c_pFunSetAudioCtrlMode[u32Channel] != NULL)
	{
		return c_pFunSetAudioCtrlMode[u32Channel](emMode, boIsForce);
	}
	return -1;
}
int SetAudioCtrlMode(u32 u32Channel, EmAudioCtrlMode emMode)
{
	return SetAudioCtrlModeInner(u32Channel, emMode, false);
}




void AudioModeCtrlInit(void)
{
	u32 i;
	AudioModePinInit();
	for (i = 0; i < TOTAL_MODE_CTRL; i++)
	{
		SetAudioCtrlModeInner(i, _Audio_Ctrl_Mode_Normal, true);
	}
}





static void AudioVolumeGPIOInit(void)
{
	//GPIO_InitTypeDef GPIO_InitStructure;
}



typedef int(*PFUN_SetAudioVolume)(StVolume stVolume, bool boIsForce);


int SetAudioVolumeAinMux(StVolume stVolume, bool boIsForce)
{
	
	if (boIsForce || (stVolume.u8Channel1 != s_stVolume[_Channel_AIN_Mux].u8Channel1))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_ADCLeftAtt];
		
		u8 u8Tmp = (u32)stVolume.u8Channel1 /* * 207 / 255 */;
		
		u16Reg &= 0xFF00;
		u16Reg |= u8Tmp;

#if 0		
		if ((u8Tmp >= (0xA4 - 8)) && (u8Tmp <= (0xA4 + 8)))
		{
			u16Reg &= (~0x0100);
		}
		else
		{
			u16Reg |= (0x0100);
		}
#endif		
		
		if (WM8776Write(_WM_Reg_ADCLeftAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_ADCLeftAtt] = u16Reg;
		s_stVolume[_Channel_AIN_Mux].u8Channel1 = stVolume.u8Channel1;
	}
	
	if (boIsForce || (stVolume.u8Channel2 != s_stVolume[_Channel_AIN_Mux].u8Channel2))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_ADCRightAtt];

		u8 u8Tmp = (u32)stVolume.u8Channel2 /* * 207 / 255 */;

		u16Reg &= 0xFF00;
		u16Reg |= u8Tmp;
		
#if 0		
		if ((u8Tmp >= (0xA4 - 8)) && (u8Tmp <= (0xA4 + 8)))
		{
			u16Reg &= (~0x0100);
		}
		else
		{
			u16Reg |= (0x0100);
		}
#endif
		if (WM8776Write(_WM_Reg_ADCRightAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_ADCRightAtt] = u16Reg;
		s_stVolume[_Channel_AIN_Mux].u8Channel2 = stVolume.u8Channel2;
	}
	
	return 0;	
}

int SetAudioVolumeDigitalPC(StVolume stVolume, bool boIsForce)
{
	if (stVolume.u8Channel1 == stVolume.u8Channel2)
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_DACMasterAtt];
		u16Reg &= 0xFF00;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= stVolume.u8Channel1;
		if (WM8776Write(_WM_Reg_DACMasterAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_DACMasterAtt] = u16Reg;

		s_stVolume[_Channel_PC].u8Channel2 =
		s_stVolume[_Channel_PC].u8Channel1 = stVolume.u8Channel1;
		
		return 0;
		
	}
	
	if (boIsForce || (stVolume.u8Channel1 != s_stVolume[_Channel_PC].u8Channel1))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_DACLeftAtt];
		u16Reg &= 0xFF00;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= stVolume.u8Channel1;
		if (WM8776Write(_WM_Reg_DACLeftAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_DACLeftAtt] = u16Reg;
		s_stVolume[_Channel_PC].u8Channel1 = stVolume.u8Channel1;
	}
	
	if (boIsForce || (stVolume.u8Channel2 != s_stVolume[_Channel_PC].u8Channel2))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_DACRightAtt];
		u16Reg &= 0xFF00;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= stVolume.u8Channel2;
		if (WM8776Write(_WM_Reg_DACRightAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_DACRightAtt] = u16Reg;
		s_stVolume[_Channel_PC].u8Channel2 = stVolume.u8Channel2;
	}
	
	return 0;	
}

int SetAudioVolumeHeaderPhone(StVolume stVolume, bool boIsForce)
{
	if (stVolume.u8Channel1 == stVolume.u8Channel2)
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_HPMasterAtt];
		u16Reg &= 0xFF80;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= (stVolume.u8Channel1 >> 1);
		if (WM8776Write(_WM_Reg_HPMasterAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_HPMasterAtt] = u16Reg;

		s_stVolume[_Channel_HeaderPhone].u8Channel2 =
		s_stVolume[_Channel_HeaderPhone].u8Channel1 = stVolume.u8Channel1;
		
		return 0;
		
	}
	
	if (boIsForce || (stVolume.u8Channel1 != s_stVolume[_Channel_HeaderPhone].u8Channel1))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_HPLeftAtt];
		u16Reg &= 0xFF80;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= (stVolume.u8Channel1 >> 1);
		if (WM8776Write(_WM_Reg_HPLeftAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_HPLeftAtt] = u16Reg;
		s_stVolume[_Channel_HeaderPhone].u8Channel1 = stVolume.u8Channel1;
	}
	
	if (boIsForce || (stVolume.u8Channel2 != s_stVolume[_Channel_HeaderPhone].u8Channel2))
	{
		uint16_t u16Reg = s_u16WMReg[_WM_Reg_HPRightAtt];
		u16Reg &= 0xFF80;
		u16Reg |= 0x0100; /* lantch */
		u16Reg |= (stVolume.u8Channel2 >> 1);
		if (WM8776Write(_WM_Reg_HPRightAtt, u16Reg) != 0)
		{
			return -1;
		}
		s_u16WMReg[_WM_Reg_HPRightAtt] = u16Reg;
		s_stVolume[_Channel_HeaderPhone].u8Channel2 = stVolume.u8Channel2;
	}
	
	return 0;	
}


int SetAudioVolumeExtern(u8 u8Index, StVolume stVolume, bool boIsForce)
{
	u8 u8Volume = ((u32)stVolume.u8Channel1 + stVolume.u8Channel2) / 2;
	u8 u8RealChannel;
	if (u8Index >= TOTAL_EXTERN_MODE_CTRL)
	{
		return -1;
	}
	
	u8RealChannel = c_u8ExternVolumeChanelToGlobleChannel[u8Index];
	
	if (boIsForce || (stVolume.u8Channel2 != s_stVolume[u8RealChannel].u8Channel2))
	{
		ExternVolumeSetVolume(u8Index, u8Volume);
	}
	
	s_stVolume[u8RealChannel].u8Channel1 = s_stVolume[u8RealChannel].u8Channel2 = u8Volume;
	
	return 0;
}

int SetAudioVolumeAIN1(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(0, stVolume, boIsForce);
}

int SetAudioVolumeAIN2(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(1, stVolume, boIsForce);
}

int SetAudioVolumeAIN3(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(2, stVolume, boIsForce);
}

int SetAudioVolumeAIN4(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(3, stVolume, boIsForce);
}

int SetAudioVolumeAIN5(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(4, stVolume, boIsForce);
}

int SetAudioVolumeInnerSpeaker(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(5, stVolume, boIsForce);
}

int SetAudioVolumeNormalOut(StVolume stVolume, bool boIsForce)
{
	return SetAudioVolumeExtern(6, stVolume, boIsForce);
}



const PFUN_SetAudioVolume c_pFunSetAudioVolume[TOTAL_MODE_CTRL] = 
{
	SetAudioVolumeAIN1, 	/* AIN 1 */
	SetAudioVolumeAIN2, 	/* AIN 2 */
	SetAudioVolumeAIN3,		/* AIN 3 */
	SetAudioVolumeAIN4,		/* AIN 4 */	
	SetAudioVolumeAIN5,		/* AIN 5 */ 
	SetAudioVolumeAinMux,			/* Ain Mux */
	SetAudioVolumeDigitalPC,		/* Digital PC */
	SetAudioVolumeHeaderPhone,		/* Header Phone */
	SetAudioVolumeInnerSpeaker,		/* Inner Speaker */
	SetAudioVolumeNormalOut,		/* Normal Out */
};
int SetAudioVolumeInner(u32 u32Channel, StVolume stVolume, bool boIsForce)
{
	if (u32Channel >= TOTAL_VOLUME_CHANNEL)
	{
		return -1;
	}
	if (c_pFunSetAudioVolume[u32Channel] != NULL)
	{
		return c_pFunSetAudioVolume[u32Channel](stVolume, boIsForce);
	}

	return -1;

}

int SetAudioVolume(u32 u32Channel, StVolume stVolume)
{
	return SetAudioVolumeInner(u32Channel, stVolume, false);
}

int SetAudioVolumeGradient(u32 u32Channel, StVolume stVolume)
{
	if (u32Channel >= TOTAL_VOLUME_CHANNEL)
	{
		return -1;
	}
	
	memcpy(s_stVolumeGradient + u32Channel, &stVolume, sizeof(StVolume));

	return 0;
}

void AudioVolumeGradientFlush(void)
{
	static u32 u32TimeCur = 0;
	if (SysTimeDiff(u32TimeCur, g_u32SysTickCnt) > 10)
	{
		u32 i;
		u32TimeCur = g_u32SysTickCnt;
		for (i = 0; i < TOTAL_VOLUME_CHANNEL; i++)
		{
			StVolume stVolume = {0, 0};
			GetAudioVolume(i, &stVolume);
			if (memcmp(&stVolume, s_stVolumeGradient + i, sizeof(StVolume)) != 0)
			{
				if (stVolume.u8Channel1 > s_stVolumeGradient[i].u8Channel1)
				{
					stVolume.u8Channel1--;
				}
				else if (stVolume.u8Channel1 < s_stVolumeGradient[i].u8Channel1)
				{
					stVolume.u8Channel1++;
				}
				
				if (stVolume.u8Channel2 > s_stVolumeGradient[i].u8Channel2)
				{
					stVolume.u8Channel2--;
				}
				else if (stVolume.u8Channel2 < s_stVolumeGradient[i].u8Channel2)
				{
					stVolume.u8Channel2++;
				}				
				SetAudioVolume(i, stVolume);
			}
		}
	}
}


int GetAudioVolume(u32 u32Channel, StVolume *pVolume)
{
	if ((u32Channel >= TOTAL_VOLUME_CHANNEL) || (pVolume == NULL))
	{
		return -1;
	}
	memcpy(pVolume, s_stVolume + u32Channel, sizeof(StVolume));
	return 0;
}


void GetAllAudioVolume(StVolume stVolume[TOTAL_VOLUME_CHANNEL])
{
	int s32Size = sizeof(StVolume) * TOTAL_VOLUME_CHANNEL;
	memcpy(stVolume, s_stVolume, s32Size);
}
void SetAllAudioVolume(StVolume stVolume[TOTAL_VOLUME_CHANNEL])
{
	int i;
	for (i = 0; i < TOTAL_VOLUME_CHANNEL; i++)
	{
		SetAudioVolume(i, stVolume[i]);
		SetAudioVolumeGradient(i, stVolume[i]);
	}
}


void AudioVolumeInit(void)
{
	u32 i;
	
	AudioVolumeGPIOInit();
	for (i = 0; i < TOTAL_VOLUME_CHANNEL; i++)
	{
		SetAudioVolumeInner(i, s_stVolume[i], true);
	}
}




typedef struct _tagStPowerCtrl
{
	StPinSource	stPin;
	bool boCurPowerState;
	bool boExpectPowerState;
}StPowerCtrl;

static StPowerCtrl s_stPowerCtrl[FANTASY_POWER_CTRL] = 
{
	{{GPIOC, GPIO_Pin_11}, false, false},
	{{GPIOC, GPIO_Pin_10}, false, false},
	//{{GPIOC, GPIO_Pin_7}, false, false},
	//{{GPIOC, GPIO_Pin_6}, false, false},
};

static void FantasyPowerPinInit(void)
{
	int i;
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	
	for (i = 0; i < FANTASY_POWER_CTRL; i++)
	{
		GPIO_WriteBit(s_stPowerCtrl[i].stPin.pPort, s_stPowerCtrl[i].stPin.u16Pin, Bit_RESET);
		GPIO_InitStructure.GPIO_Pin = s_stPowerCtrl[i].stPin.u16Pin;
		GPIO_Init(s_stPowerCtrl[i].stPin.pPort, &GPIO_InitStructure);	
		GPIO_WriteBit(s_stPowerCtrl[i].stPin.pPort, s_stPowerCtrl[i].stPin.u16Pin, Bit_RESET);
	}

}



void PowerPWMFlush(void)
{
}


int SetFantasyPowerState(u32 u32Channel, bool boIsEnable)
{
	if (u32Channel >= FANTASY_POWER_CTRL)
	{
		return -1;
	}
	
	s_stPowerCtrl[u32Channel].boExpectPowerState = boIsEnable;
	/* <TODO> open the 48V power */
	if (boIsEnable != s_stPowerCtrl[u32Channel].boCurPowerState)
	{
		GPIO_WriteBit(s_stPowerCtrl[u32Channel].stPin.pPort, 
			s_stPowerCtrl[u32Channel].stPin.u16Pin, boIsEnable ? Bit_SET : Bit_RESET);		
		s_stPowerCtrl[u32Channel].boCurPowerState = boIsEnable;
	}
	
	return 0;
}


int GetFantasyPowerState(u32 u32Channel, bool *pState)
{
	if ((u32Channel >= FANTASY_POWER_CTRL) || (pState == NULL))
	{
		return -1;
	}
	
	*pState = s_stPowerCtrl[u32Channel].boExpectPowerState;
	
	return 0;
}


void GetAllFantasyPowerState(bool boState[FANTASY_POWER_CTRL])
{
	int i;
	for (i = 0; i < FANTASY_POWER_CTRL; i++)
	{
		GetFantasyPowerState(i, boState + i);
	}	
}

void SetAllFantasyPowerState(bool boState[FANTASY_POWER_CTRL])
{
	int i;
	for (i = 0; i < FANTASY_POWER_CTRL; i++)
	{
		SetFantasyPowerState(i, boState[i]);
	}	
}

void FantasyPowerInit(void)
{
	u32 i;

	FantasyPowerPinInit();
	
	
	for (i = 0; i < FANTASY_POWER_CTRL; i++)
	{
		SetFantasyPowerState(i, false);
	}
}


void LoadMemoryToDevice(void *pMemory)
{
	StMemory *pMem = pMemory;
	if(pMem == NULL)
	{
		return;
	}
	SetAllAudioCtrlMode(pMem->emAudioCtrlMode);
	SetAllAudioVolume(pMem->stVolume);
	//SetAllFantasyPowerState(pMem->boFantasyPower);
	WM8776EnableAINChannel(pMem->u8AINChannelEnableState);
	WM8776EnableOutputChannel(pMem->u8OutputChannelEnableState);
	
}
void SaveMemoryFromDevice(void *pMemory)
{

	StMemory *pMem = pMemory;
	if(pMem == NULL)
	{
		return;
	}
	
	GetAllAudioCtrlMode(pMem->emAudioCtrlMode);
	GetAllAudioVolume(pMem->stVolume);
	GetAllFantasyPowerState(pMem->boFantasyPower);
	pMem->u8AINChannelEnableState = WM8776GetAINChannelEnableState();
	pMem->u8OutputChannelEnableState = WM8776GetOutputChannelEnableState();
	
}


void LoadPowerOffMemoryToDevice(void)
{
	LoadMemoryToDevice(&(g_stSave.stPowerOffMemory));
}

void LoadFactoryMemoryToDevice(void)
{
	LoadMemoryToDevice(&(g_stSave.stFactoryMemory));	
}


void PowerOffMemoryFlush(void)	/* 100ms flush */
{
	static StMemory StMemMonitor = {0};
	static StMemory StMemCur = {0};
	static bool boIsGetMonitor = false;
	static u32 u32GetMonitorTime = 0;
	static u32 u32MonitorTimeInterval = 0;
	
	if (SysTimeDiff(u32MonitorTimeInterval, g_u32SysTickCnt) < 100)
	{
		return;
	}
	u32MonitorTimeInterval = g_u32SysTickCnt;

	memset(&StMemCur, 0, sizeof(StMemory));
	SaveMemoryFromDevice(&StMemCur);
	if (memcmp(&StMemMonitor, &StMemCur, sizeof(StMemory)) != 0)
	{
		/* begin to monitor */
		memcpy(&StMemMonitor, &StMemCur, sizeof(StMemory));
		boIsGetMonitor = true;
		u32GetMonitorTime = g_u32SysTickCnt;
	}
	if (boIsGetMonitor)
	{
		if (SysTimeDiff(u32GetMonitorTime, g_u32SysTickCnt) > 15 * 1000)	/* 15S unchanged */
		{
			/* prevent to write flash after power on */
			if (memcmp(&StMemMonitor, &g_stSave.stPowerOffMemory, sizeof(StMemory)) != 0)
			{
				memcpy(&(g_stSave.stPowerOffMemory), &StMemMonitor, sizeof(StMemory));
				WriteSaveData();
			}
			boIsGetMonitor = false;
		}
	}
	

}



