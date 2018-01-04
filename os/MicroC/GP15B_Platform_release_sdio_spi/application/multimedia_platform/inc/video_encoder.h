#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "application.h"

extern void video_encode_entrance(void);
extern void video_encode_exit(void);
extern CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg);
extern CODEC_START_STATUS video_encode_preview_stop(void);
extern CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src);
extern CODEC_START_STATUS video_encode_stop(void);
extern CODEC_START_STATUS video_encode_Info(VIDEO_INFO *info);
extern VIDEO_CODEC_STATUS video_encode_status(void);
extern CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio);
extern CODEC_START_STATUS video_encode_set_jpeg_quality(INT8U quality_value);
extern CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src);
extern CODEC_START_STATUS video_encode_capture_size(MEDIA_SOURCE src, INT8U quality, INT16U width, INT16U height);
extern CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src);

extern void video_encode_register_display_callback(INT32U (*disp)(INT16U, INT16U, INT32U));
extern void video_decode_register_user_write_callback(INT32S (*put_data)(void* , unsigned long , long , const void *, int , int));
#endif