#ifndef __VIDENC_STATE_H__
#define __VIDENC_STATE_H__

#include "videnc_cfg.h"
#include "application.h"

/* video encode video format */
#define C_XVID_FORMAT		0x44495658
#define C_MJPG_FORMAT		0x47504A4D

/* video encode status */
#define C_STS_STOP			0
#define C_STS_PREVIEW		1
#define C_STS_RECORD		2

typedef struct VidEncParam_s
{
	INT8U source_type;
	INT8U channel_no;
	INT16U sample_rate;
	INT32U audio_format;
	
	INT32U video_format;
	INT16U frame_rate;
	INT16U quality_value;
	INT32U display_format;
	
	INT16U sensor_w; 
	INT16U sensor_h;
	INT16U disp_w;
	INT16U disp_h;
	INT16U target_w; 
	INT16U target_h;
} VidEncParam_t;

extern void *videnc_open(void);
extern void videnc_close(void *workmem);
extern void videnc_register_disp_fun(INT32U (*disp)(INT16U, INT16U, INT32U));

extern void *videnc_packer_start(void *workmem, INT16S fd, INT32U task_prio);
extern INT32S videnc_packer_stop(void *packer_workmem);

extern INT32S videnc_preview_start(void *workmem, VidEncParam_t *param);
extern INT32S videnc_preview_stop(void *workmem);

extern INT32S videnc_encode_start(void *workmem);
extern INT32S videnc_encode_stop(void *workmem);

extern INT32S videnc_get_status(void *workmem);
extern INT32S videnc_set_zoom(void *workmem, FP32 ZoomFactor);
extern INT32S videnc_capture_start(void *workmem,INT16S fd);
extern INT32S videnc_scaler_up_capture_start(void *workmem,INT16S fd);
extern INT32S wrap_save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size);

#endif //__VIDENC_STATE_H__