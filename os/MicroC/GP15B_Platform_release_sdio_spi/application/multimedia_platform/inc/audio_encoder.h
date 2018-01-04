#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include "application.h"
#include "audio_record.h"

extern void audio_encode_entrance(void);
extern void audio_encode_exit(void);
extern CODEC_START_STATUS audio_encode_start(MEDIA_SOURCE src, INT32U channle_no, INT16U sample_rate, INT32U bit_rate);
extern void audio_encode_stop(void);
extern AUDIO_CODEC_STATUS audio_encode_status(void);
extern CODEC_START_STATUS audio_encode_set_downsample(INT8U bEnable, INT8U DownSampleFactor);

extern void audio_encode_register_user_write_callback(INT32S (*user_write)(INT32U, INT8U , INT32U , INT32U));
#endif