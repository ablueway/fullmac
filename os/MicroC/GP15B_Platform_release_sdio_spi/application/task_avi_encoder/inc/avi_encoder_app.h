#ifndef __AVI_ENCODER_APP_H__
#define __AVI_ENCODER_APP_H__

#include "application.h"
#include "avi_encoder_scaler_jpeg.h"
#include "AviPackerV3.h"
#include "wav_dec.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// avi encode status
#define C_AVI_ENCODE_IDLE       0x00000000
#define C_AVI_ENCODE_PACKER0	0x00000001
#define C_AVI_ENCODE_AUDIO      0x00000002
#define C_AVI_ENCODE_VIDEO      0x00000004
#define C_AVI_ENCODE_SCALER     0x00000008
#define C_AVI_ENCODE_SENSOR     0x00000010
#define C_AVI_ENCODE_PACKER1	0x00000020
#define C_AVI_ENCODE_MD			0x00000040

#define C_AVI_ENCODE_START      0x00000100
#define C_AVI_ENCODE_PRE_START	0x00000200
#define C_AVI_ENCODE_JPEG		0x00000400
#define C_AVI_ENCODE_ERR		0x20000000
#define C_AVI_ENCODE_FRAME_END	0x40000000
#define C_AVI_ENCODE_FIFO_ERR	0x80000000

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/ 
typedef struct AviEncAudPara_s
{
	//audio encode
	INT32U  audio_format;			// audio encode format
	INT16U  audio_sample_rate;		// sample rate
	INT16U  channel_no;				// channel no, 1:mono, 2:stereo
	INT8U   *work_mem;				// wave encode work memory 
	INT32U  pack_size;				// wave encode package size in byte
	INT32U  pack_buffer_addr;
	INT32U  pcm_input_size;			// wave encode pcm input size in short
	INT32U  pcm_input_addr[AVI_ENCODE_PCM_BUFFER_NO];
} AviEncAudPara_t;

typedef struct AviEncVidPara_s
{
	//sensor input 
    INT32U  sensor_output_format;   // sensor output format
    INT16U  sensor_capture_width;   // sensor width
    INT16U  sensor_capture_height;  // sensor height
    INT32U  csi_frame_addr[AVI_ENCODE_CSI_BUFFER_NO];	// sensor input buffer addr
    INT32U  csi_fifo_addr[AVI_ENCODE_CSI_FIFO_NO];		// sensor fifo mode use
    
    //scaler output
    INT16U 	scaler_flag;
    INT16U 	dispaly_scaler_flag;  	// 0: not scaler, 1: scaler again for display 
    FP32    scaler_zoom_ratio;      // for zoom scaler   
    INT32U  scaler_output_addr[AVI_ENCODE_SCALER_BUFFER_NO];	// scaler output buffer addr
      								
    //display scaler
    INT32U  display_output_format;  // for display use
    INT16U  display_width;          // display width
    INT16U  display_height;         // display height
    INT16U  display_buffer_width;   // display width
    INT16U  display_buffer_height;  // display height 
    INT32U  display_output_addr[AVI_ENCODE_DISPALY_BUFFER_NO];	// display scaler buffer addr
    
    //video encode
    INT32U  video_format;			// video encode format
    INT8U   dwScale;				// dwScale
    INT8U   dwRate;					// dwRate 
    INT16U  quality_value;			// video encode quality
    INT16U  encode_width;           // video encode width
    INT16U  encode_height;          // videoe ncode height
    INT32U  video_encode_addr[AVI_ENCODE_VIDEO_BUFFER_NO]; //video encode buffer
} AviEncVidPara_t;

typedef struct AviEncPacker_s
{
	void 	*avi_workmem;
	INT16S  file_handle;
    INT16S  index_handle;
    INT8U   index_path[16];
    
    //avi video info
    GP_AVI_AVISTREAMHEADER *p_avi_vid_stream_header;
    INT32U  bitmap_info_cblen;		// bitmap header length
    GP_AVI_BITMAPINFO *p_avi_bitmap_info;
    
    //avi audio info
    GP_AVI_AVISTREAMHEADER *p_avi_aud_stream_header;
    INT32U  wave_info_cblen;		// wave header length
    GP_AVI_PCMWAVEFORMAT *p_avi_wave_info;
    
    //avi packer 
	INT32U  task_prio;
	void    *file_write_buffer;
	INT32U  file_buffer_size;		// AviPacker file buffer size in byte
	void    *index_write_buffer;
	INT32U	index_buffer_size;		// AviPacker index buffer size in byte
} AviEncPacker_t;

typedef struct VidEncFrame_s
{
	INT32U ready_frame;
	INT32U encode_size;
	INT32U key_flag;
} VidEncFrame_t;

typedef struct AviEncPara_s
{   
	INT8U  source_type;				// SOURCE_TYPE_FS or SOURCE_TYPE_USER_DEFINE
    INT8U  skip_cnt;
    INT8U  fifo_enc_err_flag;
    INT8U  fifo_irq_cnt;
    INT8U  *memptr;					// capture jpeg file for SOURCE_TYPE_USER_DEFINE
    
    INT8U sensor_interface;
    INT8U abBufFlag;
    INT8U RSD0;
    INT8U RSD1;
    
    //avi file info
    INT32U  avi_encode_status;      // 0:IDLE
    AviEncPacker_t *AviPackerCur;
    
	//allow record size
	INT64U  disk_free_size;			// disk free size
	INT32U  record_total_size;		// AviPacker storage to disk total size
    
	//aud & vid sync
	INT64S  delta_ta, tick;
    INT64S  delta_Tv;
    INT32S  freq_div;
    INT64S  ta, tv, Tv;
    INT32U  pend_cnt, post_cnt;
	INT32U  vid_pend_cnt, vid_post_cnt;
	VidEncFrame_t video[AVI_ENCODE_VIDEO_BUFFER_NO]; 
} AviEncPara_t;

typedef struct JpegPara_s
{
	INT32U jpeg_status;
	INT32U wait_done;
	INT32U quality_value;
	
	INT32U input_format;
	INT32U width;
	INT32U height;
	INT32U input_buffer_y;
	INT32U input_buffer_u;
	INT32U input_buffer_v;
	INT32U input_y_len;
	INT32U input_uv_len;
	INT32U output_buffer;
} JpegPara_t;

typedef struct capture_s
{
	INT16U width;
	INT16U height;
	INT32U quality;
	INT32U csi_buf;
	INT32U jpeg_buf;
	INT32U jpeg_len;
	OS_EVENT *Sensor_m;
	OS_EVENT *Ack_m;
	INT32U Stack[512];
} catpure_t;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG_MSG	DBG_PRINT
#else
	#define DEBUG_MSG(...)
#endif

#define RETURN(x)			{nRet = x; goto Return;}

#define POST_MESSAGE(msg_queue, message, ack_mbox, msc_time, msg, err)\
{\
	msg = (INT32S) OSMboxAccept(ack_mbox);\
	err = OSQPost(msg_queue, (void *)message);\
	if(err != OS_NO_ERR)\
	{\
		DEBUG_MSG("OSQPost Fail!!!\r\n");\
		RETURN(STATUS_FAIL);\
	}\
	msg = (INT32S) OSMboxPend(ack_mbox, msc_time/10, &err);\
	if(err != OS_NO_ERR || msg == ACK_FAIL)\
	{\
		DEBUG_MSG("OSMbox ack Fail!!!\r\n");\
		RETURN(STATUS_FAIL);\
	}\
}

/**************************************************************************
 *                              E X T E R N A L                           *
 **************************************************************************/
extern OS_EVENT *AVIEncodeApQ;
extern OS_EVENT *scaler_task_q;
extern OS_EVENT *scaler_frame_q;  
extern OS_EVENT *display_frame_q;
extern OS_EVENT *cmos_frame_q;
extern OS_EVENT *vid_enc_task_q;
extern OS_EVENT *vid_enc_frame_q;
extern OS_EVENT *video_frame_q;
extern OS_EVENT *avi_aud_q;

extern AviEncPara_t *pAviEncPara;
extern AviEncAudPara_t *pAviEncAudPara;
extern AviEncVidPara_t *pAviEncVidPara;
extern AviEncPacker_t *pAviEncPacker0, *pAviEncPacker1;
extern catpure_t *pCap;

// callback function
extern INT32S (*pfn_avi_encode_put_data)(void* workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);
extern INT32U (*videnc_display)(INT16U w, INT16U h, INT32U frame_addr);

//avi encode state
extern INT32U avi_enc_packer_start(AviEncPacker_t *pAviEncPacker);
extern INT32U avi_enc_packer_stop(AviEncPacker_t *pAviEncPacker); 
extern INT32S vid_enc_preview_start(void);
extern INT32S vid_enc_preview_stop(void);
extern INT32S avi_enc_start(void);
extern INT32S avi_enc_stop(void);
extern INT32S avi_enc_save_jpeg(void);
extern INT32S avi_enc_capture(INT16S fd, INT8U quality, INT16U width, INT16U height);
extern INT32S avi_enc_one_frame(void);
extern INT32S avi_enc_storage_full(void);
extern INT32S avi_packer_err_handle(INT32S ErrCode);
extern INT32S avi_encode_state_task_create(INT8U pori);
extern INT32S avi_encode_state_task_del(void);
extern void   avi_encode_state_task_entry(void *para);

//scaler task
extern INT32S scaler_task_create(INT8U pori);
extern INT32S scaler_task_del(void);
extern INT32S scaler_task_start(void);
extern INT32S scaler_task_stop(void);
extern void   scaler_task_entry(void *parm);

//video_encode task
extern INT32S video_encode_task_create(INT8U pori);
extern INT32S video_encode_task_del(void);
extern INT32S video_encode_task_start(void);
extern INT32S video_encode_task_stop(void);
extern INT32S video_encode_task_post_q(INT32U mode);
extern INT32S video_encode_task_empty_q(void);
extern void   video_encode_task_entry(void *parm);

//audio task
extern INT32S avi_adc_record_task_create(INT8U pori);
extern INT32S avi_adc_record_task_del(void);
extern INT32S avi_audio_record_start(void);
extern INT32S avi_audio_record_restart(void);
extern INT32S avi_audio_record_stop(void);

//avi encode api
extern void   avi_encode_init(void);

extern void avi_encode_video_timer_start(void);
extern void avi_encode_video_timer_stop(void);

extern INT32S avi_encode_set_file_handle_and_caculate_free_size(AviEncPacker_t *pAviEncPacker, INT16S FileHandle);
extern INT16S avi_encode_close_file(AviEncPacker_t *pAviEncPacker);
extern void avi_encode_fail_handle_close_file(AviEncPacker_t *pAviEncPacker);
extern INT32S avi_encode_set_avi_header(AviEncPacker_t *pAviEncPacker);
extern void avi_encode_set_curworkmem(void *pAviEncPacker);
//status
extern void   avi_encode_set_status(INT32U bit);
extern void   avi_encode_clear_status(INT32U bit);
extern INT32S avi_encode_get_status(void);
//memory
extern INT32U avi_encode_post_empty(OS_EVENT *event, INT32U frame_addr);
extern INT32U avi_encode_get_empty(OS_EVENT *event);
extern INT32U avi_encode_get_csi_frame(void);
extern INT32S avi_encode_memory_alloc(void);
extern INT32S avi_packer_memory_alloc(void);
extern void avi_encode_memory_free(void);
extern void avi_packer_memory_free(void);

//format
extern void avi_encode_set_sensor_format(INT32U format);
extern void avi_encode_set_display_format(INT32U format);
//other
extern void avi_encode_set_display_scaler(void);
extern INT32S avi_encode_set_jpeg_quality(INT8U quality_value);
extern BOOLEAN avi_encode_disk_size_is_enough(INT32S cb_write_size);

// isr handle
extern void csi_eof_isr(INT32U event);
extern void csi_capture_isr(INT32U event);
extern void csi_fifo_isr(INT32U event);
extern void cdsp_eof_isr(void);
extern void cdsp_capture_isr(void);
extern void cdsp_fifo_isr(void);
						
extern INT32S jpeg_encode_once(JpegPara_t *pJpegEnc);

// jpeg fifo encode
extern INT32S jpeg_encode_fifo_start(JpegPara_t *pJpegEnc);
extern INT32S jpeg_encode_fifo_once(JpegPara_t *pJpegEnc);
extern INT32S jpeg_encode_fifo_stop(JpegPara_t *pJpegEnc);
							
extern INT32S save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size);
extern INT32S avi_audio_memory_allocate(INT32U	cblen);
extern void avi_audio_memory_free(void);
extern INT32U avi_audio_get_next_buffer(void);

extern void avi_adc_hw_start(INT32U sample_rate);
extern void avi_adc_hw_stop(void);

#if	AVI_ENCODE_SHOW_TIME == 1
void cpu_draw_osd(const INT8U *source_addr, INT32U target_addr, INT16U offset, INT16U res);	
void cpu_draw_time_osd(TIME_T current_time, INT32U target_buffer, INT16U resolution);
#endif
#endif // __AVI_ENCODER_APP_H__
