#ifndef __TURNKEY_AUDIO_DAC_TASK_H__
#define __TURNKEY_AUDIO_DAC_TASK_H__
#include "application.h"

// api function
extern void *audio_out_open(AUDIO_CHANNEL_TYPE type);
extern void audio_out_close(void *workmem);
extern INT32S audio_out_send_ready_buf(void *workmem, INT16S *pcm_addr, INT32U pcm_cwlen);
extern INT32S audio_out_send_empty_buf(void *workmem, INT32U pcm_addr);
extern INT16S *audio_out_get_empty_buf(void *workmem);
extern INT32S audio_out_get_ready_buf(void *workmem, INT32U *pcm_addr, INT32U *pcm_cwlen);
extern INT32S audio_out_init(void *workmem, INT32U pcm_size);
extern INT32S audio_out_start(void *workmem, INT32U channle_no, INT32U sample_rate);
extern INT32S audio_out_pause(void *workmem);
extern INT32S audio_out_resume(void *workmem);
extern INT32S audio_out_volume(void *workmem, INT32U volume);
extern INT32S audio_out_stop(void *workmem);
extern INT16S *audio_out_prepare_ramp_down(void *workmem, INT32U *cwlen);
#endif //__TURNKEY_AUDIO_DAC_TASK_H__
