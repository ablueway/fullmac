#include "videnc_api.h"
#include "jpeg_header.h"

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MSG_VIDEO_ENC_START = 0x300,
	MSG_VIDEO_ENC_STOP,
	MSG_VIDEO_ENC_EXIT	
} MSG_VIDEO_ENC;

INT32S videnc_vid_task_start(VidEncCtrl_t *pVidEnc, INT32U Qvalue)
{
	INT8U err;
	INT32U i;
	
	switch(pVidEnc->conv422_fmt)
	{
	case JPEG_YUV422:
		pVidEnc->Qvalue = jpeg_header_generate(JPEG_IMG_FORMAT_422, Qvalue, pVidEnc->target_w, pVidEnc->target_h);
		break;
		
	case JPEG_GPYUV420:
		pVidEnc->Qvalue = jpeg_header_generate(JPEG_IMG_FORMAT_420, Qvalue, pVidEnc->target_w, pVidEnc->target_h);
		break;
		
	default:
		return STATUS_FAIL;
	}
	
	OSQFlush(pVidEnc->VidEmptyBufQ);
	OSQFlush(pVidEnc->VidReadyBufQ);
	
	for(i=0; i<VIDEO_BUF_NUM; i++) {
		gp_memcpy((INT8S *)pVidEnc->video_buf[i].addr, (INT8S *)jpeg_header_get_addr(), jpeg_header_get_size());	
		err = OSQPost(pVidEnc->VidEmptyBufQ, (void *)&pVidEnc->video_buf[i]);
		if(err != OS_NO_ERR) {
			MSG("%s, VidEmptyBufQFail\r\n", __func__);
		}
	}

	return videnc_send_msg(pVidEnc->VideoQ, pVidEnc->VideoM, MSG_VIDEO_ENC_START);
}

INT32S videnc_vid_task_stop(VidEncCtrl_t *pVidEnc)
{
	return videnc_send_msg(pVidEnc->VideoQ, pVidEnc->VideoM, MSG_VIDEO_ENC_STOP);
}

INT32S videnc_vid_task_exit(VidEncCtrl_t *pVidEnc)
{
	return videnc_send_msg(pVidEnc->VideoQ, pVidEnc->VideoM, MSG_VIDEO_ENC_EXIT);
}

void videnc_vid_task_entry(void *p_arg)
{
	INT8U err, FrameEnd=0, Cnt=0, Times=0;
	INT32U msg, ylen, uvlen;
	INT32U fifo_ready;
	INT32S ret;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)p_arg;
	OS_EVENT *VideoQ = pVidEnc->VideoQ;
	OS_EVENT *VideoM = pVidEnc->VideoM;
	VidBuf_t *pVidBuf=NULL;
	MJpegPara_t mjpeg;
	
	while(1) 
	{
		msg = (INT32U) OSQPend(VideoQ, 0, &err);
		if(err != OS_NO_ERR) {
			MSG("VideoQPendFail = %d\r\n", err);
			while(1);
		}
		
		switch(msg)
		{
		case MSG_VIDEO_ENC_START:
			MSG("MSG_VIDEO_ENC_INIT\r\n");
			mjpeg.jpeg_status = 0;
			mjpeg.Qvalue = pVidEnc->Qvalue;
			mjpeg.yuv_sample_fmt = pVidEnc->conv422_fmt;
			mjpeg.input_width = pVidEnc->target_w;
			mjpeg.input_height = pVidEnc->target_h;
			
			ylen = pVidEnc->target_w * pVidEnc->fifo_line_num;
			switch(pVidEnc->conv422_fmt)
			{
			case JPEG_YUV422:
				uvlen = ylen >> 1;
				break;
				
			case JPEG_GPYUV420:
				uvlen = ylen >> 2;
				break;
				
			default:
				OSMboxPost(VideoM, (void *)ACK_FAIL);
				continue;
			}
				
			mjpeg.input_len = ylen + uvlen + uvlen; 
			mjpeg.encode_size = 0;
			
			FrameEnd = Cnt = 0;
			Times = pVidEnc->target_h / pVidEnc->fifo_line_num;
			if(pVidEnc->target_h % pVidEnc->fifo_line_num) {
				Times++;
			}
			
			MSG("FIFOTimes = %d\r\n", Times);
			OSMboxPost(VideoM, (void *)ACK_OK);
			break;
		
		case MSG_VIDEO_ENC_STOP:
			MSG("MSG_VIDEO_ENC_STOP\r\n");
			FrameEnd = Cnt = 0;
			videnc_jpeg_encode_stop();
			OSMboxPost(VideoM, (void *)ACK_OK);
			break;
			
		case MSG_VIDEO_ENC_EXIT:
			MSG("MSG_VIDEO_ENC_EXIT\r\n");
			OSMboxPost(VideoM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
			
		default:
			// check frame end
			fifo_ready = msg;
			if(fifo_ready & FLAG_FRAME_END) {
				fifo_ready &= ~FLAG_FRAME_END;
				FrameEnd = 1;
			}
			
			// check encode
			if(pVidEnc->state != STATE_ENCODE) {
				OSQPost(pVidEnc->FifoBufQ, (void *)fifo_ready);
				continue;
			}
			
			// check fifo mode or frame mode
			if(Times > 1) {
				goto __fifo_mode;
			}
			
			// frame mode
			// get empty buffer
			pVidBuf = (VidBuf_t *)OSQAccept(pVidEnc->VidEmptyBufQ, &err);
			if((pVidBuf == 0) || (err != OS_NO_ERR)) {
				MSG("S");
				pVidBuf = (VidBuf_t *)OSQAccept(pVidEnc->VidReadyBufQ, &err);
				if((pVidBuf == 0) || (err != OS_NO_ERR)) {
					MSG("GetReadyBufFail\r\n");
					while(1);
				}
			}
			
			// jpeg encode one frame
			mjpeg.jpeg_status = 0;
			mjpeg.input_buffer = fifo_ready;
			mjpeg.output_buffer = pVidBuf->addr + 624;
			ret = videnc_jpeg_encode_once(&mjpeg);
			if(ret < 0) {
				MSG("videnc_jpeg_encode_once fail\r\n");
				while(1);
			}		
			
			// post ready buffer
			pVidBuf->keyflag = 1;
			pVidBuf->size = mjpeg.encode_size + 624;
			err = OSQPost(pVidEnc->VidReadyBufQ, (void *)pVidBuf);
			if(err == OS_NO_ERR) {
				pVidBuf = 0;
			} else {
				MSG("OSQPost VidReadyBufQ Fail\r\n");
			}
			
			// send fifo back
			err = OSQPost(pVidEnc->FifoBufQ, (void *)fifo_ready);
			if(err == OS_NO_ERR) {
				fifo_ready = 0;
			} else {
				MSG("OSQPost FifoBufQ Fail\r\n");
			}
			break;
			
		__fifo_mode:
			// fifo mode 
			// check error
			Cnt++;
			if(FrameEnd && (Cnt != Times)) {
				MSG("Cnt(%d) != Times(%d)\r\n", Cnt, Times);
				FrameEnd = Cnt = 0;
				if(fifo_ready) {
					err = OSQPost(pVidEnc->FifoBufQ, (void *)fifo_ready);
					if(err == OS_NO_ERR) {
						fifo_ready = 0;
					}
				}
				
				if(pVidBuf) {
					err = OSQPost(pVidEnc->VidEmptyBufQ, (void *)pVidBuf);
					if(err == OS_NO_ERR) {
						pVidBuf = 0;
					}
				}
				
				videnc_jpeg_encode_stop();
				continue;
			}
			
			if(Cnt == 1) {
				// get empty buffer
				pVidBuf = (VidBuf_t *)OSQAccept(pVidEnc->VidEmptyBufQ, &err);
				if((pVidBuf == 0) || (err != OS_NO_ERR)) {
					MSG("S");
					pVidBuf = (VidBuf_t *)OSQAccept(pVidEnc->VidReadyBufQ, &err);
					if((pVidBuf == 0) || (err != OS_NO_ERR)) {
						MSG("GetReadyBufFail\r\n");
						while(1);
					}
				}
    			
				// jpeg encode fifo mode
				mjpeg.jpeg_status = 0;
				mjpeg.input_buffer = fifo_ready;
				mjpeg.output_buffer = pVidBuf->addr + 624;
				ret = videnc_jpeg_encode_fifo_start(&mjpeg);
				if(ret < 0) {
					MSG("videnc_jpeg_encode_fifo_start fail\r\n");
					while(1);
				}	
			} else if(Cnt <= Times) {
				mjpeg.input_buffer = fifo_ready;
				if(FrameEnd) {
					FrameEnd = Cnt = 0;
					ret = videnc_jpeg_encode_fifo_stop(&mjpeg);
					if(ret < 0) {
						MSG("videnc_jpeg_encode_fifo_stop fail\r\n");
						while(1);
					}
					
					// post ready buffer
					pVidBuf->keyflag = 1;
					pVidBuf->size = mjpeg.encode_size + 624;
					err = OSQPost(pVidEnc->VidReadyBufQ, (void *)pVidBuf);
					if(err == OS_NO_ERR) {
						pVidBuf = 0;
					} else {
						MSG("OSQPost VidReadyBufQ Fail\r\n");
					}
				} else {
					ret = videnc_jpeg_encode_fifo_once(&mjpeg);
					if(ret < 0) {
						MSG("videnc_jpeg_encode_fifo_once fail\r\n");
						while(1);
					}
				}
			} else {
				while(1);
			}
			
			// send fifo back
			err = OSQPost(pVidEnc->FifoBufQ, (void *)fifo_ready);
			if(err == OS_NO_ERR) {
				fifo_ready = 0;
			} else {
				MSG("OSQPost FifoBufQ Fail\r\n");
			}
			break;	
		}	
	}
	while(1);		
}
