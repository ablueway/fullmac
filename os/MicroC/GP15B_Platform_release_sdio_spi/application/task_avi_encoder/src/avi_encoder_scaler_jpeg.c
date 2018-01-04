#include "avi_encoder_app.h"
#include "drv_l1_scaler.h"
#include "drv_l2_scaler.h"
#include "define.h"
#if APP_QRCODE_BARCODE_EN == 1
#include "task_code_decoder.h"
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define C_SCALER_STACK_SIZE			512
#define	C_JPEG_STACK_SIZE			512
#define C_SCALER_QUEUE_MAX			5
#define C_JPEG_QUEUE_MAX			5

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MSG_SCALER_TASK_INIT = 0x2000,
	MSG_SCALER_TASK_STOP,
	MSG_SCALER_TASK_EXIT
} AVI_ENCODE_SCALER_ENUM;

typedef enum
{
	MSG_VIDEO_ENCODE_TASK_MJPEG_INIT = 0x3000,
	MSG_VIDEO_ENCODE_TASK_STOP,
	MSG_VIDEO_ENCODE_TASK_EXIT
} AVI_ENCODE_VIDEO_ENUM;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT32U	ScalerTaskStack[C_SCALER_STACK_SIZE];
INT32U	JpegTaskStack[C_JPEG_STACK_SIZE];

OS_EVENT *scaler_task_q;
OS_EVENT *scaler_task_ack_m;
OS_EVENT *scaler_frame_q;  
OS_EVENT *display_frame_q;
OS_EVENT *cmos_frame_q;
OS_EVENT *vid_enc_task_q;
OS_EVENT *vid_enc_frame_q;
OS_EVENT *video_frame_q;
OS_EVENT *vid_enc_task_ack_m;
void *scaler_task_q_stack[C_SCALER_QUEUE_MAX];
void *scaler_frame_q_stack[AVI_ENCODE_SCALER_BUFFER_NO];
void *display_frame_q_stack[AVI_ENCODE_DISPALY_BUFFER_NO];
void *cmos_frame_q_stack[AVI_ENCODE_CSI_BUFFER_NO];
void *video_encode_task_q_stack[C_JPEG_QUEUE_MAX];
void *video_encode_frame_q_stack[AVI_ENCODE_VIDEO_BUFFER_NO];
void *video_frame_q_stack[AVI_ENCODE_VIDEO_BUFFER_NO];

INT32U (*videnc_display)(INT16U w, INT16U h, INT32U frame_addr);

// scaler task
INT32S scaler_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;
	
	scaler_task_q = OSQCreate(scaler_task_q_stack, C_SCALER_QUEUE_MAX);
	if(!scaler_task_q) RETURN(STATUS_FAIL);
	
	scaler_task_ack_m = OSMboxCreate(NULL);
	if(!scaler_task_ack_m) RETURN(STATUS_FAIL);
	
	scaler_frame_q = OSQCreate(scaler_frame_q_stack, AVI_ENCODE_SCALER_BUFFER_NO);
	if(!scaler_frame_q) RETURN(STATUS_FAIL);
	
	display_frame_q = OSQCreate(display_frame_q_stack, AVI_ENCODE_DISPALY_BUFFER_NO);
	if(!display_frame_q) RETURN(STATUS_FAIL);
	
	cmos_frame_q = OSQCreate(cmos_frame_q_stack, AVI_ENCODE_CSI_BUFFER_NO);
	if(!cmos_frame_q) RETURN(STATUS_FAIL);	
		
	err = OSTaskCreate(scaler_task_entry, (void *)NULL, &ScalerTaskStack[C_SCALER_STACK_SIZE - 1], pori);	
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S scaler_task_del(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_EXIT, scaler_task_ack_m, 5000, msg, err);
Return:	
	OSQFlush(scaler_task_q);
   	OSQDel(scaler_task_q, OS_DEL_ALWAYS, &err);
	scaler_task_q = NULL;
	
   	OSQFlush(scaler_frame_q);
   	OSQDel(scaler_frame_q, OS_DEL_ALWAYS, &err);
   	scaler_frame_q = NULL;
   	
   	OSQFlush(display_frame_q);
   	OSQDel(display_frame_q, OS_DEL_ALWAYS, &err);
   	display_frame_q = NULL;
   	   	
   	OSQFlush(cmos_frame_q);
   	OSQDel(cmos_frame_q, OS_DEL_ALWAYS, &err);
   	cmos_frame_q = NULL;
   	
	OSMboxDel(scaler_task_ack_m, OS_DEL_ALWAYS, &err);
	return nRet;
}

INT32S scaler_task_start(void)
{
	INT8U  err;
	INT32S i, nRet, msg;
	
	OSQFlush(scaler_frame_q);
	for(i=0; i<AVI_ENCODE_SCALER_BUFFER_NO; i++) {
		avi_encode_post_empty(scaler_frame_q, pAviEncVidPara->scaler_output_addr[i]);
	}
	
	OSQFlush(display_frame_q);
	for(i=0; i<AVI_ENCODE_DISPALY_BUFFER_NO; i++) {
		avi_encode_post_empty(display_frame_q, pAviEncVidPara->display_output_addr[i]);
	}
	
	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_INIT, scaler_task_ack_m, 5000, msg, err);
Return:		
	return nRet;	
}

INT32S scaler_task_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(scaler_task_q, MSG_SCALER_TASK_STOP, scaler_task_ack_m, 5000, msg, err);
Return:		
	return nRet;
}

void scaler_task_entry(void *parm)
{
	INT8U err;
	INT32U msg_id;
	INT32U sensor_frame, scaler_frame, display_frame;
	ScalerFormat_t scale;
#if	AVI_ENCODE_SHOW_TIME == 1
	TIME_T osd_time;
#endif
	
	drv_l2_scaler_init();
	while(1)
	{
		msg_id = (INT32U) OSQPend(scaler_task_q, 0, &err);
		if((err != OS_NO_ERR)||	!msg_id) {
			continue;
		}
			
		switch(msg_id)
		{
		case MSG_SCALER_TASK_INIT:
			DEBUG_MSG("[MSG_SCALER_TASK_INIT]\r\n");
			display_frame = 0;
			pAviEncPara->fifo_enc_err_flag = 0;
			pAviEncPara->fifo_irq_cnt = 0; 
			pAviEncPara->vid_pend_cnt = 0;
			pAviEncPara->vid_post_cnt = 0;
			OSMboxPost(scaler_task_ack_m, (void*)ACK_OK);
			break;
		
		case MSG_SCALER_TASK_STOP:
			DEBUG_MSG("[MSG_SCALER_TASK_STOP]\r\n");
			OSQFlush(scaler_task_q);
			OSMboxPost(scaler_task_ack_m, (void*)ACK_OK);
			break;
				
		case MSG_SCALER_TASK_EXIT:
			DEBUG_MSG("[MSG_SCALER_TASK_EXIT]\r\n");
			OSMboxPost(scaler_task_ack_m, (void*)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
		
		default:
			sensor_frame = msg_id;
		 	if(AVI_ENCODE_DIGITAL_ZOOM_EN == 1 || pAviEncVidPara->scaler_flag > 0) {
		 		scaler_frame = avi_encode_get_empty(scaler_frame_q);
    			if(scaler_frame == 0) {
					avi_encode_post_empty(cmos_frame_q, sensor_frame);
    				DEBUG_MSG("scaler_frame Fail!!!\r\n");
					goto DEFAULT_END;
				}
							
				scale.input_format = pAviEncVidPara->sensor_output_format;
				scale.input_width = pAviEncVidPara->sensor_capture_width;
				scale.input_height = pAviEncVidPara->sensor_capture_height;
				scale.input_visible_width = pAviEncVidPara->sensor_capture_width;
				scale.input_visible_height = pAviEncVidPara->sensor_capture_height;
				scale.input_x_offset = 0;
				scale.input_y_offset = 0;
				
				scale.input_y_addr = sensor_frame;
				scale.input_u_addr = 0;
				scale.input_v_addr = 0;
				
				scale.output_format = C_SCALER_CTRL_OUT_RGB565;
				scale.output_width = pAviEncVidPara->encode_width;
				scale.output_height = pAviEncVidPara->encode_height;
				scale.output_buf_width = pAviEncVidPara->encode_width;
				scale.output_buf_height = pAviEncVidPara->encode_height;
				scale.output_x_offset = 0;
				
				scale.output_y_addr = scaler_frame;
				scale.output_u_addr = 0;
				scale.output_v_addr = 0;
				
			#if AVI_ENCODE_DIGITAL_ZOOM_EN == 1
				scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
				scale.scale_mode = C_SCALER_FULL_SCREEN_BY_DIGI_ZOOM;
				scale.digizoom_m = (INT8U)(pAviEncVidPara->scaler_zoom_ratio * 10);
				scale.digizoom_n = 10; 
			#else		
				scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
				scale.scale_mode = C_SCALER_FULL_SCREEN_BY_RATIO;
				scale.digizoom_m = 10;
				scale.digizoom_n = 10;
			#endif		
    			drv_l2_scaler_trigger(SCALER_0, 1, &scale, 0x008080);
    			
				avi_encode_post_empty(cmos_frame_q, sensor_frame);
			} else {
				scaler_frame = sensor_frame;
			}
		
		#if APP_QRCODE_BARCODE_EN == 1
	    	code_decoder_set_frame(scaler_frame, 
								pAviEncVidPara->encode_width, 
								pAviEncVidPara->encode_height, 
								BITMAP_YUYV);
		#endif
		
		#if AVI_ENCODE_PREVIEW_DISPLAY_EN == 1	
			if(pAviEncVidPara->dispaly_scaler_flag == 0) {
				display_frame = scaler_frame;
			#if	AVI_ENCODE_SHOW_TIME == 1
				cal_time_get(&osd_time);
				cpu_draw_time_osd(osd_time, display_frame, pAviEncVidPara->display_width);
			#endif
		    	if(videnc_display) {
		    		videnc_display(pAviEncVidPara->display_buffer_width, 
		    					   pAviEncVidPara->display_buffer_height, 
		    					   display_frame);
		    	}
			} else {	
				display_frame = avi_encode_get_empty(display_frame_q);
				if(display_frame == 0) {
					DEBUG_MSG("display_frame = 0\r\n");
					goto DEFAULT_END;
				}
				
				scale.input_format = C_SCALER_CTRL_IN_UYVY;//C_SCALER_CTRL_IN_YUYV;
				scale.input_width = pAviEncVidPara->encode_width;
				scale.input_height = pAviEncVidPara->encode_height;
				scale.input_visible_width = pAviEncVidPara->encode_width;
				scale.input_visible_height = pAviEncVidPara->encode_height;
				scale.input_x_offset = 0;
				scale.input_y_offset = 0;
				
				scale.input_y_addr = scaler_frame;
				scale.input_u_addr = 0;
				scale.input_v_addr = 0;
				
				scale.output_format = pAviEncVidPara->display_output_format;
				scale.output_width = pAviEncVidPara->display_width;
				scale.output_height = pAviEncVidPara->display_height;
				scale.output_buf_width = pAviEncVidPara->display_buffer_width;
				scale.output_buf_height = pAviEncVidPara->display_buffer_height;
				scale.output_x_offset = 0;
				
				scale.output_y_addr = display_frame;
				scale.output_u_addr = 0;
				scale.output_v_addr = 0;
				
				scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
				scale.scale_mode = C_SCALER_FULL_SCREEN;
				scale.digizoom_m = 10;
				scale.digizoom_n = 10;
    			drv_l2_scaler_trigger(SCALER_0, 1, &scale, 0x008080);
				
				//post ready frame to video encode task
				if(scaler_frame) {
					OSQPost(vid_enc_task_q, (void *)scaler_frame);//scaler_frame
					scaler_frame = 0;
				}
				
			#if	AVI_ENCODE_SHOW_TIME == 1
				cal_time_get(&osd_time);
				cpu_draw_time_osd(osd_time, display_frame, pAviEncVidPara->display_width);
			#endif			
		    	if(videnc_display) {
		    		videnc_display(pAviEncVidPara->display_buffer_width, 
		    					   pAviEncVidPara->display_buffer_height, 
		    					   display_frame);
		    	}
		    	
				avi_encode_post_empty(display_frame_q, display_frame);				
        	}
		#endif
			
		DEFAULT_END:
			//post ready frame to video encode task
			if(scaler_frame) {
				OSQPost(vid_enc_task_q, (void *)scaler_frame);
				scaler_frame = 0;
			}
			break;	
		}
	}		
}

//	video encode task
INT32S video_encode_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;
	
	vid_enc_task_q = OSQCreate(video_encode_task_q_stack, C_JPEG_QUEUE_MAX);
	if(vid_enc_task_q == 0) {
		RETURN(STATUS_FAIL);
	}
	
	vid_enc_task_ack_m = OSMboxCreate(NULL);
	if(vid_enc_task_ack_m == 0) {
		RETURN(STATUS_FAIL);
	}
	
	vid_enc_frame_q = OSQCreate(video_encode_frame_q_stack, AVI_ENCODE_VIDEO_BUFFER_NO);
	if(vid_enc_frame_q == 0) {
		RETURN(STATUS_FAIL);
	}
	
	video_frame_q = OSQCreate(video_frame_q_stack, AVI_ENCODE_VIDEO_BUFFER_NO);
	if(video_frame_q == 0) {
		RETURN(STATUS_FAIL);
	}
	
	err = OSTaskCreate(video_encode_task_entry, (void *)NULL, &JpegTaskStack[C_JPEG_STACK_SIZE-1], pori); 	
	if(err != OS_NO_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S video_encode_task_del(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_EXIT, vid_enc_task_ack_m, 5000, msg, err);
Return:	
	OSQFlush(vid_enc_task_q);
   	OSQDel(vid_enc_task_q, OS_DEL_ALWAYS, &err);
   	vid_enc_task_q = NULL;
   	
   	OSQFlush(vid_enc_frame_q);
   	OSQDel(vid_enc_frame_q, OS_DEL_ALWAYS, &err);
   	vid_enc_frame_q = NULL;
   	
   	OSQFlush(video_frame_q);
   	OSQDel(video_frame_q, OS_DEL_ALWAYS, &err);
   	video_frame_q = NULL;  	
	OSMboxDel(vid_enc_task_ack_m, OS_DEL_ALWAYS, &err);
	return nRet;
}

INT32S video_encode_task_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_MJPEG_INIT, vid_enc_task_ack_m, 5000, msg, err);
Return:
	return nRet;	
}

INT32S video_encode_task_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(vid_enc_task_q, MSG_VIDEO_ENCODE_TASK_STOP, vid_enc_task_ack_m, 5000, msg, err);
Return:
    return nRet;	
}

INT32S video_encode_task_post_q(INT32U mode)
{
	INT32U i;
	
	if(mode == 0) {
		//avi queue
		OSQFlush(vid_enc_frame_q);
    	OSQFlush(video_frame_q);
    	for(i=0; i<AVI_ENCODE_VIDEO_BUFFER_NO; i++) {
			OSQPost(video_frame_q, (void *) pAviEncVidPara->video_encode_addr[i]);	
		}
	} else {

	}
	return STATUS_OK;
}

INT32S video_encode_task_empty_q(void)
{
	INT32U video_frame;
	VidEncFrame_t * pVideo;
	
	do {
		video_frame = avi_encode_get_empty(vid_enc_frame_q);
		if(video_frame) {
			pVideo = (VidEncFrame_t *)video_frame;
			avi_encode_post_empty(video_frame_q, pVideo->ready_frame);
		}
	} while(video_frame);
	return STATUS_OK;
}

void video_encode_task_entry(void *parm)
{
	INT8U   err, rCnt=0;
	INT32S  header_size=0, encode_size;
	INT32U  msg_id;
	INT32U	video_frame, scaler_frame;
	JpegPara_t jpeg;
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
	INT8U  EncMode, jpeg_start_flag;
	INT16U scaler_height; 
	INT32U input_y_len, input_uv_len, uv_length;
	INT32U y_frame, u_frame, v_frame; 
	INT32S status;
	ScalerFormat_t scale;
#endif
#if	AVI_ENCODE_SHOW_TIME == 1
	TIME_T osd_time;
#endif

	//R_IOC_DIR |= 0x0C; R_IOC_ATT |= 0x0C; R_IOC_O_DATA = 0x0;
	while(1)
	{
		msg_id = (INT32U) OSQPend(vid_enc_task_q, 0, &err);
		if(err != OS_NO_ERR) {
		    continue;
		}
		    
		switch(msg_id)
		{
		case MSG_VIDEO_ENCODE_TASK_MJPEG_INIT:
			DEBUG_MSG("[MSG_VIDEO_ENCODE_TASK_MJPEG_INIT]\r\n");
			rCnt = 0;
		#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
			pAviEncPara->vid_post_cnt = pAviEncVidPara->sensor_capture_height / SENSOR_FIFO_LINE;
			if(pAviEncVidPara->sensor_capture_height % SENSOR_FIFO_LINE) {
				while(1);//this must be no remainder
			}

			if(pAviEncPara->vid_post_cnt > AVI_ENCODE_CSI_FIFO_NO) {
				while(1);//fifo buffer is not enough
			}
			
			jpeg_start_flag = 0;
			scaler_height = pAviEncVidPara->encode_height / pAviEncPara->vid_post_cnt;
			uv_length = pAviEncVidPara->encode_width * (scaler_height + 2);
			input_y_len = pAviEncVidPara->encode_width * scaler_height;
			input_uv_len = input_y_len >> 1; //YUV422
			//input_uv_len = input_y_len >> 2; //YUV420	
		#endif
		#if AVI_ENCODE_VIDEO_ENCODE_EN == 1
			header_size = avi_encode_set_jpeg_quality(pAviEncVidPara->quality_value);
		#endif
			OSMboxPost(vid_enc_task_ack_m, (void*)ACK_OK);
			break;
				 		
		case MSG_VIDEO_ENCODE_TASK_STOP:
			DEBUG_MSG("[MSG_VIDEO_ENCODE_TASK_STOP]\r\n");
			OSQFlush(vid_enc_task_q);
			OSMboxPost(vid_enc_task_ack_m, (void*)ACK_OK);
			break;
			
		case MSG_VIDEO_ENCODE_TASK_EXIT:
			DEBUG_MSG("[MSG_VIDEO_ENCODE_TASK_EXIT]\r\n");
			OSMboxPost(vid_enc_task_ack_m, (void*)ACK_OK);	
			OSTaskDel(OS_PRIO_SELF);
			break;
		
		default:
#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
			scaler_frame = msg_id;
			if(avi_encode_get_status()&C_AVI_ENCODE_PRE_START) {
				video_frame = avi_encode_get_empty(video_frame_q);
			} else {
				goto VIDEO_ENCODE_FRAME_MODE_END;
			}
			
			if(video_frame == 0) {
				//DEBUG_MSG(DBG_PRINT("video_frame = 0x%x\r\n", video_frame));
				goto VIDEO_ENCODE_FRAME_MODE_END;
			}
			
			if(pAviEncVidPara->video_format == C_MJPG_FORMAT) {
			#if	AVI_ENCODE_SHOW_TIME == 1
				if(pAviEncVidPara->dispaly_scaler_flag == 1) {
					cal_time_get(&osd_time);
					cpu_draw_time_osd(osd_time, scaler_frame, pAviEncVidPara->encode_width);
				}
			#endif
				jpeg.quality_value = pAviEncVidPara->quality_value;
				jpeg.input_format = 1;
				jpeg.width = pAviEncVidPara->encode_width;
				jpeg.height = pAviEncVidPara->encode_height;
				jpeg.input_buffer_y = scaler_frame;
				jpeg.input_buffer_u = 0;
				jpeg.input_buffer_v = 0;
				jpeg.output_buffer = video_frame + header_size;
				encode_size = jpeg_encode_once(&jpeg);
				if(encode_size < 0) {
					DEBUG_MSG("encode_size = 0\r\n");
					goto VIDEO_ENCODE_FRAME_MODE_END;
				}
				
				if(pAviEncPara->avi_encode_status & C_AVI_ENCODE_START && 
					pAviEncPara->video[rCnt].key_flag == 0) {
					pAviEncPara->video[rCnt].ready_frame = video_frame;
					pAviEncPara->video[rCnt].encode_size = encode_size + header_size;
					pAviEncPara->video[rCnt].key_flag = AVIIF_KEYFRAME;
					avi_encode_post_empty(vid_enc_frame_q, (INT32U)&pAviEncPara->video[rCnt]);
					rCnt++;
					if(rCnt >= AVI_ENCODE_VIDEO_BUFFER_NO) {
						rCnt = 0;
					}
				} else if(pAviEncPara->avi_encode_status & C_AVI_ENCODE_JPEG) {
					pAviEncPara->video[rCnt].ready_frame = video_frame;
					pAviEncPara->video[rCnt].encode_size = encode_size + header_size;
					pAviEncPara->video[rCnt].key_flag = AVIIF_KEYFRAME;
					avi_encode_post_empty(vid_enc_frame_q, (INT32U)&pAviEncPara->video[rCnt]);
					//clear status
					pAviEncPara->avi_encode_status &= ~C_AVI_ENCODE_JPEG;
				} else {
					//post back to queue
					avi_encode_post_empty(video_frame_q, video_frame);
				}
				
			VIDEO_ENCODE_FRAME_MODE_END:
				//post empty buffer to scale or sensor
				if((AVI_ENCODE_DIGITAL_ZOOM_EN == 0) && (pAviEncVidPara->scaler_flag == 0)) {
					avi_encode_post_empty(cmos_frame_q, scaler_frame);
				} else {
					avi_encode_post_empty(scaler_frame_q, scaler_frame);
				}
			}
			break;
			
#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE	
			if(msg_id & C_AVI_ENCODE_FIFO_ERR) {
				DEBUG_MSG(DBG_PRINT("F1"));
				goto VIDEO_ENCODE_FIFO_MODE_FAIL;
			}
			
			if(pAviEncVidPara->scaler_flag) {
				//R_IOC_O_DATA ^= 0x08;
				scaler_frame = msg_id & (~C_AVI_ENCODE_FRAME_END);
				y_frame = avi_encode_get_empty(scaler_frame_q);
				if(y_frame == 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				u_frame = y_frame + uv_length;   		
				v_frame = u_frame + (uv_length >> 1);
				
				scale.input_format = pAviEncVidPara->sensor_output_format;
				scale.input_width = pAviEncVidPara->sensor_capture_width;
				scale.input_height = SENSOR_FIFO_LINE;
				scale.input_visible_width = 0;
				scale.input_visible_height = 0;
				scale.input_x_offset = 0;
				scale.input_y_offset = 0;
				scale.input_addr_y = scaler_frame;
				scale.input_addr_u = 0;
				scale.input_addr_v = 0;
				
				scale.output_format = C_SCALER_CTRL_OUT_YUV422;
				scale.output_width = pAviEncVidPara->encode_width;
				scale.output_height = scaler_height + 2; //+2 is to fix block line
				scale.output_buf_width = pAviEncVidPara->encode_width;
				scale.output_buf_height = scaler_height + 2; //+2 is to fix block line
				scale.output_x_offset = 0;
				scale.output_addr_y = y_frame;
				scale.output_addr_u = u_frame;
				scale.output_addr_v = v_frame;
				
				scale.fifo_mode = C_SCALER_CTRL_FIFO_DISABLE;
				scale.scale_mode = C_SCALER_FULL_SCREEN;
				scale.digizoom_m = 10;
				scale.digizoom_n = 10;
    			drv_l2_scaler_trigger(SCALER_0, 1, &scale, 0x008080);
				//R_IOC_O_DATA ^= 0x08;
			} else {
				y_frame = msg_id & (~C_AVI_ENCODE_FRAME_END);
				u_frame = v_frame = 0;
			}

			pAviEncPara->vid_pend_cnt++;
			if(msg_id & C_AVI_ENCODE_FRAME_END)	{
				if(pAviEncPara->vid_pend_cnt != pAviEncPara->vid_post_cnt) {
					DEBUG_MSG(DBG_PRINT("F2"));
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				if(jpeg_start_flag == 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
									
				EncMode = 3; //jpeg encode end
				pAviEncPara->vid_pend_cnt = 0; 
			} else if(pAviEncPara->vid_pend_cnt == 1) {
				if(jpeg_start_flag == 1) {
					jpeg_start_flag = 0;
					jpeg_encode_stop();
				}
				EncMode = 1; //jpeg encode start
			} else if(pAviEncPara->vid_pend_cnt < pAviEncPara->vid_post_cnt) {
				if(jpeg_start_flag == 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
					
				EncMode = 2; //jpeg encode once		
			} else {
				// error happen
				goto VIDEO_ENCODE_FIFO_MODE_FAIL;//while(1);
			}
			
			switch(EncMode)
			{
			case 1:
				//DEBUG_MSG(DBG_PRINT("J"));  	
				if(avi_encode_get_status()&C_AVI_ENCODE_PRE_START) {
					video_frame = avi_encode_get_empty(video_frame_q);
				} else {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				if(video_frame == 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				jpeg.jpeg_status = 0;
				jpeg.wait_done = 0;
				jpeg.quality_value = pAviEncVidPara->quality_value;
				if(pAviEncVidPara->scaler_flag) {
					jpeg.input_format = C_JPEG_FORMAT_YUV_SEPARATE;
				} else {
					jpeg.input_format = C_JPEG_FORMAT_YUYV;
				}
				
				jpeg.width = pAviEncVidPara->encode_width;
				jpeg.height = pAviEncVidPara->encode_height;
				jpeg.input_buffer_y = y_frame;
				jpeg.input_buffer_u = u_frame;
				jpeg.input_buffer_v = v_frame;
				jpeg.input_y_len = input_y_len;
				jpeg.input_uv_len = input_uv_len;
				jpeg.output_buffer = video_frame + header_size;	
				status = jpeg_encode_fifo_start(&jpeg);	
				if(status < 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				jpeg_start_flag = 1;
				break;
				
			case 2:
				//DEBUG_MSG(DBG_PRINT("*"));
				jpeg.jpeg_status = status;
				jpeg.wait_done = 0;
				jpeg.input_buffer_y = y_frame;
				jpeg.input_buffer_u = u_frame;
				jpeg.input_buffer_v = v_frame;
				jpeg.input_y_len = input_y_len;
				jpeg.input_uv_len = input_uv_len;
				status = jpeg_encode_fifo_once(&jpeg);
				if(status < 0) { 
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				break;
				
			case 3:
				//DEBUG_MSG(DBG_PRINT("G\r\n"));
				jpeg.jpeg_status = status;
				jpeg.wait_done = 0;
				jpeg.input_buffer_y = y_frame;
				jpeg.input_buffer_u = u_frame;
				jpeg.input_buffer_v = v_frame;
				jpeg.input_y_len = input_y_len;
				jpeg.input_uv_len = input_uv_len;
				encode_size = jpeg_encode_fifo_stop(&jpeg);
				if(encode_size < 0) {
					goto VIDEO_ENCODE_FIFO_MODE_FAIL;
				}
				
				jpeg_start_flag = 0;
				if(pAviEncPara->avi_encode_status & C_AVI_ENCODE_START && 
					pAviEncPara->video[rCnt].key_flag == 0) {
					pAviEncPara->video[rCnt].ready_frame = video_frame;
					pAviEncPara->video[rCnt].encode_size = encode_size + header_size;
					pAviEncPara->video[rCnt].key_flag = AVIIF_KEYFRAME;
					avi_encode_post_empty(vid_enc_frame_q, (INT32U)&pAviEncPara->video[rCnt]);
					rCnt++;
					if(rCnt >= AVI_ENCODE_VIDEO_BUFFER_NO) {
						rCnt = 0;
					}
				} else if(pAviEncPara->avi_encode_status & C_AVI_ENCODE_JPEG) {
					pAviEncPara->video[rCnt].ready_frame = video_frame;
					pAviEncPara->video[rCnt].encode_size = encode_size + header_size;
					pAviEncPara->video[rCnt].key_flag = AVIIF_KEYFRAME;
					avi_encode_post_empty(vid_enc_frame_q, (INT32U)&pAviEncPara->video[rCnt]);
					//clear status
					pAviEncPara->avi_encode_status &= ~C_AVI_ENCODE_JPEG;	
				} else {
					//post back to queue
					avi_encode_post_empty(video_frame_q, video_frame);
				}
				break;
			}
			
			if(pAviEncVidPara->scaler_flag) {
				avi_encode_post_empty(scaler_frame_q, y_frame);
			}
			break;

VIDEO_ENCODE_FIFO_MODE_FAIL:
			pAviEncPara->vid_pend_cnt = 0;
			if(jpeg_start_flag) {
				jpeg_start_flag = 0;
				jpeg_encode_stop();
			}
			//post back to queue
			avi_encode_post_empty(video_frame_q, video_frame);
#endif
		break;
		}
	}	
}
