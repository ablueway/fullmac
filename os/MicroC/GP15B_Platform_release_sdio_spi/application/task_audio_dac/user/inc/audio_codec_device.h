#ifndef __AUDIO_CODEC_DEVICE_H__
#define __AUDIO_CODEC_DEVICE_H__
#include "application.h"

#define CODEC_W8988			1	

typedef struct AudCodecDev_s
{
	INT32S (*init)(void);
	void   (*uninit)(void);
	INT32S (*start)(INT32U mode);
	void   (*stop)(void);
	void   (*adjust_volume)(INT32U volume);
} AudCodecDev_t;

extern const AudCodecDev_t AudCodec_device;
#endif