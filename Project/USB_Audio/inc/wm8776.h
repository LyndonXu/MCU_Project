/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：wm8776.c
* 摘要: WM8776控制源文件
* 版本：0.0.1
* 作者：许龙杰
* 日期：2017年11月29日
*******************************************************************************/

#ifndef _WM8776_H_
#define _WM8776_H_
#include "stdbool.h"
#include "user_api.h"



enum
{
/*  0 */		_WM_Reg_HPLeftAtt = 0,		
/*  1 */		_WM_Reg_HPRightAtt,			
/*  2 */		_WM_Reg_HPMasterAtt,
/*  3 */		_WM_Reg_DACLeftAtt,
/*  4 */		_WM_Reg_DACRightAtt,
/*  5 */		_WM_Reg_DACMasterAtt,
/*  6 */		_WM_Reg_PHASE,
/*  7 */		_WM_Reg_DACCtrl1,
/*  8 */		_WM_Reg_DACMute,
/*  9 */		_WM_Reg_DACCtrl2,
/* 10 */		_WM_Reg_DACInterfaceCtrl,
/* 11 */		_WM_Reg_ADCInterfaceCtrl,
/* 12 */		_WM_Reg_MasterModeCtrl,
/* 13 */		_WM_Reg_PWRDownCtrl,
/* 14 */		_WM_Reg_ADCLeftAtt,
/* 15 */		_WM_Reg_ADCRightAtt,
/* 16 */		_WM_Reg_ALCCtrl1,
/* 17 */		_WM_Reg_ALCCtrl2,
/* 18 */		_WM_Reg_ALCCtrl3,
/* 19 */		_WM_Reg_NoiseGateCtrl,
/* 20 */		_WM_Reg_LimiterCtrl,
/* 21 */		_WM_Reg_ADCMuxCtrl,
/* 22 */		_WM_Reg_OutputMux,
/* 23 */		_WM_Reg_SoftwareReset,
				_WM_Reg_Reserved,
};

enum
{
	_Channel_AIN_1,
	_Channel_AIN_2,
	_Channel_AIN_3,
	_Channel_AIN_4,
	_Channel_AIN_5,
	_Channel_AIN_Mux,
	_Channel_PC,
	_Channel_HeaderPhone,
	_Channel_InnerSpeaker,
	_Channel_NormalOut,

};



void WM8776Init(void);
int WM8776EnableAINChannel(u8 u8Channel);
int WM8776EnableOutputChannel(u8 u8Channel);
u8 WM8776GetAINChannelEnableState(void);
u8 WM8776GetOutputChannelEnableState(void);


#ifndef TOTAL_MODE_CTRL_IN
#define TOTAL_MODE_CTRL_IN		7
#endif

#ifndef TOTAL_MODE_CTRL_OUT
#define TOTAL_MODE_CTRL_OUT		3
#endif

#ifndef TOTAL_MODE_CTRL
#define TOTAL_MODE_CTRL			10
#endif

#ifndef TOTAL_EXTERN_MODE_CTRL
#define TOTAL_EXTERN_MODE_CTRL		7
#endif

typedef enum _tagEmAudioCtrlMode
{
	_Audio_Ctrl_Mode_Normal = 0,	/* left to left, right to right */
	_Audio_Ctrl_Mode_ShieldLeft,
	_Audio_Ctrl_Mode_ShieldRight,
	_Audio_Ctrl_Mode_ShieldLeftAndRight,
	_Audio_Ctrl_Mode_LeftToRight,
	_Audio_Ctrl_Mode_RightToLeft,
	_Audio_Ctrl_Mode_Mux,
	
	_Audio_Ctrl_Mode_Reserved,	
}EmAudioCtrlMode;


typedef struct _tagStModeCtrl
{
	StPinSource stChannel1;
	StPinSource stChannel2;
	StPinSource stSourceGND;
	StPinSource stCopy;
}StModeCtrl;


void AudioModeCtrlInit(void);
int GetAudioCtrlMode(u32 u32Channel, EmAudioCtrlMode *pMode);
int SetAudioCtrlMode(u32 u32Channel, EmAudioCtrlMode emMode);
void GetAllAudioCtrlMode(EmAudioCtrlMode emAudioCtrlMode[TOTAL_MODE_CTRL]);
void SetAllAudioCtrlMode(EmAudioCtrlMode emAudioCtrlMode[TOTAL_MODE_CTRL]);


#ifndef TOTAL_VOLUME_CHANNEL
#define TOTAL_VOLUME_CHANNEL 			TOTAL_MODE_CTRL
#endif


#ifndef TOTAL_EXTERN_VOLUME_CHANNEL
#define TOTAL_EXTERN_VOLUME_CHANNEL 			TOTAL_EXTERN_MODE_CTRL
#endif

#define CHANNEL_ALL		0x00
#define CHANNEL_1		0x02
#define CHANNEL_2		0x03

typedef struct _tagStVolume
{
	u8 u8Channel1;
	u8 u8Channel2;
}StVolume;

void AudioVolumeInit(void);
void VolumeWriteData(u32 u32Channel, u16 u16Data);
int SetAudioVolume(u32 u32Channel, StVolume stVolume);
int GetAudioVolume(u32 u32Channel, StVolume *pVolume);
void GetAllAudioVolume(StVolume stVolume[TOTAL_VOLUME_CHANNEL]);
void SetAllAudioVolume(StVolume stVolume[TOTAL_VOLUME_CHANNEL]);

int SetAudioVolumeGradient(u32 u32Channel, StVolume stVolume);
void AudioVolumeGradientFlush(void);
void FlushExternVolumeCmd(void);


#ifndef FANTASY_POWER_CTRL
#define FANTASY_POWER_CTRL		4
#endif


int GetFantasyPowerState(u32 u32Channel, bool *pState);
int SetFantasyPowerState(u32 u32Channel, bool boIsEnable);
void FantasyPowerInit(void);
void GetAllFantasyPowerState(bool boState[FANTASY_POWER_CTRL]);
void SetAllFantasyPowerState(bool boState[FANTASY_POWER_CTRL]);
void PowerPWMFlush(void);




void LoadMemoryToDevice(void *pMemory);
void SaveMemoryFromDevice(void *pMemory);


void LoadFactoryMemoryToDevice(void);

void LoadPowerOffMemoryToDevice(void);
void PowerOffMemoryFlush(void);	/* 100ms flush */

#endif
