#include <string.h>
#include "avi_encoder_state.h"
#include "drv_l1_csi.h"
#include "drv_l1_cdsp.h"
#include "drv_l2_cdsp.h"
#include "drv_l2_sensor.h"
#include "drv_l2_display.h"
#include "jpeg_header.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/ 
#define C_AVI_ENCODE_STATE_STACK_SIZE	1024
#define AVI_ENCODE_QUEUE_MAX_LEN		5

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MSG_AVI_START_SENSOR = 0x1000,
	MSG_AVI_STOP_SENSOR,
	MSG_AVI_START_ENCODE,
	MSG_AVI_STOP_ENCODE,
	MSG_AVI_CAPTURE_PICTURE,
	MSG_AVI_STORAGE_FULL,
	MSG_AVI_STORAGE_ERR,
	MSG_AVI_ENCODE_STATE_EXIT,
	MSG_AVI_ONE_FRAME_ENCODE,
	MSG_AVI_START_USB_WEBCAM,
	MSG_AVI_STOP_USB_WEBCAM  
} AVI_ENCODE_STATE_ENUM;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void capture_task_entry(void *parm);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT32U AVIEncodeStateStack[C_AVI_ENCODE_STATE_STACK_SIZE];
OS_EVENT *AVIEncodeApQ;
OS_EVENT *avi_encode_ack_m;
void *AVIEncodeApQ_Stack[AVI_ENCODE_QUEUE_MAX_LEN];

INT32S (*pfn_avi_encode_put_data)(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);
static drv_l2_sensor_ops_t *pSencor;


#if AVI_PACKER_LIB_EN == 1
// callback function
static INT32S video_encode_frame_ready(void* workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag)
{
	if(fourcc == 0x63643030) { //"00dc", video frame ready 
	
	} else if(fourcc == 0x62773130) { //"01wb", audio frame ready
	
	} else {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

void video_encode_end(void* workmem)
{
	//add description to avi file
	//int AviPackerV3_AddInfoStr(const char *fourcc, const char *info_string);
	AviPackerV3_AddInfoStr(workmem, "ISRC", "Generplus");
	AviPackerV3_AddInfoStr(workmem, "IART", "Generplus");
	AviPackerV3_AddInfoStr(workmem, "ICOP", "Generplus");
	AviPackerV3_AddInfoStr(workmem, "ICRD", "2010-06-29");	
}
#endif

#if AUDIO_SFX_HANDLE
INT32U video_encode_audio_sfx(INT16U *PCM_Buf, INT32U cbLen)
{
	return (INT32U)PCM_Buf;
}
#endif

static INT32S sensor_init(CHAR *SensorName)
{
	CHAR *p;
	INT32U i;

	// sensor init	
	drv_l2_sensor_init();
	
	// get sensor module
	if(SensorName == NULL) {
		pSencor = drv_l2_sensor_get_ops(0);
		DEBUG_MSG("SensorName = %s\r\n", pSencor->name);
	} else {
		for(i=0; i<3; i++) {
			pSencor = drv_l2_sensor_get_ops(i);
			DEBUG_MSG("SensorName = %s\r\n", pSencor->name);
			if(strcmp(SensorName, pSencor->name) == 0) {
				pSencor = drv_l2_sensor_get_ops(i);
				break;
			}
		}
		
		if(i == 3) {
			return STATUS_FAIL;
		}
	}
	
	// get csi or cdsp
	p = (CHAR *)strrchr((CHAR *)pSencor->name, 'c');
	if(p == 0) {
		return STATUS_FAIL;
	}
	
	if(strncmp((CHAR *)p, "csi", 3) == 0) {
		pAviEncPara->sensor_interface = CSI_INTERFACE;
	} else if(strncmp((CHAR *)p, "cdsp", 4) == 0) {
		pAviEncPara->sensor_interface = CDSP_INTERFACE;
	} else {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

static INT32S sensor_start(INT16U width, INT16U height, INT32U csi_frame1, INT32U csi_frame2)
{
	INT32U i, idx;
	drv_l2_sensor_info_t *pInfo;
	
	for(i=0; i<3; i++) {
		pInfo = pSencor->get_info(i);
		if(pInfo->target_w == width && pInfo->target_h == height) {
			idx = i;
			break;
		}
	}
	
	if(i == 3) {
		return STATUS_FAIL;
	}
	
	pSencor->init();
	pSencor->stream_start(idx, csi_frame1, csi_frame2);
	avi_encode_set_sensor_format(pInfo->output_format);
	return STATUS_OK;
}

static INT32S sensor_stop(void)
{
	pSencor->stream_stop();
	pSencor->uninit();
	return STATUS_OK;
}

static void eof_handle_register(void) 
{
	if(pAviEncPara->sensor_interface == CSI_INTERFACE) {
		drv_l2_sensor_set_path(1);
	#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
		drv_l1_register_csi_cbk(csi_eof_isr);
	#else	
		drv_l1_register_csi_cbk(csi_fifo_isr);
	#endif
	}
	
	if(pAviEncPara->sensor_interface == CDSP_INTERFACE) {
		drv_l2_sensor_set_path(0);
	#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
		pAviEncPara->abBufFlag = 0;
		drv_l2_CdspIsrRegister(C_ISR_EOF, cdsp_eof_isr);
	#else
	
	#endif
	}	
}

static void capture_handle_register(void) 
{
	if(pAviEncPara->sensor_interface == CSI_INTERFACE) {
		drv_l1_register_csi_cbk(csi_capture_isr);
	}
	
	if(pAviEncPara->sensor_interface == CDSP_INTERFACE) {
		pAviEncPara->abBufFlag = 0;
		drv_l2_CdspIsrRegister(C_ISR_EOF, cdsp_capture_isr);
	}
}

// api function
INT32U avi_enc_packer_start(AviEncPacker_t *pAviEncPacker)
{
#if AVI_PACKER_LIB_EN == 1
	INT32S nRet; 
	INT32U bflag;
	
	if(pAviEncPacker == pAviEncPacker0) {
		bflag = C_AVI_ENCODE_PACKER0;
		pAviEncPacker->task_prio = AVI_PACKER0_PRIORITY;
	} else if(pAviEncPacker == pAviEncPacker1) {
		bflag = C_AVI_ENCODE_PACKER1;
		pAviEncPacker->task_prio = AVI_PACKER1_PRIORITY;
	} else {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
	if((avi_encode_get_status() & bflag) == 0) {
		switch(pAviEncPara->source_type)
		{
		case SOURCE_TYPE_FS:
			avi_encode_set_avi_header(pAviEncPacker);
			nRet = AviPackerV3_Open(pAviEncPacker->avi_workmem,
									pAviEncPacker->file_handle, 
									pAviEncPacker->index_handle,
									pAviEncPacker->p_avi_vid_stream_header,
									pAviEncPacker->bitmap_info_cblen,
									pAviEncPacker->p_avi_bitmap_info,
									pAviEncPacker->p_avi_aud_stream_header,
									pAviEncPacker->wave_info_cblen,
									pAviEncPacker->p_avi_wave_info, 
									pAviEncPacker->task_prio,
									pAviEncPacker->file_write_buffer, 
									pAviEncPacker->file_buffer_size, 
									pAviEncPacker->index_write_buffer, 
									pAviEncPacker->index_buffer_size);
			AviPackerV3_SetErrHandler(pAviEncPacker->avi_workmem, avi_packer_err_handle);
			pfn_avi_encode_put_data = AviPackerV3_PutData;
			break;
		case SOURCE_TYPE_USER_DEFINE:
			pAviEncPacker->p_avi_wave_info = pAviEncPacker->avi_workmem;
			pfn_avi_encode_put_data = video_encode_frame_ready;
			break;
		}		
		avi_encode_set_status(bflag);
		DEBUG_MSG("a.AviPackerOpen[0x%x] = 0x%x\r\n", bflag, nRet);
	} else {
		RETURN(STATUS_FAIL);
	}
Return:	
	return nRet;
#else
	return STATUS_FAIL;
#endif
}

INT32U avi_enc_packer_stop(AviEncPacker_t *pAviEncPacker)
{
#if AVI_PACKER_LIB_EN == 1
	INT32S nRet=0;
	INT32U bflag;
	
	if(pAviEncPacker == pAviEncPacker0) {
		bflag = C_AVI_ENCODE_PACKER0;
	} else if(pAviEncPacker == pAviEncPacker1) {
		bflag = C_AVI_ENCODE_PACKER1;
	} else {
		RETURN(STATUS_FAIL);
	}
	
	if(avi_encode_get_status() & bflag) {
		 avi_encode_clear_status(bflag);
		if(avi_encode_get_status() & C_AVI_ENCODE_ERR) {
			avi_encode_clear_status(C_AVI_ENCODE_ERR);
			switch(pAviEncPara->source_type) 
			{
			case SOURCE_TYPE_FS:
				nRet = AviPackerV3_Close(pAviEncPacker->avi_workmem);
				avi_encode_fail_handle_close_file(pAviEncPacker);
				break;
			case SOURCE_TYPE_USER_DEFINE:
				nRet = STATUS_OK;
				break;
			}
		} else {
			switch(pAviEncPara->source_type)
			{
			case SOURCE_TYPE_FS:
				video_encode_end(pAviEncPacker->avi_workmem);
				nRet = AviPackerV3_Close(pAviEncPacker->avi_workmem); 
				avi_encode_close_file(pAviEncPacker);
				break;
			case SOURCE_TYPE_USER_DEFINE:
				nRet = STATUS_OK;
				break;
			} 
		}

		if(nRet < 0) {
			RETURN(STATUS_FAIL);
		}
		DEBUG_MSG("c.AviPackerClose[0x%x] = 0x%x\r\n", bflag, nRet); 
	}
	nRet = STATUS_OK;
Return:	
	return nRet;
#else
	return STATUS_FAIL;
#endif	
}

INT32S vid_enc_preview_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	avi_encode_set_display_scaler();
	
	//alloc memory
	if(avi_encode_memory_alloc() < 0) {
		avi_encode_memory_free();
		DEBUG_MSG("avi memory alloc fail!!!\r\n");
		RETURN(STATUS_FAIL);				
	}
	
	if(avi_packer_memory_alloc() < 0) {
		avi_packer_memory_free();
		DEBUG_MSG("avi packer memory alloc fail!!!\r\n");
		RETURN(STATUS_FAIL);				
	}
	
	// start scaler
	if((avi_encode_get_status()&C_AVI_ENCODE_SCALER) == 0) {
		if(scaler_task_start() < 0) RETURN(STATUS_FAIL);
		avi_encode_set_status(C_AVI_ENCODE_SCALER);
		DEBUG_MSG("a.scaler start\r\n"); 
	}
	
#if AVI_ENCODE_VIDEO_ENCODE_EN == 1 
	// start video
	if((avi_encode_get_status()&C_AVI_ENCODE_VIDEO) == 0) {	
		if(video_encode_task_start() < 0) RETURN(STATUS_FAIL);
		avi_encode_set_status(C_AVI_ENCODE_VIDEO);
		DEBUG_MSG("b.video start\r\n");
	}
#endif

#if AVI_ENCODE_PRE_ENCODE_EN == 1
	if((avi_encode_get_status() & C_AVI_ENCODE_PRE_START) == 0) {
		video_encode_task_post_q(0);	
		avi_encode_set_status(C_AVI_ENCODE_PRE_START); 
	}
#endif

	// start sensor
	if((avi_encode_get_status()&C_AVI_ENCODE_SENSOR) == 0) {
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_set_status(C_AVI_ENCODE_SENSOR);
		DEBUG_MSG("c.sensor start\r\n"); 
	}
	
#if (AVI_AUDIO_ENCODE_EN == 1) && (AVI_ENCODE_PRE_ENCODE_EN == 1) 
	// start audio 
	if(pAviEncAudPara->audio_format && ((avi_encode_get_status()&C_AVI_ENCODE_AUDIO) == 0)) {
		if(avi_audio_record_start() < 0) RETURN(STATUS_FAIL);
		avi_encode_set_status(C_AVI_ENCODE_AUDIO);
		DEBUG_MSG("d.audio start\r\n");
	}
#endif	
Return:	
	return nRet;
}

INT32S vid_enc_preview_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
#if AVI_AUDIO_ENCODE_EN == 1
	// stop audio
	if(avi_encode_get_status()&C_AVI_ENCODE_AUDIO)
	{
		avi_encode_clear_status(C_AVI_ENCODE_AUDIO);
		nRet = avi_audio_record_stop();
		if(nRet < 0) {
			RETURN(STATUS_FAIL);
		}
		DEBUG_MSG("a.audio stop\r\n");
	}
#endif

	// stop sensor
	if(avi_encode_get_status()&C_AVI_ENCODE_SENSOR)
	{
		avi_encode_clear_status(C_AVI_ENCODE_SENSOR);
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		DEBUG_MSG("b.sensor stop\r\n");
	}	
	// stop video
	if(avi_encode_get_status()&C_AVI_ENCODE_VIDEO)
	{	
		avi_encode_clear_status(C_AVI_ENCODE_VIDEO);
		if(video_encode_task_stop() < 0) {
			RETURN(STATUS_FAIL);
		}	
		DEBUG_MSG("c.video stop\r\n");
	}
	// stop scaler
	if(avi_encode_get_status()&C_AVI_ENCODE_SCALER)
	{
		avi_encode_clear_status(C_AVI_ENCODE_SCALER);
		if(scaler_task_stop() < 0) {
			RETURN(STATUS_FAIL);
		}
		DEBUG_MSG("d.scaler stop\r\n");  
	}
Return:	
	//free memory
	avi_encode_memory_free();
	avi_packer_memory_free();
	DEBUG_MSG("e.free memory\r\n");  
	return nRet;
}

INT32S avi_enc_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
#if AVI_AUDIO_ENCODE_EN == 1
	// start audio
	if(pAviEncAudPara->audio_format && ((avi_encode_get_status()&C_AVI_ENCODE_AUDIO) == 0)) {
		if(avi_audio_record_start() < 0) {
			RETURN(STATUS_FAIL);
		}
		avi_encode_set_status(C_AVI_ENCODE_AUDIO);
		DEBUG_MSG("b.audio start\r\n");
	}
#endif
	
#if (AVI_AUDIO_ENCODE_EN == 1) && (AVI_ENCODE_PRE_ENCODE_EN == 1)
	// restart audio 
	if(pAviEncAudPara->audio_format && (avi_encode_get_status()&C_AVI_ENCODE_AUDIO)) {
		if(avi_audio_record_restart() < 0) {
			RETURN(STATUS_FAIL);
		}
		DEBUG_MSG("b.audio restart\r\n");
	}
#endif
	// start avi encode
	if((avi_encode_get_status()&C_AVI_ENCODE_START) == 0) {
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_ENCODE, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_set_status(C_AVI_ENCODE_START);
		avi_encode_set_status(C_AVI_ENCODE_PRE_START);
		DEBUG_MSG("c.encode start\r\n"); 
	}
Return:	
	return nRet;
}

INT32S avi_enc_stop(void)
{
	INT8U err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
#if (AVI_AUDIO_ENCODE_EN == 1) && (AVI_ENCODE_PRE_ENCODE_EN == 0)	
	// stop audio
	if(avi_encode_get_status()&C_AVI_ENCODE_AUDIO) {
		avi_encode_clear_status(C_AVI_ENCODE_AUDIO);
		if(avi_audio_record_stop() < 0) {
			RETURN(STATUS_FAIL);
		}
		DEBUG_MSG("a.audio stop\r\n");
	}
#endif

	// stop avi encode
	if(avi_encode_get_status()&C_AVI_ENCODE_START) {
		avi_encode_clear_status(C_AVI_ENCODE_START);	
	#if AVI_ENCODE_PRE_ENCODE_EN == 0			
		avi_encode_clear_status(C_AVI_ENCODE_PRE_START); 
	#endif	
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_ENCODE, avi_encode_ack_m, 5000, msg, err);	
		DEBUG_MSG("b.encode stop\r\n"); 
	}
	//empty vid_enc_frame_q and post to video_frame_q.
	video_encode_task_empty_q();
Return:	
	return nRet;
}

INT32S avi_enc_save_jpeg(void)
{
	INT8U err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	if((avi_encode_get_status() & C_AVI_ENCODE_PRE_START) == 0) {
		video_encode_task_post_q(0);
		avi_encode_set_status(C_AVI_ENCODE_PRE_START);
	}
	avi_encode_set_status(C_AVI_ENCODE_JPEG);
	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_CAPTURE_PICTURE, avi_encode_ack_m, 5000, msg, err);		
Return: 
	//empty vid_enc_frame_q and post to video_frame_q.
	video_encode_task_empty_q();
#if AVI_ENCODE_PRE_ENCODE_EN == 0   
	avi_encode_clear_status(C_AVI_ENCODE_PRE_START);
	avi_encode_clear_status(C_AVI_ENCODE_JPEG);  
#endif	
	return nRet;
}

INT32S avi_enc_capture(INT16S fd, INT8U quality, INT16U width, INT16U height)
{
	INT8U err;
	INT32S nRet, msg;
	
	pCap = (catpure_t *)gp_malloc_align(sizeof(catpure_t), 4);
	if(pCap == 0) {
		return STATUS_FAIL;
	}
	
	pAviEncPara->skip_cnt = 3;
	gp_memset((INT8S *)pCap, 0x00, sizeof(catpure_t));
	pCap->quality = quality;
	pCap->width = width;
	pCap->height = height;
	
	nRet = width * height * 3;
	pCap->csi_buf = (INT32U)gp_malloc_align(nRet, 32); 
	if(pCap->csi_buf == 0) {
		RETURN(STATUS_FAIL);
	}
	
	pCap->jpeg_buf = pCap->csi_buf + (width * height * 2);
	pCap->Sensor_m = OSMboxCreate(NULL);
	if(pCap->Sensor_m == 0) {
		RETURN(STATUS_FAIL);
	}
	
	pCap->Ack_m = OSMboxCreate(NULL);
	if(pCap->Ack_m == 0) {
		RETURN(STATUS_FAIL);
	}
	
	// create task
	err = OSTaskCreate(capture_task_entry, (void*)pCap, &pCap->Stack[512 - 1], CAPTURE_PRIORITY);
	if(err != OS_NO_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	// wait ack
	msg = (INT32S) OSMboxPend(pCap->Ack_m, 5000/10, &err);
	if(err != OS_NO_ERR || msg == ACK_FAIL) {
		DEBUG_MSG("Ack Fail\r\n"); 
		RETURN(STATUS_FAIL);
	}
		
	// save file
	if(fd >= 0) {
		nRet = save_jpeg_file(fd, pCap->jpeg_buf, pCap->jpeg_len); 
		if(nRet < 0) {
			RETURN(STATUS_FAIL);
		}
	}
	
Return:
	if(pCap->csi_buf) {
		gp_free((void *)pCap->csi_buf);
	}
	
	if(pCap->Sensor_m) {
		OSMboxDel(pCap->Sensor_m, OS_DEL_ALWAYS, &err);
	}
	
	if(pCap->Ack_m) {
		OSMboxDel(pCap->Ack_m, OS_DEL_ALWAYS, &err);
	}

	if(pCap) {
		gp_free((void *)pCap);
		pCap = 0;
	}

	return nRet;
}

// avi encode state function
INT32S avi_encode_state_task_create(INT8U pori)
{
	INT8U err;
	INT32S nRet;
	
	AVIEncodeApQ = OSQCreate(AVIEncodeApQ_Stack, AVI_ENCODE_QUEUE_MAX_LEN);
	if(!AVIEncodeApQ) {
		RETURN(STATUS_FAIL);
	}

	avi_encode_ack_m = OSMboxCreate(NULL);
	if(!avi_encode_ack_m) {
		RETURN(STATUS_FAIL);
	}
	
	err = OSTaskCreate(avi_encode_state_task_entry, (void*) NULL, &AVIEncodeStateStack[C_AVI_ENCODE_STATE_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) {
		RETURN(STATUS_FAIL);
	}
	nRet = STATUS_OK;
Return:
	return nRet;
}

INT32S avi_encode_state_task_del(void)
{
	INT8U err;
	INT32S nRet, msg;

	nRet = STATUS_OK; 
	POST_MESSAGE(AVIEncodeApQ, MSG_AVI_ENCODE_STATE_EXIT, avi_encode_ack_m, 5000, msg, err);	
Return:	
	OSQFlush(AVIEncodeApQ);
	OSQDel(AVIEncodeApQ, OS_DEL_ALWAYS, &err);
	OSMboxDel(avi_encode_ack_m, OS_DEL_ALWAYS, &err);
	AVIEncodeApQ = NULL;
	return nRet;
}

// encode one frame
INT32S avi_enc_one_frame(void)
{
	INT8U err;
	
	err = OSQPost(AVIEncodeApQ, (void*)MSG_AVI_ONE_FRAME_ENCODE);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
} 

// error handle
INT32S avi_enc_storage_full(void)
{
	DEBUG_MSG("avi encode storage full!!!\r\n");
	if(OSQPost(AVIEncodeApQ, (void*)MSG_AVI_STORAGE_FULL) != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	return STATUS_OK;
} 

INT32S avi_packer_err_handle(INT32S ErrCode)
{
	DEBUG_MSG("AviPacker-ErrID = 0x%x!!!\r\n", ErrCode);
	avi_encode_set_status(C_AVI_ENCODE_ERR);	
#if 1
	/* close avi packer task automatic */
	OSQPost(AVIEncodeApQ, (void*)MSG_AVI_STORAGE_ERR);
	return 1;
#else
	/* cloase avipack task by user */
	return 0;
#endif			
}

// avi encode state task function
void avi_encode_state_task_entry(void *para)
{
	INT8U   err, success_flag, key_flag;
	INT32S  nRet;
	INT32U  msg_id, video_frame, video_stream, encode_size;
	INT64S  dwtemp;	
	VidEncFrame_t *pVideo;	
	OS_CPU_SR cpu_sr;
		
	while(1)
	{
		msg_id = (INT32U) OSQPend(AVIEncodeApQ, 0, &err);
		if((!msg_id) || (err != OS_NO_ERR)) {
			continue;
		}
	
		switch(msg_id)
		{
		case MSG_AVI_START_SENSOR:	//sensor
			DEBUG_MSG("[MSG_AVI_START_SENSOR]\r\n");
			// init sensor
			sensor_init(NULL);
			//sensor_init("ov_7670_csi");
			//sensor_init("ov_7670_cdsp");
			
			OSQFlush(cmos_frame_q);
		#if VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FRAME_MODE
			if(pAviEncPara->sensor_interface == CSI_INTERFACE) {
				video_frame = pAviEncVidPara->csi_frame_addr[0];
				video_stream = 0;
				for(nRet = 1; nRet<AVI_ENCODE_CSI_BUFFER_NO; nRet++) {
					OSQPost(cmos_frame_q, (void *) pAviEncVidPara->csi_frame_addr[nRet]);
				}
			} else {
			#if C_DMA_CH == 0
				video_frame = pAviEncVidPara->csi_frame_addr[0];
				video_stream = pAviEncVidPara->csi_frame_addr[1];
				for(nRet = 2; nRet<AVI_ENCODE_CSI_BUFFER_NO; nRet++) {
					OSQPost(cmos_frame_q, (void *) pAviEncVidPara->csi_frame_addr[nRet]);
				}
			#elif C_DMA_CH == 1
				video_frame = pAviEncVidPara->csi_frame_addr[0];
				video_stream = 0;
				for(nRet = 1; nRet<AVI_ENCODE_CSI_BUFFER_NO; nRet++) {
					OSQPost(cmos_frame_q, (void *) pAviEncVidPara->csi_frame_addr[nRet]);
				}
			#elif C_DMA_CH == 2
				video_frame = 0;
				video_stream = pAviEncVidPara->csi_frame_addr[0];
				for(nRet = 1; nRet<AVI_ENCODE_CSI_BUFFER_NO; nRet++) {
					OSQPost(cmos_frame_q, (void *) pAviEncVidPara->csi_frame_addr[nRet]);
				}
			#endif
			}
		#elif VIDEO_ENCODE_MODE == C_VIDEO_ENCODE_FIFO_MODE
			video_frame = avi_encode_get_csi_frame();
			video_stream = avi_encode_get_csi_frame();
			for(nRet = 0; nRet<pAviEncPara->vid_post_cnt; nRet++) {
				pAviEncVidPara->csi_fifo_addr[nRet] = avi_encode_get_csi_frame();		
			}		
		#endif			
			nRet = sensor_start(pAviEncVidPara->sensor_capture_width, 
								pAviEncVidPara->sensor_capture_height,
								video_frame, video_stream);
			if(nRet >= 0) {
				eof_handle_register();
				OSMboxPost(avi_encode_ack_m, (void*)ACK_OK);
			} else {
				OSMboxPost(avi_encode_ack_m, (void*)ACK_FAIL);
			}
			break;

		case MSG_AVI_STOP_SENSOR:
			DEBUG_MSG("[MSG_AVI_STOP_SENSOR]\r\n"); 
			nRet = sensor_stop();
			OSQFlush(cmos_frame_q);
			if(nRet >= 0) {
				OSMboxPost(avi_encode_ack_m, (void*)ACK_OK);
			} else {
				OSMboxPost(avi_encode_ack_m, (void*)ACK_FAIL);
			}
			break;

		case MSG_AVI_START_ENCODE:
			DEBUG_MSG("[MSG_AVI_START_ENCODE]\r\n"); 
			if((avi_encode_get_status()&C_AVI_ENCODE_PRE_START) == 0) {
				nRet = video_encode_task_post_q(0);	
			}
			
			pAviEncPara->ta = 0;
			pAviEncPara->tv = 0;
			pAviEncPara->Tv = 0;
			pAviEncPara->pend_cnt = 0;
			pAviEncPara->post_cnt = 0;
			for(nRet=0; nRet<AVI_ENCODE_VIDEO_BUFFER_NO; nRet++) {
				pAviEncPara->video[nRet].ready_frame = 0;
				pAviEncPara->video[nRet].encode_size = 0;
				pAviEncPara->video[nRet].key_flag = 0;
			}
			
			if(pAviEncPara->AviPackerCur->p_avi_wave_info) {
				pAviEncPara->freq_div = pAviEncAudPara->audio_sample_rate/AVI_ENCODE_TIME_BASE;
				pAviEncPara->tick = (INT64S)pAviEncVidPara->dwRate * pAviEncPara->freq_div;
				pAviEncPara->delta_Tv = pAviEncVidPara->dwScale * pAviEncAudPara->audio_sample_rate;
			} else {
				pAviEncPara->freq_div = 1;
				pAviEncPara->tick = (INT64S)pAviEncVidPara->dwRate * pAviEncPara->freq_div;
				pAviEncPara->delta_Tv = pAviEncVidPara->dwScale * AVI_ENCODE_TIME_BASE;
			}
			
			avi_encode_video_timer_start();
			OSMboxPost(avi_encode_ack_m, (void*)ACK_OK);
			break;

		case MSG_AVI_STOP_ENCODE:
			DEBUG_MSG("[MSG_AVI_STOP_ENCODE]\r\n");  
			avi_encode_video_timer_stop();
			OSMboxPost(avi_encode_ack_m, (void*)ACK_OK);
			break;

		case MSG_AVI_CAPTURE_PICTURE:
			DEBUG_MSG("[MSG_AVI_CAPTURE_PICTURE]\r\n");  
			do {
			video_frame = avi_encode_get_empty(vid_enc_frame_q);
				pVideo = (VidEncFrame_t *)video_frame;
			} while(video_frame == 0);
			
			nRet = save_jpeg_file(pAviEncPara->AviPackerCur->file_handle, 
								pVideo->ready_frame, pVideo->encode_size);					
			avi_encode_post_empty(video_frame_q, pVideo->ready_frame);
			if(nRet < 0) {
				goto AVI_CAPTURE_PICTURE_FAIL;
			}
			OSMboxPost(avi_encode_ack_m, (void*)ACK_OK);
			break;

AVI_CAPTURE_PICTURE_FAIL: 
			OSMboxPost(avi_encode_ack_m, (void*)ACK_FAIL);
			break;

		case MSG_AVI_STORAGE_ERR:
			DEBUG_MSG("[MSG_AVI_STORAGE_ERR]\r\n");  
		case MSG_AVI_STORAGE_FULL:
			DEBUG_MSG("[MSG_AVI_STORAGE_FULL]\r\n"); 
		#if (AVI_AUDIO_ENCODE_EN == 1) && (AVI_ENCODE_PRE_ENCODE_EN == 0)
			// stop audio
			if(avi_encode_get_status()&C_AVI_ENCODE_AUDIO) {
				avi_encode_clear_status(C_AVI_ENCODE_AUDIO);
				avi_audio_record_stop();
			}
		#endif
		
			//stop avi encode timer
			if(avi_encode_get_status()&C_AVI_ENCODE_START) {
				avi_encode_clear_status(C_AVI_ENCODE_START);	
			#if AVI_ENCODE_PRE_ENCODE_EN == 0			
				avi_encode_clear_status(C_AVI_ENCODE_PRE_START); 
			#endif	
				avi_encode_video_timer_stop();
			}
			//close avi packer
			avi_enc_packer_stop(pAviEncPara->AviPackerCur);
			OSQFlush(AVIEncodeApQ);
			OSMboxAccept(avi_encode_ack_m);
			break;

		case MSG_AVI_ENCODE_STATE_EXIT:
			DEBUG_MSG("[MSG_AVI_ENCODE_STATE_EXIT]\r\n");
			OSMboxPost(avi_encode_ack_m, (void*)ACK_OK); 
			OSTaskDel(OS_PRIO_SELF);
			break;

		case MSG_AVI_ONE_FRAME_ENCODE:
			success_flag = 0;
			OS_ENTER_CRITICAL();
			dwtemp = (INT64S)(pAviEncPara->tv - pAviEncPara->Tv);
			OS_EXIT_CRITICAL();
			if(dwtemp > (pAviEncPara->delta_Tv << 2)) {
				goto EncodeNullFrame;
			}
							
			video_frame = avi_encode_get_empty(vid_enc_frame_q);
			if(video_frame == 0) {
				//DEBUG_MSG(DBG_PRINT("one frame = 0x%x\r\n", video_frame));
				goto EndofEncodeOneFrame;
			}
			
			pVideo = (VidEncFrame_t *)video_frame;
			if(pVideo->encode_size == 0) {
				DEBUG_MSG("encode_size = 0x%x\r\n", pVideo->encode_size);
				avi_encode_post_empty(video_frame_q, pVideo->ready_frame);
				goto EndofEncodeOneFrame;
			}
			
			video_stream = pVideo->encode_size + 8 + 2*16;
			if(!avi_encode_disk_size_is_enough(video_stream)) {
				avi_encode_post_empty(video_frame_q, pVideo->ready_frame);
				avi_enc_storage_full();
				goto EndofEncodeOneFrame;
			}
			video_frame = pVideo->ready_frame;
			encode_size = pVideo->encode_size;
			key_flag = pVideo->key_flag;
			nRet = pfn_avi_encode_put_data(	pAviEncPara->AviPackerCur->avi_workmem,
											*(long*)"00dc", 
											encode_size, 
											(void *)video_frame, 
											1, 
											key_flag);
			pVideo->encode_size = 0;
			pVideo->key_flag = 0;
			avi_encode_post_empty(video_frame_q, pVideo->ready_frame);
			if(nRet >= 0) {	
				DEBUG_MSG(".");
				success_flag = 1;
				goto EndofEncodeOneFrame;
			} else { 	
				avi_encode_disk_size_is_enough(-video_stream);
				DEBUG_MSG("VidPutData = %x, size = %d !!!\r\n", nRet-0x80000000, encode_size);
			}
EncodeNullFrame:
			avi_encode_disk_size_is_enough(8+2*16);
			nRet = pfn_avi_encode_put_data(	pAviEncPara->AviPackerCur->avi_workmem,
											*(long*)"00dc", 
											0, 
											(void *)NULL, 
											1, 
											0x00);
			if(nRet >= 0) {
				DEBUG_MSG("N");
				success_flag = 1;
			} else {
				avi_encode_disk_size_is_enough(-(8+2*16));
				DEBUG_MSG("VidPutDataNULL = %x!!!\r\n", nRet-0x80000000);
			}
			
EndofEncodeOneFrame:
			if(success_flag) {
				OS_ENTER_CRITICAL();
				pAviEncPara->Tv += pAviEncPara->delta_Tv;
				OS_EXIT_CRITICAL();
			}
			pAviEncPara->pend_cnt++;
			break;
		}
	}
}

// capture task function 
static void capture_task_entry(void *parm)
{
	extern INT8U jpeg_422_header[624];

	INT8U encflag = 0;
	INT8U err;
	INT32U msg;
	INT32S nRet;
	JpegPara_t jpeg;
	catpure_t *cap = (catpure_t *)parm;
	
	DEBUG_MSG("%s\r\n", __func__);
	
	// stop sensor
	if(avi_encode_get_status()&C_AVI_ENCODE_SENSOR) {
		avi_encode_clear_status(C_AVI_ENCODE_SENSOR);
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_STOP_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		DEBUG_MSG("a.sensor stop.\r\n"); 
	}
	
	// change sensor resolution
	DEBUG_MSG("b.sensor change size.\r\n");
	nRet = sensor_start(cap->width, cap->height, cap->csi_buf, cap->csi_buf);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	} 
	
	// register capture isr
	capture_handle_register();
	
	// get jpeg header
	jpeg_header_generate(JPEG_IMG_FORMAT_422, cap->quality, cap->width, cap->height);
	gp_memcpy((INT8S*)cap->jpeg_buf, (INT8S*)jpeg_header_get_addr(), jpeg_header_get_size()); 
		 
	//wait sensor ready
	DEBUG_MSG("c.wait frame buffer ready\r\n"); 
	msg = (INT32S) OSMboxPend(cap->Sensor_m, 5000/10, &err);
	if(err != OS_NO_ERR || msg == 0) {
		RETURN(STATUS_FAIL);
	}
	
	encflag = 1;	
Return:
	// start sensor
	if((avi_encode_get_status()&C_AVI_ENCODE_SENSOR) == 0) {
		POST_MESSAGE(AVIEncodeApQ, MSG_AVI_START_SENSOR, avi_encode_ack_m, 5000, msg, err);	
		avi_encode_set_status(C_AVI_ENCODE_SENSOR);
		DEBUG_MSG("d.sensor recovery size\r\n"); 
	}
	
	if(encflag) {
		jpeg.quality_value = cap->quality;
		//jpeg.input_format = C_JPEG_FORMAT_YUYV;
		jpeg.width = cap->width;
		jpeg.height = cap->height;
		jpeg.input_buffer_y = cap->csi_buf;
		jpeg.input_buffer_u = 0;
		jpeg.input_buffer_v = 0;
		jpeg.output_buffer = cap->jpeg_buf + sizeof(jpeg_422_header);
		cap->jpeg_len = jpeg_encode_once(&jpeg);
		cap->jpeg_len += sizeof(jpeg_422_header);
		DEBUG_MSG("jpeg encode size = %d\r\n", cap->jpeg_len); 
	}

	if(nRet >= 0) {
		OSMboxPost(cap->Ack_m, (void*)ACK_OK);	
	} else {
		OSMboxPost(cap->Ack_m, (void*)ACK_FAIL);	
	}
	OSTaskDel(OS_PRIO_SELF);
}
