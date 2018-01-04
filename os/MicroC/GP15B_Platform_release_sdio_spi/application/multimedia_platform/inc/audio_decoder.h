#ifndef __AUDIO_DECODER_H__
#define __AUDIO_DECODER_H__

#include "application.h"

extern void audio_decode_entrance(void);						
extern void audio_decode_exit(void);							

extern CODEC_START_STATUS audio_decode_parser_start(AUDIO_ARGUMENT arg, MEDIA_SOURCE src, AUDIO_INFO *audio_info);
extern CODEC_START_STATUS audio_decode_parser_stop(AUDIO_ARGUMENT arg);

extern CODEC_START_STATUS audio_decode_start(AUDIO_ARGUMENT arg);
extern void audio_decode_pause(AUDIO_ARGUMENT arg);
extern void audio_decode_resume(AUDIO_ARGUMENT arg);
extern void audio_decode_stop(AUDIO_ARGUMENT arg);
extern CODEC_START_STATUS audio_decode_seek_play(AUDIO_ARGUMENT arg, INT32U sec);

extern void audio_decode_volume_set(AUDIO_ARGUMENT arg, INT32S volume);
extern void audio_decode_mute(AUDIO_ARGUMENT arg);

extern AUDIO_CODEC_STATUS audio_decode_status(AUDIO_ARGUMENT arg);
extern INT32U audio_decode_get_total_time(AUDIO_ARGUMENT arg);
extern INT32U audio_decode_get_cur_time(AUDIO_ARGUMENT arg);

extern void audio_decode_register_user_read_callback(INT32S (*user_read)(INT32U, INT32U, INT32U, INT8U *, INT32U));
extern void audio_decode_register_end_callback(void (*end)(INT32U));
#endif