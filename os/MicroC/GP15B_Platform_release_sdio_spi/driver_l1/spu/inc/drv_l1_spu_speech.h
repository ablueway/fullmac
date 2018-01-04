#ifndef _DRV_L1_SPU_SPEECH_H_
#define _DRV_L1_SPU_SPEECH_H_

#include "driver_l1.h"

#define C_ChannelNo          8

// User must assign a correct I2S LRCLK when SPU output sample rate is controlled by I2S (0xD0000040[0]=1)
#define C_I2S_SAMPLE_RATE	48000 

#define D_SpeechPlaying		0x08
#define D_SpeechRepeat		0x04
#define D_SpeechPause		0x02
#define D_SpeechZCJump		0x01

#define C_ADPCM_4bit		0x00
#define C_ADPCM_5bit		0x40
#define C_PCM_8bit			0x80
#define C_PCM_16bit			0xC0

extern void SPU_Speech_Play(INT8U* R_SPU_Speech_ResIndex, INT8U SpuCh);
extern void SPU_Speech_GetHeaderData(INT8U* R_SPU_Speech_ResIndex, INT8U SpuCh);
extern INT8U SPU_Speech_GetStatus(INT8U SpuCh);
extern void SPU_Speech_Pause(INT8U SpuCh);
extern void SPU_Speech_Resume(INT8U SpuCh);
extern void SPU_Speech_Stop(INT8U SpuCh);
extern void SPU_Speech_SetVol(INT8U SpuCh, INT8U SpuVolume);
extern void SPU_Speech_RepeatTimes(INT8U SpuCh, INT32U RepeatNums);
extern void SPU_Speech_AlwaysRepeat(INT8U SpuCh);
extern void SPU_Speech_StopRepeat(INT8U SpuCh);
extern void SPU_Speech_NextFrameAddrSet(INT8U * R_SPU_Speech_NextFrameAddr, INT8U SpuCh);
extern void SPU_Speech_Register_Callback(INT32S (*end)(INT8U ch));
#endif