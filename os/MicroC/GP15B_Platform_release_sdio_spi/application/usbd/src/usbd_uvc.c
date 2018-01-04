/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2014 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/*******************************************************
    Include file
*******************************************************/
#include "drv_l1_sfr.h"
#include "drv_l1_uart.h"
#include "drv_l1_csi.h"
#include "drv_l1_dma.h"
#include "drv_l1_jpeg.h"
#include "drv_l1_dac.h"
#include "drv_l1_i2s.h"
#include "drv_l2_sensor.h"
#include "fsystem.h"
#include "application.h"
#include "project.h"
#include "gplib_jpeg.h"
#include "gplib_jpeg_encode.h"
#include "jpeg_header.h"
#include "usbd.h"
#include "drv_l2_cdsp.h"

/******************************************************
    Definition and variable declaration
*******************************************************/
#define UVC_SENSOR_NAME		SENSOR_GC1004_CDSP_MIPI_NAME
#define UVC_WIDTH	WIDTH_1280
#define UVC_HEIGHT	HEIGHT_720
#define SENSOR_FRAME_BUF_NUM	2
#define JPEG_BUF_NUM			2
#define JPEG_BUF_SIZE			(250 << 10)		/* 250K */
#define JPEG_HEADER_LEN			624
#define JPEG_Q_VALUE			50

#define JPEG_IMG_FORMAT_422 	0x21
#define JPEG_IMG_FORMAT_420 	0x22

typedef enum
{	
	/* 0 for system DMA done */
	AUDIO_ENCODE_FRAME_DONE_EVENT = 1,	/* same as C_DMA_STATUS_DONE in system DMA */
	SENSOR_ENCODE_FRAME_DONE_EVENT,
	JPEG_ENCODE_DONE_EVENT,
	USBD_SEND_JPEG_EVENT,
	USBD_SEND_JPEG_DONE_EVENT,
	USBD_SEND_AUDIO_EVENT,
	USBD_SEND_AUDIO_DONE_EVENT,
	USBD_START_VIDEO_EVENT,
	USBD_STOP_VIDEO_EVENT,
	USBD_START_AUDIO_EVENT,
	USBD_STOP_AUDIO_EVENT,
	USBD_SET_INTERFACE_EVENT
} UVC_TASK_EVENT_E;	

char* event_str[13] =
{
	"null",
	"audio encode frame done",
	"sensor encode frame done",
	"jpeg encode done",
	"send jpeg",
	"send jpeg done",
	"send audio",
	"send audio done",
	"start video",
	"stop video",
	"start audio",
	"stop audio",
	"set interface",
};	

typedef enum
{
	USBD_ISO_IDLE_STATE = 0,
	USBD_ISO_SENDING_VIDEO_STATE,
	USBD_ISO_VIDEO_DONE_STATE,
	USBD_ISO_SENDING_AUDIO_STATE,
	USBD_ISO_AUDIO_DONE_STATE
} USBD_ISO_STATE_E;	

char* state_str[5] =
{
	"idle state",
	"sending video",
	"video done",
	"sending audio ",
	"audio done",
};	

typedef enum
{
	UVC_BUF_EMPTY_STATE = 0,
	UVC_BUF_FILLING_STATE,
	UVC_BUF_FULL_STATE,
	UVC_BUF_JPEG_ENCODING_STATE,
	UVC_BUF_SENDING_STATE
} UVC_BUF_STATE_E;	

typedef struct USBD_BUF_S
{
	INT32U bufptr;
	struct USBD_BUF_S* next;
	INT32U state;
	INT32U size;
} UVC_BUF_T;

typedef struct JPEG_PARAMETER_S
{
	INT32U mode;
	INT32U inputbuf;
	INT32U outputbuf;
	INT32U Qvalue;
	INT16U width;
	INT16U height;
} JPEG_PARAMETER_T;

static INT16U uvc_width = UVC_WIDTH;
static INT16U uvc_height = UVC_HEIGHT;
static JPEG_PARAMETER_T jpeg_pars;
static UVC_BUF_T sensor_frame_buf[SENSOR_FRAME_BUF_NUM];
static UVC_BUF_T jpeg_buf[JPEG_BUF_NUM];
static UVC_BUF_T* current_frame_buf = NULL;
static UVC_BUF_T* cur_jpeg_write_ptr = NULL;
static UVC_BUF_T* cur_jpeg_read_ptr = NULL;
static INT32U send_video = 0;
static INT32U send_audio = 0;
static drv_l2_sensor_ops_t* sensor_ops = NULL;
static drv_l2_sensor_info_t* sensor_info = NULL;
static INT32U sensor_info_index;
static INT32U usb_iso_state = USBD_ISO_IDLE_STATE;

#if (_OPERATING_SYSTEM == _OS_UCOS2)
#define UVCStackSize 		512
#define USBAPPStackSize 	512
#define UVCENC_QUEUE_MAX_LEN	16
#define UVCAP_QUEUE_MAX_LEN		8
INT32U UVCStack[UVCStackSize];
INT32U USBAPPStack[USBAPPStackSize];
static void* UVCQ_Stack[UVCENC_QUEUE_MAX_LEN];
OS_EVENT *UVCQ = NULL;
static void* USBAPQ_Stack[UVCAP_QUEUE_MAX_LEN];
OS_EVENT *USBAPQ = NULL;
#endif

#if (UVC_AUDIO_SUPPORT == 1)
#define UAC_AUDIO_PCM_SAMPLES (((((UAC_AUDIO_SAMPLE_RATE/1000)*36) + 7) & ~0x7))		/* How many audio samples in 33ms(30 FPS time interval) */
#define UAC_AUDIO_BUF_SIZE (UAC_AUDIO_PCM_SAMPLES<<1)
#define UAC_AUDIO_BUF_NUM	4
static DMA_STRUCT audio_dma;
static UVC_BUF_T audio_buf[UAC_AUDIO_BUF_NUM];
static UVC_BUF_T* cur_audio_write_ptr = NULL;
static UVC_BUF_T* cur_audio_read_ptr = NULL;
static INT32U audiostop = 0;
#endif

INT8U sensor_cdsp_type;

/******************************************************
    Functions declaration
*******************************************************/
static void _jpeg_get_send_buffer(INT32U* buf, INT32U* size)
{
	if(cur_jpeg_read_ptr->state == UVC_BUF_FULL_STATE)
	{
		cur_jpeg_read_ptr->state = UVC_BUF_SENDING_STATE;
		*buf = cur_jpeg_read_ptr->bufptr;
		*size = cur_jpeg_read_ptr->size;
	}
	else
	{
		*buf = 0;
		*size = 0;
	}
		
}	

static INT32U _jpeg_get_next_buffer(void)
{
	INT32U ret = NULL;
	
	if(cur_jpeg_write_ptr->state == UVC_BUF_EMPTY_STATE)
	{
		cur_jpeg_write_ptr->state = UVC_BUF_FILLING_STATE;
		ret = cur_jpeg_write_ptr->bufptr;
	}
	else if(cur_jpeg_write_ptr->next->state == UVC_BUF_EMPTY_STATE)
	{
		cur_jpeg_write_ptr->next->state = UVC_BUF_FILLING_STATE;
		ret = cur_jpeg_write_ptr->next->bufptr;
	}	
	//DBG_PRINT("JB = 0x%x\r\n", ret);
	return ret;
}	

static void _jpeg_isr_cbk(INT32S event, INT32U vlcCount)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U msg = JPEG_ENCODE_DONE_EVENT;
	INT8U err;
#endif
	
	if(event & C_JPG_STATUS_ENCODE_DONE)
	{		
		cur_jpeg_write_ptr->size = vlcCount + JPEG_HEADER_LEN;
#if _OPERATING_SYSTEM == _OS_UCOS2		
		err = OSQPost(UVCQ, (void*)msg);
		if(err != OS_NO_ERR)
		{
			DBG_PRINT("_jpeg_isr_cbk send UVCQ failed, err %d\r\n", err);
		}
#endif		
		return;
	}
	DBG_PRINT("J\r\n");
}

static void _jpeg_do_encode(JPEG_PARAMETER_T* para)
{
	/* Init JPEG layer 1 driver */
	drv_l1_jpeg_init();
	/* Generate JPEG header */
	jpeg_header_generate(JPEG_IMG_FORMAT_422, para->Qvalue, uvc_width, uvc_height);
	/* Copy header to output buffer */
	gp_memcpy((INT8S *)para->outputbuf, (INT8S *)jpeg_header_get_addr(), jpeg_header_get_size());
	/* Set sampling_mode */
	drv_l1_jpeg_yuv_sampling_mode_set(jpeg_pars.mode);
	/* Set JPEG image size */
	drv_l1_jpeg_image_size_set(uvc_width, uvc_height);
	/* Set input YUV data buffer address */
	drv_l1_jpeg_yuv_addr_set(para->inputbuf, 0, 0);
	/* Set output jpeg buffer address (16-byte alignment) */
	drv_l1_jpeg_vlc_addr_set(para->outputbuf + JPEG_HEADER_LEN);
	/* Start to encode jpeg */
	drv_l1_jpeg_compression_start(_jpeg_isr_cbk);
}	

static void _set_frame_buffer_state(INT32U state)
{
	INT32U i;
	
	for(i=0; i<SENSOR_FRAME_BUF_NUM; i++)
	{
		sensor_frame_buf[i].state = state;
	}
}

static void _set_jpeg_buffer_state(INT32U state)
{
	INT32U i;
	
	for(i=0; i<JPEG_BUF_NUM; i++)
	{
		jpeg_buf[i].state = state;
	}
}

static void _usbd_ep5_send_done_cbk(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U msg = USBD_SEND_JPEG_DONE_EVENT;
	INT8U err;
	
	err = OSQPost(UVCQ, (void*)msg);
	if(err != OS_NO_ERR)
	{
		DBG_PRINT("_usbd_ep5_send_done_cbk send UVCQ failed, err %d\r\n", err);
	}
#endif
	/* Set EP5 frame end flag for UVC header */
	drv_l1_usbd_set_frame_end_ep5();	
}	

static void _sensor_complete_buffer_handler(INT32U buf, INT32U event)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U msg = SENSOR_ENCODE_FRAME_DONE_EVENT;
	INT8U err;
	
	err = OSQPost(UVCQ, (void*)msg);
	if(err != OS_NO_ERR)
	{
		DBG_PRINT("_sensor_complete_buffer_handler send UVCQ failed, err %d\r\n", err);
	}
#endif	
}

static INT32U _sensor_get_next_buffer(void)
{
	INT32U ret = NULL;
	
	if(current_frame_buf->state == UVC_BUF_EMPTY_STATE)
	{
		current_frame_buf->state = UVC_BUF_FILLING_STATE;
		ret = current_frame_buf->bufptr;
	}
	else if(current_frame_buf->next->state == UVC_BUF_EMPTY_STATE)
	{
		current_frame_buf->next->state = UVC_BUF_FILLING_STATE;
		ret = current_frame_buf->next->bufptr;
	}	
	
	return ret;
}	

static void _sensor_cdsp_isr_cbk(void)
{
	INT32U complete;

	/* Get the ready buffer address */
	complete = drv_l1_csi_get_buf();
	_sensor_complete_buffer_handler(complete, NULL);	
	
	
}

void _sensor_isr_cbk(INT32U event)
{
	INT32U complete;
	
	switch(event)
	{
		case CSI_SENSOR_FRAME_END_EVENT:
			/* Get the ready buffer address */
			complete = drv_l1_csi_get_buf();
			_sensor_complete_buffer_handler(complete, event);	
			break;
		default:
			break;
	}	
}

#if (UVC_AUDIO_SUPPORT == 1)
static void _set_audio_buffer_state(INT32U state)
{
	INT32U i;
	
	for(i=0; i<UAC_AUDIO_BUF_NUM; i++)
	{
		audio_buf[i].state = state;
	}
}

static void _audio_get_send_buffer(INT32U* buf, INT32U* size)
{
	if(cur_audio_read_ptr->state == UVC_BUF_FULL_STATE)
	{
		cur_audio_read_ptr->state = UVC_BUF_SENDING_STATE;
		*buf = cur_audio_read_ptr->bufptr;
		*size = cur_audio_read_ptr->size;
	}
	else
	{
		*buf = 0;
		*size = 0;
	}	
}

static INT32U _audio_get_next_buffer(void)
{
	INT32U ret = NULL;
	
	if(cur_audio_write_ptr->state == UVC_BUF_EMPTY_STATE)
	{
		cur_audio_write_ptr->state = UVC_BUF_FILLING_STATE;
		ret = cur_audio_write_ptr->bufptr;
	}
	else if(cur_audio_write_ptr->next->state == UVC_BUF_EMPTY_STATE)
	{
		cur_audio_write_ptr->next->state = UVC_BUF_FILLING_STATE;
		ret = cur_audio_write_ptr->next->bufptr;
	}	
	else
	{
		DBG_PRINT("Can not get free buffer for audio writing\r\n");
	}	
	return ret;
}

static INT32S _audio_buffer_put(INT32U data, INT32U len, OS_EVENT *os_q)
{
	INT32S status;

	if(data == NULL)
		return STATUS_FAIL;
		
	audio_dma.s_addr = (INT32U)P_I2SRX_DATA;
	audio_dma.t_addr = (INT32U)data;
	audio_dma.width = DMA_DATA_WIDTH_4BYTE;		
	audio_dma.count = (INT32U)len/2;
	audio_dma.notify = NULL;
	audio_dma.timeout = 0;
	status = drv_l1_dma_transfer_with_double_buf(&audio_dma, os_q);
	if(status < 0)
		return status;
	
	return STATUS_OK;
}	

static INT32S _audio_buffer_set(INT32U data, INT32U len)
{
	INT32S status;

	if(data == NULL)
		return STATUS_FAIL;
		
	audio_dma.s_addr = (INT32U)P_I2SRX_DATA;
	audio_dma.t_addr = (INT32U)data;
	audio_dma.width = DMA_DATA_WIDTH_4BYTE;		
	audio_dma.count = (INT32U)len/2;
	audio_dma.notify = NULL;
	audio_dma.timeout = 0;
	status = drv_l1_dma_transfer_double_buf_set(&audio_dma);
	if(status < 0)
		return status;
	
	return STATUS_OK;
}	

static void _audio_start(void)
{
	/* init I2S RX and sampling rate */
	drv_l1_i2s_rx_init();
	drv_l1_i2s_rx_sample_rate_set(UAC_AUDIO_SAMPLE_RATE);
	drv_l1_i2s_rx_mono_ch_set();
	drv_l1_i2s_rx_start();
}	

static void _audio_stop(void)
{
	audiostop = 1;
}
#endif

void uvc_usbd_task(void *param)
{
	INT8U err;
	INT32U event;
	INT32U vbuf, vsize;
#if (UVC_AUDIO_SUPPORT == 1)	
	INT32U abuf, asize;
#endif	
	static INT32U old_video = 0;
	static INT32U old_audio = 0;
	
	while(1)
	{
		/* Receive message from queue */
    	event = (INT32U)OSQPend(USBAPQ, 0, &err);
    	//DBG_PRINT("event %s, state %s, v[%d] a[%d]\r\n", event_str[event], state_str[usb_iso_state], send_video, send_audio);
    	OSSchedLock();
    	switch(event)
    	{
    		case USBD_SET_INTERFACE_EVENT:
    			/* Check video interface */	
    			if(old_video != send_video)
    			{
#if _OPERATING_SYSTEM == _OS_UCOS2
    				INT32U msg;
					INT8U err;
    				if(send_video)
    				{
						msg = USBD_START_VIDEO_EVENT;	
    				}
    				else
    				{
						msg = USBD_STOP_VIDEO_EVENT;	
    				}

    				err = OSQPost(UVCQ, (void*)msg);
					if(err != OS_NO_ERR)
					{
						DBG_PRINT("uvc_usbd_task send UVCQ failed, err %d\r\n", err);
					}
#endif						
    				old_video = send_video;
    			}
    			/* Check audio interface */	
    			if(old_audio != send_audio)
    			{
#if _OPERATING_SYSTEM == _OS_UCOS2    				
    				INT32U msg = USBD_START_AUDIO_EVENT;
					INT8U err;
    				if(send_audio)
    				{
						msg = USBD_START_AUDIO_EVENT;
    				}
    				else
    				{
						msg = USBD_STOP_AUDIO_EVENT;
    				}


    				err = OSQPost(UVCQ, (void*)msg);
					if(err != OS_NO_ERR)
					{
						DBG_PRINT("uvc_usbd_task send UVCQ failed, err %d\r\n", err);
					}
#endif					
    				old_audio = send_audio;		
    			}
    			
    			if(old_video == 0 && old_audio == 0)
				{
					/* reset state to idle when video & audio are in idle */
					usb_iso_state = USBD_ISO_IDLE_STATE;
				}
    			DBG_PRINT("Get set interface state 0x%x v[%d] a[%d]\r\n", usb_iso_state, send_video, send_audio);	
    			break;
    			
    			case USBD_SEND_JPEG_EVENT:
    				DBG_PRINT("event %s, state %s, v[%d] a[%d]\r\n", event_str[event], state_str[usb_iso_state], send_video, send_audio);
    				if(send_video)
	    			{	
	    				if(((usb_iso_state == USBD_ISO_IDLE_STATE || usb_iso_state == USBD_ISO_AUDIO_DONE_STATE) && send_audio) ||
	    					(usb_iso_state != USBD_ISO_SENDING_VIDEO_STATE && !send_audio))
	    				{
	    					_jpeg_get_send_buffer(&vbuf, &vsize);
	    					if(vbuf)
	    					{
	    						usb_iso_state = USBD_ISO_SENDING_VIDEO_STATE;
	    						drv_l1_usbd_frame_iso_ep5_in((void *)vbuf, vsize, 1);
	    					}
	    					else
	    					{
	    						/* No JPEG to send */
	    						DBG_PRINT("No available JPEG buffer to send\r\n");
	    					}		
	    				}
	    				else
	    				{
	    					/* Send  USBD_SEND_JPEG_EVENT for waiting usb_iso_state */
#if _OPERATING_SYSTEM == _OS_UCOS2
							INT32U msg = USBD_SEND_JPEG_EVENT;
							INT8U err;
						
							//OSTimeDly(1);
							err = OSQPost(USBAPQ, (void*)msg);
							if(err != OS_NO_ERR)
							{
								DBG_PRINT("uvc_usbd_task send UVCQ failed, err %d\r\n", err);
							}
#endif
	    					if(usb_iso_state == USBD_ISO_VIDEO_DONE_STATE)
								usb_iso_state = USBD_ISO_IDLE_STATE;
	    				}	
    				}
    				else
    				{
    					/* Discard this JPEG buffer due to video alternative interface value is 0 */
    					cur_jpeg_read_ptr->state = UVC_BUF_EMPTY_STATE;
						cur_jpeg_read_ptr->size = 0;
						cur_jpeg_read_ptr = cur_jpeg_read_ptr->next;
						//DBG_PRINT("LV\r\n");
    				}		
    				break;

#if (UVC_AUDIO_SUPPORT == 1)    			
    			case USBD_SEND_AUDIO_EVENT:
    				DBG_PRINT("event %s, state %s, v[%d] a[%d]\r\n", event_str[event], state_str[usb_iso_state], send_video, send_audio);
    				if(send_audio)
	    			{	
	    				if(((usb_iso_state == USBD_ISO_IDLE_STATE || usb_iso_state == USBD_ISO_VIDEO_DONE_STATE) && send_video) ||
	    					(usb_iso_state != USBD_ISO_SENDING_AUDIO_STATE && !send_video))

	    				{
	    					_audio_get_send_buffer(&abuf, &asize);
	    	
	    					if(abuf)
	    					{
	    						usb_iso_state = USBD_ISO_SENDING_AUDIO_STATE;
	    						drv_l1_usbd_dma_iso_ep7_in((void *)abuf, asize);
	    					}
	    					else
	    					{
	    						/* No JPEG to send */
	    						DBG_PRINT("No available audio buffer to send\r\n");
	    					}		
	    				}
	    				else
	    				{
	    					/* Send USBD_SEND_AUDIO_EVENT for waiting usb_iso_state */
#if _OPERATING_SYSTEM == _OS_UCOS2
							INT32U msg = USBD_SEND_AUDIO_EVENT;
							INT8U err;
						
							//OSTimeDly(1);
							err = OSQPost(USBAPQ, (void*)msg);
							if(err != OS_NO_ERR)
							{
								DBG_PRINT("uvc_usbd_task send UVCQ failed, err %d\r\n", err);
							}
#endif
							if(usb_iso_state == USBD_ISO_AUDIO_DONE_STATE)
								usb_iso_state = USBD_ISO_IDLE_STATE;
	    				}	
    				}
    				else
    				{
    					/* Discard this JPEG buffer due to video alternative interface value is 0 */
    					cur_audio_read_ptr->state = UVC_BUF_EMPTY_STATE;
						cur_audio_read_ptr = cur_audio_read_ptr->next;
						//usb_iso_state = USBD_ISO_AUDIO_DONE_STATE;
						//DBG_PRINT("AV\r\n");
    				}		
    				break;	
#endif    			
    		default:
    			break;
    	}	
    	OSSchedUnlock();
    }	
}

void uvc_main_task(void *param)
{
	INT8U err;
	INT32U event, next;
	
	while(1)
	{
		/* Receive message from queue */
    	event = (INT32U)OSQPend(UVCQ, 0, &err);
    	OSSchedLock();
    	//DBG_PRINT("main task, event 0x%x\r\n", event);
    	switch(event)
		{
#if (UVC_AUDIO_SUPPORT == 1)			
			case AUDIO_ENCODE_FRAME_DONE_EVENT:
				/* audio DMA done */
				if(cur_audio_write_ptr->state == UVC_BUF_FILLING_STATE)
				{
					cur_audio_write_ptr->state = UVC_BUF_FULL_STATE;					
					cur_audio_write_ptr = cur_audio_write_ptr->next;
				}
				
				if(!audiostop)
				{
					_audio_buffer_set(_audio_get_next_buffer(), UAC_AUDIO_PCM_SAMPLES);
				}	
				else
				{		
					/* Stop audio hardware */
					drv_l1_i2s_rx_stop();
				}	

#if _OPERATING_SYSTEM == _OS_UCOS2
				{
					INT32U msg = USBD_SEND_AUDIO_EVENT;
					INT8U err;
			
					err = OSQPost(USBAPQ, (void*)msg);
					if(err != OS_NO_ERR)
					{
						DBG_PRINT("uvc_main_task send UVCQ failed, err %d\r\n", err);
					}
				}		
#endif				
				break;
			
			case USBD_SEND_AUDIO_DONE_EVENT:
				/* send audio done via USB */
				if(cur_audio_read_ptr->state == UVC_BUF_SENDING_STATE)
				{
					cur_audio_read_ptr->state = UVC_BUF_EMPTY_STATE;
					cur_audio_read_ptr = cur_audio_read_ptr->next;
				}	
				usb_iso_state = USBD_ISO_AUDIO_DONE_STATE;
				break;
			
			case USBD_START_AUDIO_EVENT:
				DBG_PRINT("Start to transfer UAC data\r\n");
				_set_audio_buffer_state(UVC_BUF_EMPTY_STATE);
				cur_audio_write_ptr = &audio_buf[0];
				cur_audio_read_ptr = &audio_buf[0];
				audiostop = 0;
				_audio_start();	
				_audio_buffer_put(_audio_get_next_buffer(), UAC_AUDIO_PCM_SAMPLES, UVCQ);
				_audio_buffer_set(_audio_get_next_buffer(), UAC_AUDIO_PCM_SAMPLES);
				break;
			
			case USBD_STOP_AUDIO_EVENT:
				_audio_stop();
				/* Flush EP7 if needs */
				if(drv_l2_usbd_uvc_check_ep7_flush())
				{	
					DBG_PRINT("EP7 flush\r\n");
					drv_l1_usbd_iso_ep7_flush();
					usb_iso_state = USBD_ISO_AUDIO_DONE_STATE;
				}	
				break;		
#endif			

			case SENSOR_ENCODE_FRAME_DONE_EVENT:
				 
				 
				/* Get next sensor frame buffer */
				 if(sensor_cdsp_type == 0)
				 	next = _sensor_get_next_buffer();
				 else
				 {
				 	//_sensor_get_next_buffer();
					current_frame_buf->state = UVC_BUF_FILLING_STATE;
					next = current_frame_buf->bufptr;
				 }
				if(next)
				{
					drv_l1_csi_set_buf(next);
				}
				else
				{
					DBG_PRINT("Can't get free frame buffer\r\n");
					break;
				}			
			
				if(current_frame_buf->state == UVC_BUF_FILLING_STATE)
				{
					current_frame_buf->state = UVC_BUF_JPEG_ENCODING_STATE;
					/* Start to JPEG encoding */
					jpeg_pars.inputbuf = current_frame_buf->bufptr;
					jpeg_pars.outputbuf = _jpeg_get_next_buffer();
					if(jpeg_pars.outputbuf)
						_jpeg_do_encode(&jpeg_pars);
					else
						DBG_PRINT("Free JPEG buffer not available\r\n");	
				}	
				break;
				
			case JPEG_ENCODE_DONE_EVENT:
				DBG_PRINT("J");
				/* Check sensor buffer state */
				if(current_frame_buf->state == UVC_BUF_JPEG_ENCODING_STATE)
				{
					current_frame_buf->state = UVC_BUF_EMPTY_STATE;
					if(sensor_cdsp_type == 0)
						current_frame_buf = current_frame_buf->next;
				}
				
				/* Check JPEG buffer state */
				if(cur_jpeg_write_ptr->state == UVC_BUF_FILLING_STATE)
				{
					cur_jpeg_write_ptr->state = UVC_BUF_FULL_STATE;
					//DBG_PRINT("JPEG DONE, size = 0x%x\r\n", cur_jpeg_write_ptr->size);
					cur_jpeg_write_ptr = cur_jpeg_write_ptr->next;
				}
				/* Send message to USBAPQ for sending JPEG via USBD */
#if _OPERATING_SYSTEM == _OS_UCOS2
				{
					INT32U msg = USBD_SEND_JPEG_EVENT;
					INT8U err;
			
					err = OSQPost(USBAPQ, (void*)msg);
					if(err != OS_NO_ERR)
					{
						DBG_PRINT("uvc_main_task send USBAPQ failed, err %d\r\n", err);
					}
				}		
#endif	
				break;
			
			case USBD_SEND_JPEG_DONE_EVENT:
				/* Find the jpeg buffer in sending status */
				if(cur_jpeg_read_ptr->state == UVC_BUF_SENDING_STATE)
				{
					cur_jpeg_read_ptr->state = UVC_BUF_EMPTY_STATE;
					cur_jpeg_read_ptr->size = 0;
					cur_jpeg_read_ptr = cur_jpeg_read_ptr->next;
				}
				usb_iso_state = USBD_ISO_VIDEO_DONE_STATE;	
				break;

			case USBD_START_VIDEO_EVENT:
				DBG_PRINT("Start to transfer UVC data\r\n");
				_set_frame_buffer_state(UVC_BUF_EMPTY_STATE);
				_set_jpeg_buffer_state(UVC_BUF_EMPTY_STATE);
				cur_jpeg_write_ptr = &jpeg_buf[0];
				cur_jpeg_read_ptr = &jpeg_buf[0];
				drv_l2_sensor_set_path(0);
				drv_l2_set_raw_yuv_path(1);
				drv_l2_set_target_sensor_size(uvc_width, uvc_height);
				sensor_ops->stream_start(sensor_info_index, _sensor_get_next_buffer(), NULL);
				break;								
			
			case USBD_STOP_VIDEO_EVENT:
				DBG_PRINT("Stop transfering UVC data\r\n");
				sensor_ops->stream_stop();
				/* Flush EP5 if needs */
				if(drv_l2_usbd_uvc_check_ep5_flush())
				{	
					DBG_PRINT("EP5 flush\r\n");
					drv_l1_usbd_iso_ep5_flush();
					usb_iso_state = USBD_ISO_VIDEO_DONE_STATE;
				}
				break;	
				
			default:
				DBG_PRINT("Unknow event %d\r\n", event);
				break;	
		}
		OSSchedUnlock();
	}		
}

static void _uvc_jpeg_uninit(void)
{
	INT32U i;
	
	/* free JPEG buffer */
	for(i=0; i<JPEG_BUF_NUM; i++)
	{
		if(jpeg_buf[i].bufptr)
		{
			gp_free(jpeg_buf[i].bufptr);
			jpeg_buf[i].bufptr = NULL;
		}
	}
	
	cur_jpeg_write_ptr = NULL;
	cur_jpeg_read_ptr = NULL;
}

static void _uvc_jpeg_init(void)
{
	INT32U i;
	
	/* Allocate JPEG encoding buffer */
	for(i=0; i<JPEG_BUF_NUM ; i++)
	{
		jpeg_buf[i].bufptr = (INT32U)gp_malloc_align(JPEG_BUF_SIZE, 16);
		
		if(jpeg_buf[i].bufptr == NULL)
		{
			DBG_PRINT("Allocate JPEG buffer failed in buffer[i]\r\n", i);
			_uvc_jpeg_uninit();
			return;
		}
		
		jpeg_buf[i].state = UVC_BUF_EMPTY_STATE;
		
		if(i == (JPEG_BUF_NUM-1))	
		{
			jpeg_buf[i].next = (UVC_BUF_T*)&jpeg_buf[0];
		}
		else
		{
			jpeg_buf[i].next = (UVC_BUF_T*)&jpeg_buf[i+1];
		}
			
		//DBG_PRINT("JPEG buffer[0x%x][%d]\r\n", jpeg_buf[i].bufptr, i);
	}
	
	cur_jpeg_write_ptr = &jpeg_buf[0];
	cur_jpeg_read_ptr = &jpeg_buf[0];
	
	/* Init default jpeg default huffman table */
	gplib_jpeg_default_huffman_table_load();
	
	/* Init JPEG parameters */
	jpeg_pars.mode = C_JPG_CTRL_YUV422;
	jpeg_pars.inputbuf = NULL;
	jpeg_pars.outputbuf = NULL;
	jpeg_pars.Qvalue = JPEG_Q_VALUE;
	jpeg_pars.width = uvc_width;
	jpeg_pars.height = uvc_height;

	/* Set JPEG Q value */
	jpeg_header_quantization_table_calculate(ENUM_JPEG_LUMINANCE_QTABLE, jpeg_pars.Qvalue, NULL);	//Y
	jpeg_header_quantization_table_calculate(ENUM_JPEG_CHROMINANCE_QTABLE, jpeg_pars.Qvalue, NULL);	//UV
	
	DBG_PRINT("UVC init JPEG %dx%d, Q = %d\r\n", jpeg_pars.width, jpeg_pars.height, jpeg_pars.Qvalue);
}	

static void _uvc_sensor_uninit(void)
{
	INT32U i;
	
	/* free frame buffer */
	for(i=0; i<SENSOR_FRAME_BUF_NUM; i++)
	{
		if(sensor_frame_buf[i].bufptr)
		{
			gp_free(sensor_frame_buf[i].bufptr);
			sensor_frame_buf[i].bufptr = NULL;
		}	
	}	
	
	current_frame_buf = NULL;
	
	if(sensor_ops)
	{
		/* Stop sensor */
		sensor_ops->stream_stop();
		/* Uninit sensor */
		sensor_ops->uninit();
	}		
}

static void _uvc_sensor_init(void)
{
	CHAR *p;
	INT32U i;

	/* Find sensor */
	for(i=0; i<MAX_SENSOR_NUM ;i++ )
	{
		sensor_ops = drv_l2_sensor_get_ops(i);
		if(sensor_ops != NULL)
		{
			if(!strcmp(UVC_SENSOR_NAME, sensor_ops->name))
			{
				DBG_PRINT("Sensor %s found in index %d\r\n", UVC_SENSOR_NAME, i);
				break;
			}		
		}
		else
		{
			DBG_PRINT("End searching of sensor operation table, %d sensor ops searched\r\n", i);
			break;
		}	
	}
	
	// set csi or cdsp path
	p = (CHAR *)strrchr((CHAR *)sensor_ops->name, 'c');
	if(p == 0) {
		return;
	}
	
	if(strncmp((CHAR *)p, "csi", 3) == 0) {
		drv_l2_sensor_set_path(1);
		sensor_cdsp_type = 0;
		DBG_PRINT("CSI\r\n");
	} else {
		drv_l2_sensor_set_path(0);
		sensor_cdsp_type = 1;
		DBG_PRINT("CDSP\r\n");
	}
	
	/* Find sensors information */
	if(sensor_ops)
	{
		/* Use index 0 in info table for UVC */
		sensor_info_index = 1;
		sensor_info = sensor_ops->get_info(sensor_info_index);
		uvc_width = sensor_info->target_w;
		uvc_height = sensor_info->target_h;
		DBG_PRINT("W[%d]/H[%d] for UVC\r\n", uvc_width, uvc_height);
	}
	else
	{
		DBG_PRINT("Can't find sensor, sensor init failed\r\n");
		_uvc_sensor_uninit();
		return;
	}	
	
	/* Allocate sensor frame buffer & JPEG buffer*/
	for(i=0; i<SENSOR_FRAME_BUF_NUM ; i++)
	{
		sensor_frame_buf[i].bufptr = (INT32U)gp_malloc_align((uvc_height*uvc_width*2), 32);
		
		if(sensor_frame_buf[i].bufptr == NULL)
		{
			DBG_PRINT("Allocate frame buffer failed in buffer[i]\r\n", i);
			_uvc_sensor_uninit();
			return;
		}
		
		sensor_frame_buf[i].state = UVC_BUF_EMPTY_STATE;
		
		if(i == (SENSOR_FRAME_BUF_NUM-1))	
		{
			sensor_frame_buf[i].next = (UVC_BUF_T*)&sensor_frame_buf[0];
		}
		else
		{
			sensor_frame_buf[i].next = (UVC_BUF_T*)&sensor_frame_buf[i+1];
		}
			
		DBG_PRINT("Frame buffer[0x%x][%d]\r\n", sensor_frame_buf[i].bufptr, i);
	}
	
	current_frame_buf = &sensor_frame_buf[0];
	
	/* Init sensor */
	sensor_ops->init();
	
	/* register CSI callback function */
	if(sensor_cdsp_type == 0)
		drv_l1_register_csi_cbk((CSI_CALLBACK_FUN)_sensor_isr_cbk);
	else
		drv_l2_CdspIsrRegister(C_ISR_EOF,_sensor_cdsp_isr_cbk);
}	

#if (UVC_AUDIO_SUPPORT == 1)
static void _uvc_audio_uninit(void)
{
	INT32U i;
	
	/* Free audio frame buffer */
	for(i=0; i<UAC_AUDIO_BUF_NUM; i++)
	{
		if(audio_buf[i].bufptr)
		{
			gp_free(audio_buf[i].bufptr);
			audio_buf[i].bufptr = NULL;
		}
	}
	
	cur_audio_write_ptr = NULL;
	cur_audio_read_ptr = NULL;
	
	drv_l1_i2s_rx_exit();
}

static void _uvc_audio_init(void)
{
	INT32U i;
	
	/* Initial DAC */
	drv_l1_dac_init();
	
	/* set mic input */	
	drv_l1_i2s_rx_set_input_path(MIC_INPUT);
	
	/* Allocate aduio frame buffer */
	for(i=0; i<UAC_AUDIO_BUF_NUM ; i++)
	{
		audio_buf[i].bufptr = (INT32U)gp_malloc_align(UAC_AUDIO_BUF_SIZE, 16);
		
		if(audio_buf[i].bufptr == NULL)
		{
			DBG_PRINT("Allocate audio buffer failed in buffer[i]\r\n", i);
			_uvc_audio_uninit();
			return;
		}
		
		audio_buf[i].state = UVC_BUF_EMPTY_STATE;
		audio_buf[i].size = UAC_AUDIO_BUF_SIZE;
		
		if(i == (UAC_AUDIO_BUF_NUM-1))	
		{
			audio_buf[i].next = (UVC_BUF_T*)&audio_buf[0];
		}
		else
		{
			audio_buf[i].next = (UVC_BUF_T*)&audio_buf[i+1];
		}
		//DBG_PRINT("Audio buffer[0x%x][%d]\r\n", jpeg_buf[i].bufptr, i);
	}
	
	cur_audio_write_ptr = &audio_buf[0];
	cur_audio_read_ptr = &audio_buf[0];
	
	DBG_PRINT("audio init, sample rate = %d, pcm sample = %d, buffer size = %d\r\n", UAC_AUDIO_SAMPLE_RATE, UAC_AUDIO_PCM_SAMPLES, UAC_AUDIO_BUF_SIZE);
}
	

static void _usbd_ep7_send_done_cbk(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U msg = USBD_SEND_AUDIO_DONE_EVENT;
	INT8U err;
	
	err = OSQPost(UVCQ, (void*)msg);
	if(err != OS_NO_ERR)
	{
		DBG_PRINT("_usbd_ep7_send_done_cbk send UVCQ failed, err %d\r\n", err);
	}
#endif	
}

#endif

void _usbd_get_set_interface(INT32U video, INT32U audio)
{
	send_video = video;
	send_audio = audio;
#if _OPERATING_SYSTEM == _OS_UCOS2
	{
		INT32U msg = USBD_SET_INTERFACE_EVENT;
		INT8U err;
		
		err = OSQPost(USBAPQ, (void*)msg);
		if(err != OS_NO_ERR)
		{
			DBG_PRINT("_usbd_get_set_interface send USBAPQ failed, err %d\r\n", err);
		}
	}
#endif	
	//DBG_PRINT("UVC set interface v[%d] a[%d]\r\n", send_video, send_audio);
}	

void usbd_uvc_init(void)
{
    INT32S ret;
    INT8U err;

    /* Init sensor related parameters */
    _uvc_sensor_init();
    
    /* Init JPEG module */
    _uvc_jpeg_init();

#if (UVC_AUDIO_SUPPORT == 1)
	/* Init audio module */
	_uvc_audio_init();
#endif
    
#if _OPERATING_SYSTEM == _OS_UCOS2
    /* Create UVC & USB task, create encode message queue */		
	UVCQ = OSQCreate(UVCQ_Stack, UVCENC_QUEUE_MAX_LEN);
	if(!UVCQ)
	{
		DBG_PRINT("UVC: Create UVCQ failed!\r\n");
		return;
	}
	else
	{
		/* clean the message Q */
		OSQFlush(UVCQ);
	}
	
	USBAPQ = OSQCreate(USBAPQ_Stack, UVCAP_QUEUE_MAX_LEN);
	if(!USBAPQ)
	{
		DBG_PRINT("USBAP: Create USBAPQ failed!\r\n");
		return;
	}
	else
	{
		/* clean the message Q */
		OSQFlush(USBAPQ);
	}
	
	/* Create UVC encode task */
	err = OSTaskCreate(uvc_main_task, (void *)0, &UVCStack[UVCStackSize - 1], UVCTASKPRIORITY);
	if(err != OS_NO_ERR)
	{
		DBG_PRINT("Create uvc_main_task task failed!\r\n");
		return;
	}
	
	/* Create UVC polling USB task */
	err = OSTaskCreate(uvc_usbd_task, (void *)0, &USBAPPStack[USBAPPStackSize - 1], USBDAPPTASKPRIORITY);
	if(err != OS_NO_ERR)
	{
		DBG_PRINT("Create uvc_usbd_task task failed!\r\n");
		return;
	}
#endif

	/* Init USBD L2 protocol layer first, including control/bulk/ISO/interrupt transfers */
    /******************************* Control transfer ************************************/
    ret = drv_l2_usbd_ctl_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l2_usbd_ctl_init failed!\r\n");
        return;
    }
    
    /* Register new descriptor table here, this action must be done after drv_l2_usbd_ctl_init() */
    drv_l2_usbd_register_descriptor(REG_DEVICE_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_DeviceDescriptor);
    drv_l2_usbd_register_descriptor(REG_CONFIG_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_ConfigDescriptor);
    drv_l2_usbd_register_descriptor(REG_DEVICE_QUALIFIER_DESCRIPTOR_TYPE, (INT8U*) USB_UVC_Qualifier_Descriptor_TBL);
    drv_l2_usbd_register_descriptor(REG_STRING0_DESCRIPTOR_TYPE, (INT8U*) UVC_String0_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING1_DESCRIPTOR_TYPE, (INT8U*) UVC_String1_Descriptor);
    drv_l2_usbd_register_descriptor(REG_STRING2_DESCRIPTOR_TYPE, (INT8U*) UVC_String2_Descriptor);
	drv_l2_usbd_register_descriptor(REG_STRING3_DESCRIPTOR_TYPE, (INT8U*) UVC_String3_Descriptor);
	drv_l2_usbd_register_descriptor(REG_STRING4_DESCRIPTOR_TYPE, (INT8U*) UVC_String4_Descriptor);
	drv_l2_usbd_register_descriptor(REG_STRING5_DESCRIPTOR_TYPE, (INT8U*) UVC_String5_Descriptor);
	
   	/* Init USBD L2 of UVC */
    ret = drv_l2_usbd_uvc_init();
    if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l2_usbd_uvc_init failed!\r\n");
        return;
    }
	
	 /* Set UVC resolution */
    drv_l2_usbd_uvc_config_resolution(uvc_width, uvc_height);
    
    /* Register EP5 sending done call back function */
    drv_l2_usbd_resigter_uvc_frame_done_cbk(_usbd_ep5_send_done_cbk);
#if (UVC_AUDIO_SUPPORT == 1)     
    /* Register EP7 sending done call back function */
    drv_l2_usbd_resigter_uac_frame_done_cbk(_usbd_ep7_send_done_cbk);
#endif    
    /* Register set interface call back */
	drv_l2_usbd_resigter_set_interface_cbk(_usbd_get_set_interface);

    /* Init USBD L1 register layer */
    ret = drv_l1_usbd_uvc_init();
     if(ret == STATUS_FAIL)
    {
        DBG_PRINT("drv_l1_usbd_uvc_init failed!\r\n");
        return;
    }
    
	/* register USBD ISR handler */
	drv_l1_usbd_enable_isr(drv_l1_usbd_isr_handle);
 
    DBG_PRINT("USB UVC device init completed\r\n");
}

void usbd_uvc_uninit(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2	
	INT8U err;
#endif	
	
	drv_l2_usbd_ctl_uninit();
	drv_l2_usbd_uvc_uninit();
	
	_uvc_sensor_uninit();
	_uvc_jpeg_uninit();
	send_video = 0;
	send_audio = 0;
#if (UVC_AUDIO_SUPPORT == 1)
	/* Uninit audio module */
	_uvc_audio_uninit();
#endif

#if _OPERATING_SYSTEM == _OS_UCOS2
	/* Delete UVC task */
	OSTaskSuspend(UVCTASKPRIORITY); 
	OSTimeDly(10);
	OSTaskDelReq(UVCTASKPRIORITY);
	OSTaskDel(UVCTASKPRIORITY);
	
	/* Delete UVC queue */
	OSQFlush(UVCQ);
	OSQDel(UVCQ, OS_DEL_ALWAYS, &err);
	
	/* Delete USB APP task */
	OSTaskSuspend(USBDAPPTASKPRIORITY); 
	OSTimeDly(10);
	OSTaskDelReq(USBDAPPTASKPRIORITY);
	OSTaskDel(USBDAPPTASKPRIORITY);
	
	/* Delete USBAP queue */
	OSQFlush(USBAPQ);
	OSQDel(USBAPQ, OS_DEL_ALWAYS, &err);
#endif	
}	

USBD_CONTROL_BLK usbd_uvc_ctl_blk = 
{
	USBD_UVC_MODE,
	usbd_uvc_init,
	usbd_uvc_uninit
};	

