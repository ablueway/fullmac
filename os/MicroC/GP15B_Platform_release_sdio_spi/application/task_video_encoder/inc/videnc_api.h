#ifndef __VIDENC_API_H__
#define __VIDENC_API_H__

#include "application.h"
#include "drv_l2_sensor.h"
#include "videnc_cfg.h"
#include "AviPackerV3.h"
#include "wav_dec.h"

#define VIDENC_QUEUE_MAX		8
#define VidEncTaskStackSize		1024

// video encode state
#define STATE_PREVIEW			0
#define STATE_CHANGE_ENCODE		1
#define STATE_ENCODE			2
#define STATE_CHANGE_PREVIEW	3
#define STATE_CHANGE_STOP		4
#define STATE_STOP				5

// packer state
#define STATE_PACKER_STOP		0
#define STATE_PACKER_START		1

// scale and conv422 done flag
#define SCALER_DONE				(1 << 0)
#define CONV422_DONE			(1 << 1)

// flag
#define FLAG_FRAME_END			(1 << 0)

// yuv format
#define JPEG_YUV422				0
#define JPEG_GPYUV420			1

#if 1
	#define MSG		DBG_PRINT
#else
	#define MSG(...)
#endif

typedef struct VidBuf_s
{
	INT32U keyflag;
	INT32U addr;
	INT32U size;
} VidBuf_t;

typedef struct PackerCtrl_s
{
	void *WorkMem;
	INT16S fd;
	INT16S index_fd;
	INT8U index_path[32];

	INT8U state;
	INT8U task_prio;
	INT8U rsd0;
	INT8U rsd1;

	// video info
	GP_AVI_AVISTREAMHEADER VidStreamHeader;
	GP_AVI_BITMAPINFO BitmapInfo;
	INT32U BitmapInfoLen;

	// audio info
	GP_AVI_AVISTREAMHEADER AudStreamHeader;
	GP_AVI_PCMWAVEFORMAT WaveInfo;
	INT32U WaveInfoLen;		// wave header length

	void *FileWriteBuf;
	void *IndexWriteBuf;
	INT32U FileWriteBufSize;		// AviPacker file buffer size in byte
	INT32U IndexWriteBufSize;		// AviPacker index buffer size in byte    
} PackerCtrl_t;

typedef struct VidEncCtrl_s
{
	OS_EVENT *VidEncQ;
	OS_EVENT *VidEncM;
	OS_EVENT *DispQ;
	OS_EVENT *DispM;
	OS_EVENT *VideoQ;
	OS_EVENT *VideoM;
	OS_EVENT *AudioQ;
	OS_EVENT *AudioM;

	OS_EVENT *DispBufQ;
	OS_EVENT *FifoBufQ;
	OS_EVENT *VidEmptyBufQ;
	OS_EVENT *VidReadyBufQ;
	
	void *videnc_q_buf[VIDENC_QUEUE_MAX];
	void *disp_q_buf[VIDENC_QUEUE_MAX];
	void *video_q_buf[VIDENC_QUEUE_MAX];
	void *audio_q_buf[VIDENC_QUEUE_MAX];
	void *dispbuf_q_buf[DISP_BUF_NUM];
	void *fifobuf_q_buf[FIFO_BUF_NUM];
	void *vidempty_q_buf[VIDEO_BUF_NUM];
	void *vidready_q_buf[VIDEO_BUF_NUM];
	
	INT32U state_task_stack[VidEncTaskStackSize];
	INT32U disp_task_stack[VidEncTaskStackSize];
	INT32U video_task_stack[VidEncTaskStackSize];
	INT32U audio_task_stack[VidEncTaskStackSize];
	
	INT8U state_task_running;
	INT8U disp_task_running;
	INT8U video_task_running;
	INT8U audio_task_running;
	
	// process state
	volatile INT8U state;
	volatile INT8U done_flag;
	INT8U csi_input;
	INT8U Qvalue;
	
	// information
	INT8U source_type;	// SOURCE_TYPE_FS or SOURCE_TYPE_USER_DEFINE
	INT8U channel_no;	// 1: mono, 2: stereo
	INT16U sample_rate;	// sample rate 
	INT32U audio_format;// audio format
	
	INT32U video_format;
	INT16U dwScale;
	INT16U dwRate;
	
	// allow record size
	INT64U disk_free_size;			// disk free size
	INT32U record_total_size;		// AviPacker storage to disk total size
	
	// aud & vid sync
	INT64S delta_ta, tick;
	INT64S delta_Tv;
	INT32S freq_div;
	INT64S ta, tv, Tv;
	INT32U pend_cnt, post_cnt;
	
	// zoom 
	INT32U DigiZoomEn;
	INT32U xfactor[2];
	INT32U yfactor[2];
	INT32U xoffset[2];
	INT32U yoffset[2];
	
	// image format
	INT32U csi_mux_fmt;
	INT32U disp_fmt;
	INT32U conv422_fmt;
	
	// image size
	INT16U sensor_w;
	INT16U sensor_h;
	INT16U disp_w;
	INT16U disp_h;
	INT16U target_w;
	INT16U target_h;
	INT16U fifo_line_num;
	INT16U fifo_buff_num;
	
	// video buffer
	INT32U disp_cur_buf;
	INT32U fifo_a_buf;
	INT32U fifo_b_buf;
	INT32U disp_buf[DISP_BUF_NUM];
	INT32U fifo_buf[FIFO_BUF_NUM];
	VidBuf_t video_buf[VIDEO_BUF_NUM];
	
	// sensor device
	drv_l2_sensor_ops_t *SensorDev;
	
	// avi packer workmem
	PackerCtrl_t *PackerCtrl;
	
	//photo capture
	INT16S fd;
	INT16U snap_delay;	// 10ms a step by using OSTimeDly()
} VidEncCtrl_t;

typedef struct MJpegPara_s
{
	INT32U jpeg_status;
	INT16U Qvalue;
	INT16U yuv_sample_fmt;
	INT16U input_width;
	INT16U input_height;
	INT32U input_buffer;
	INT32U input_len;
	INT32U output_buffer;
	INT32U encode_size;
} MJpegPara_t;

extern INT32S videnc_send_msg(OS_EVENT *msg_queue, OS_EVENT *ack_mbox, INT32U message);
extern INT32S videnc_packer_err_handle(INT32S ErrCode);
extern void videnc_sync_timer_start(void);
extern void videnc_sync_timer_stop(void);
extern INT32S videnc_memory_allocate(VidEncCtrl_t *pVidEnc);
extern void videnc_memory_free(VidEncCtrl_t *pVidEnc);

extern INT32S videnc_sensor_init(VidEncCtrl_t *pVidEnc, CHAR *SensorName);
extern INT32S videnc_sensor_on(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_sensor_off(VidEncCtrl_t *pVidEnc);

extern INT32S videnc_preview_on(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_preview_off(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_encode_on(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_encode_off(VidEncCtrl_t *pVidEnc);

extern INT32S videnc_state_one_frame(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_state_error(VidEncCtrl_t *pVidEnc);
extern void disp_send_ready_buf(VidEncCtrl_t *pVidEnc, INT32U disp_ready_buf);

extern INT32U videnc_jpeg_encode_once(MJpegPara_t *pJpegEnc);
extern INT32S videnc_jpeg_encode_fifo_start(MJpegPara_t *pJpegEnc);
extern INT32S videnc_jpeg_encode_fifo_once(MJpegPara_t *pJpegEnc);
extern INT32S videnc_jpeg_encode_fifo_stop(MJpegPara_t *pJpegEnc);
extern INT32S videnc_jpeg_encode_stop(void);

static void scaler_wrap_path_restart(void);

extern INT32S videnc_scaler0up_on(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_scaler0up_off(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_fifo_to_framebuf(VidEncCtrl_t *pVidEnc);
extern INT32S videnc_fifo_to_framebuf_off(VidEncCtrl_t *pVidEnc);


#endif //__VIDENC_STATE_H__