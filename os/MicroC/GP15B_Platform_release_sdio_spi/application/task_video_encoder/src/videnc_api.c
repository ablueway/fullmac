#include <string.h>
#include "videnc_api.h"
#include "drv_l1_sfr.h"
#include "drv_l1_cache.h"
#include "drv_l1_csi.h"
#include "drv_l1_cdsp.h"
#include "drv_l1_scaler.h"
#include "drv_l1_wrap.h"
#include "drv_l1_conv422to420.h"
#include "drv_l1_conv420to422.h"
#include "drv_l1_jpeg.h"
#include "drv_l1_timer.h"
#include "drv_l2_cdsp.h"
#include "gplib_jpeg.h"
#include "gplib_jpeg_encode.h"
#include "jpeg_header.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define ACK_TIMEOUT				1000	// 10 second
#define SCALER1_PATH_EN			0		// path select

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static VidEncCtrl_t *gpVidEnc;

static INT32U buffer_addrs;
static INT32U sca_fifo_buf[2];
static INT32U scaler0up_AB_fifo_flag;
static INT32U scaler0up_en_flag;

INT32S videnc_send_msg(OS_EVENT *msg_queue, OS_EVENT *ack_mbox, INT32U message)
{
	INT8U err;
	INT32S ack_msg;
	
	// clear ack mbox
	OSMboxAccept(ack_mbox);
	
	// post message
	err = OSQPost(msg_queue, (void *)message);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	// wait ack mbox
	ack_msg = (INT32S)OSMboxPend(ack_mbox, ACK_TIMEOUT, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return ack_msg;
}

INT32S videnc_packer_err_handle(INT32S ErrCode)
{
	if(videnc_state_error(gpVidEnc) >= 0) {
		return 0;
	}
	
	return 1;
}

static void videnc_sync_timer_isr(void)
{
	if(gpVidEnc->audio_format) {
		if((gpVidEnc->tv - gpVidEnc->ta) < gpVidEnc->delta_ta) {
			gpVidEnc->tv += gpVidEnc->tick;
		}	 
	} else {
		gpVidEnc->tv += gpVidEnc->tick;
	}
	
	if((gpVidEnc->tv - gpVidEnc->Tv) > 0) {
		if(gpVidEnc->post_cnt == gpVidEnc->pend_cnt) {
			if(videnc_state_one_frame(gpVidEnc) >= 0) {
				gpVidEnc->post_cnt++;
			}
		}
	}
}

void videnc_sync_timer_start(void)
{
	MSG("%s\r\n", __func__);
	timer_freq_setup(VID_ENC_SYMC_TIMER, VID_ENC_TIME_BASE, 0, videnc_sync_timer_isr);
}

void videnc_sync_timer_stop(void)
{
	MSG("%s\r\n", __func__);
	timer_stop(VID_ENC_SYMC_TIMER);
}

INT32S videnc_memory_allocate(VidEncCtrl_t *pVidEnc)
{
	INT32U i;
	INT32U size;
	
	// display
	size = pVidEnc->disp_w * pVidEnc->disp_h * 2;
	pVidEnc->disp_buf[0] = (INT32U) gp_malloc_align(size * DISP_BUF_NUM, 32);
	if(pVidEnc->disp_buf[0] == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S *)pVidEnc->disp_buf[0], 0x00, size * DISP_BUF_NUM);
	for(i=0; i<DISP_BUF_NUM; i++) {
		pVidEnc->disp_buf[i] = pVidEnc->disp_buf[0] + (size * i);
		MSG("disp_buf[%d] = 0x%x\r\n", i, pVidEnc->disp_buf[i]);
	}
	
	// fifo
	size = pVidEnc->target_w * pVidEnc->fifo_line_num * 2 + 64;
	for(i=0; i<pVidEnc->fifo_buff_num; i++) {
		if(pVidEnc->csi_input == CSI_INTERFACE) {
			pVidEnc->fifo_buf[i] = (INT32U) gp_malloc_align(size, 32);
		}
		else if(pVidEnc->csi_input == CDSP_INTERFACE) {
			pVidEnc->fifo_buf[i] = (INT32U) gp_iram_malloc_align(size, 32);
			if(pVidEnc->fifo_buf[i] == 0) {
				pVidEnc->fifo_buf[i] = (INT32U) gp_malloc_align(size, 32);
			}
		}
		else {
			pVidEnc->fifo_buf[i] = (INT32U) gp_malloc_align(size, 32);
		}
		
		if(pVidEnc->fifo_buf[i] == 0) {
			return STATUS_FAIL;
		}
		
		gp_memset((INT8S *)pVidEnc->fifo_buf[i], 0x00, size);
		MSG("fifo_buf[%d] = 0x%x\r\n", i, pVidEnc->fifo_buf[i]);
	}
	
	// video
	size = pVidEnc->target_w * pVidEnc->target_h / 4;
	//size = (pVidEnc->target_w * pVidEnc->target_h*2) / JPEG_FIFO_LINE;
	pVidEnc->video_buf[0].addr = (INT32U) gp_malloc_align(size * VIDEO_BUF_NUM, 32);
	if(pVidEnc->video_buf[0].addr == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S *)pVidEnc->video_buf[0].addr, 0x00, size * VIDEO_BUF_NUM);
	for(i=0; i<VIDEO_BUF_NUM; i++) {
		pVidEnc->video_buf[i].addr = pVidEnc->video_buf[0].addr + (size * i);
		MSG("video_buf[%d].addr = 0x%x\r\n", i, pVidEnc->video_buf[i].addr);
	}
	
	buffer_addrs = (INT32U)gp_malloc_align(pVidEnc->target_w*pVidEnc->target_h*2, 32); // Scaler A/B Output Buffer
	DBG_PRINT("capture up buf addrs = 0x%x\r\n",buffer_addrs);
	
	sca_fifo_buf[0] = (INT32U)gp_malloc_align(SCALOR_UP_W*JPEG_FIFO_LINE*2*2, 32);
	sca_fifo_buf[1] = sca_fifo_buf[0] + SCALOR_UP_W*JPEG_FIFO_LINE*2;
	DBG_PRINT("sca_fifo_buf[%d] = 0x%x\r\n",0,sca_fifo_buf[0]);
		
	return STATUS_OK;
}

void videnc_memory_free(VidEncCtrl_t *pVidEnc)
{
	INT32S i;
	
	// display
	if(pVidEnc->disp_buf[0]) {
		gp_free((void *)pVidEnc->disp_buf[0]);
		for(i=0; i<DISP_BUF_NUM; i++) {
			pVidEnc->disp_buf[i] = 0;
		}
	}
	
	// fifo
	for(i=0; i<pVidEnc->fifo_buff_num; i++) {
		if(pVidEnc->fifo_buf[i]) {
			gp_free((void *)pVidEnc->fifo_buf[i]);
			pVidEnc->fifo_buf[i] = 0;
		}
	}
	
	// video
	if(pVidEnc->video_buf[0].addr) {
		gp_free((void *)pVidEnc->video_buf[0].addr);
		for(i=0; i<VIDEO_BUF_NUM; i++) {
			pVidEnc->video_buf[i].addr = 0;
		}
	}
}

INT32S videnc_sensor_init(VidEncCtrl_t *pVidEnc, CHAR *SensorName) 
{
	CHAR *p;
	INT32U i;
	drv_l2_sensor_ops_t *SensorDev;
	
	// sensor init	
	drv_l2_sensor_init();
	
	// get sensor module
	if(SensorName == NULL) {
		SensorDev = drv_l2_sensor_get_ops(0);
		MSG("SensorName = %s\r\n", SensorDev->name);
	} else {
		for(i=0; i<3; i++) {
			SensorDev = drv_l2_sensor_get_ops(i);
			MSG("SensorName = %s\r\n", SensorDev->name);
			if(strcmp(SensorName, SensorDev->name) == 0) {
				SensorDev = drv_l2_sensor_get_ops(i);
				break;
			}
		}
		
		if(i == 3) {
			return STATUS_FAIL;
		}
	}
	
	// get csi or cdsp
	pVidEnc->SensorDev = SensorDev;
	p = (CHAR *)strrchr((CHAR *)SensorDev->name, 'c');
	if(p == 0) {
		return STATUS_FAIL;
	}
	
	if(strncmp((CHAR *)p, "csi", 3) == 0) {
		pVidEnc->csi_input = CSI_INTERFACE; 
	}
	else if(strncmp((CHAR *)p, "cdsp", 4) == 0) {
		pVidEnc->csi_input = CDSP_INTERFACE;
	}
	else {
		return STATUS_FAIL;
	}
		
	return STATUS_OK;
}

INT32S videnc_sensor_on(VidEncCtrl_t *pVidEnc)
{
	INT32U i, idx;
	drv_l2_sensor_info_t *pInfo;
	drv_l2_sensor_ops_t *SensorDev = pVidEnc->SensorDev;
	
	if(SensorDev == 0) {
		return STATUS_FAIL;
	}
	
	for(i=0; i<3; i++) {
		pInfo = SensorDev->get_info(i);
		if(pInfo->target_w == pVidEnc->sensor_w && pInfo->target_h == pVidEnc->sensor_h) {
			idx = i;
			break;
		}
	}
	
	if(i == 3) {
		return STATUS_FAIL;
	}
	
	// get the scale input format
	if(pInfo->output_format == V4L2_PIX_FMT_YVYU) {
		pVidEnc->csi_mux_fmt = C_SCALER_CTRL_IN_UYVY;
	}
	else if(pInfo->output_format ==  V4L2_PIX_FMT_VYUY) {
		pVidEnc->csi_mux_fmt = C_SCALER_CTRL_IN_YUYV;
	}
	else {
		MSG("Scaler not support input format!!!\r\n");
		return STATUS_FAIL;
	}
	
	// it need to block sensor signal into wrap fisrt
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		drv_l2_sensor_set_path(1);
	}
	else if(pVidEnc->csi_input == CDSP_INTERFACE) {
		drv_l2_sensor_set_path(0);
	}
	
	SensorDev->init();
	SensorDev->stream_start(idx, DUMMY_BUFFER_ADDRS, NULL);
	return STATUS_OK;
}

INT32S videnc_sensor_off(VidEncCtrl_t *pVidEnc)
{
	drv_l2_sensor_ops_t *SensorDev = pVidEnc->SensorDev;
	
	if(SensorDev == 0) {
		return STATUS_FAIL;
	}
	
	// block sensor signal into wrap
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		drv_l2_sensor_set_path(1);
	}
	else if(pVidEnc->csi_input == CDSP_INTERFACE) {
		drv_l2_sensor_set_path(0);
	}
	
	SensorDev->stream_stop();
	SensorDev->uninit();
	return STATUS_OK;
}

static void videnc_scaler_zoom_set(VidEncCtrl_t *pVidEnc) 
{
	if(pVidEnc->DigiZoomEn) {
		MSG("DigiZoomSet\r\n");
		pVidEnc->DigiZoomEn = 0;
		
	#if SCALER1_PATH_EN == 1
		// scaler1 for target size
		drv_l1_scaler_output_pixels_set(SCALER_1, pVidEnc->xfactor[SCALER_1], pVidEnc->yfactor[SCALER_1], pVidEnc->target_w, pVidEnc->target_h);
		drv_l1_scaler_input_offset_set(SCALER_1, pVidEnc->xoffset[SCALER_1], pVidEnc->yoffset[SCALER_1]);
	#endif
		
		// scaler0 for display size
		drv_l1_scaler_output_pixels_set(SCALER_0, pVidEnc->xfactor[SCALER_0], pVidEnc->yfactor[SCALER_0], pVidEnc->disp_w, pVidEnc->disp_h);
		drv_l1_scaler_input_offset_set(SCALER_0, pVidEnc->xoffset[SCALER_0], pVidEnc->yoffset[SCALER_0]);
	}
}

static void display_update_buffer(void)
{
	INT8U err;
	INT32U disp_addr;
	
	//R_IOD_DIR |= 1 << 9;
	//R_IOD_ATT |= 1 << 9;
	//R_IOD_O_DATA ^= (1 << 9);
	disp_addr = (INT32U)OSQAccept(gpVidEnc->DispBufQ, &err);
	if((disp_addr == 0) || (err != OS_NO_ERR)) {
		return;
	}
	
	drv_l1_scaler_output_addr_set(SCALER_0, disp_addr, 0, 0);
	disp_send_ready_buf(gpVidEnc, gpVidEnc->disp_cur_buf);
	gpVidEnc->disp_cur_buf = disp_addr;
}

static void scaler_wrap_path_stop(void)
{
#if SCALER1_PATH_EN == 1
	drv_l1_scaler_wrap_mode_stop(SCALER_1);
#endif
	drv_l1_scaler_wrap_mode_stop(SCALER_0);
}

static void scaler_wrap_path_restart(void)
{
	drv_l1_wrap_filter_flush(WRAP_CSIMUX);
	drv_l1_wrap_filter_flush(WRAP_CSI2SCA);
	
	// zoom in / out function.
	videnc_scaler_zoom_set(gpVidEnc);
	
#if SCALER1_PATH_EN == 1
	drv_l1_scaler_wrap_mode_restart(SCALER_1);
#endif
	drv_l1_scaler_wrap_mode_restart(SCALER_0);
}

static void videnc_scaler_isr_handle(INT32U scaler0_event, INT32U scaler1_event)
{
	INT32U fifo_ready;
	INT8U err;

#if SCALER1_PATH_EN == 1
	scaler0_event &= C_SCALER_STATUS_DONE;
	scaler1_event &= C_SCALER_STATUS_DONE;
	if((scaler0_event == 0) || (scaler1_event == 0)) {
		return;
	}
#else
	
	if((scaler0_event & C_SCALER_STATUS_DONE) == C_SCALER_STATUS_DONE) {

		if(scaler0up_en_flag==1)
		{
			OSQFlush(gpVidEnc->FifoBufQ);
			fifo_ready = sca_fifo_buf[scaler0up_AB_fifo_flag];
			fifo_ready |= FLAG_FRAME_END;
			scaler0up_AB_fifo_flag = 0;
			err = OSQPost(gpVidEnc->VideoQ, (void *)fifo_ready);
			if(err != OS_NO_ERR) {
				DBG_PRINT("post end Q err\r\n");
			}
		}
		
	}
	else if((scaler0_event & C_SCALER_STATUS_OUTPUT_FULL) == C_SCALER_STATUS_OUTPUT_FULL) {
		
		OSQFlush(gpVidEnc->FifoBufQ);
		fifo_ready = sca_fifo_buf[scaler0up_AB_fifo_flag];

		scaler0up_AB_fifo_flag++;
		if(scaler0up_AB_fifo_flag == 2)
		{
			scaler0up_AB_fifo_flag = 0;
		}
		
		err = OSQPost(gpVidEnc->VideoQ, (void *)fifo_ready);
		if(err != OS_NO_ERR) {
			DBG_PRINT("post line Q err\r\n");
		}
		
		drv_l1_scaler_restart(SCALER_0);
		return;
	}
	else
	{
		return;
	}
#endif
	
	// scaler stop
	scaler_wrap_path_stop();
	// update scaler output address	& post display address
	if(scaler0up_en_flag==0)
	{
		display_update_buffer();
	}

	// state handle
	switch(gpVidEnc->state)
	{		
	case STATE_PREVIEW:
		//MSG("@");
		break;
		
	case STATE_CHANGE_ENCODE:
		// conv422to420 protect
#if 0//select output path at cdsp frame end
	#if SCALER1_PATH_EN == 1
		(*((volatile INT32U *) 0xC0130004)) = ((INT32U)0x1C << 16) | ((INT32U)0xBF << 24);		
	#else
		(*((volatile INT32U *) 0xC0130004)) = ((INT32U)0x1E << 16) | ((INT32U)0xBF << 24);
	#endif
		
		drv_l1_wrap_path_set(WRAP_CSI2SCA, 1, 1); //path-sr enable and path-o enable
		gpVidEnc->state = STATE_ENCODE;
		//MSG("ChageToEncode\r\n");
#endif 
		break;
		
	case STATE_ENCODE:
		//MSG("#");
		break;	
		
	case STATE_CHANGE_PREVIEW:
		drv_l1_wrap_path_set(WRAP_CSI2SCA, 1, 0);//path-sr enable, path-o disable
		drv_l1_conv422_irq_enable(DISABLE);	
		
		// conv422to420 protect
	#if SCALER1_PATH_EN == 1
		(*((volatile INT32U *) 0xC0130004)) = (0x5C << 16);
	#else
		(*((volatile INT32U *) 0xC0130004)) = (0x5E << 16);
	#endif
		
		gpVidEnc->state = STATE_PREVIEW;
		//MSG("ChageToPreview\r\n");
		break;
		
	case STATE_CHANGE_STOP:
		// block sensor signal into wrap
		if(gpVidEnc->csi_input == CSI_INTERFACE) {
			drv_l2_sensor_set_path(1);
		}
		else if(gpVidEnc->csi_input == CDSP_INTERFACE) {
			drv_l2_sensor_set_path(0);
		}
		else {
			return;
		}
		
		drv_l1_wrap_path_set(WRAP_CSIMUX, 0, 0); //path-sr disable, path-o disable
		
		drv_l1_wrap_filter_flush(WRAP_CSIMUX);
		drv_l1_wrap_filter_flush(WRAP_CSI2SCA);
		
		drv_l1_wrap_filter_enable(WRAP_CSIMUX,0);
		drv_l1_wrap_filter_enable(WRAP_CSI2SCA,0);
		
		drv_l1_wrap_protect_enable(WRAP_CSIMUX, 0);
		drv_l1_wrap_protect_enable(WRAP_CSI2SCA,0);
		
	#if SCALER1_PATH_EN	== 1
		drv_l1_scaler_stop(SCALER_1);
	#endif	
		drv_l1_scaler_stop(SCALER_0);
		gpVidEnc->state = STATE_STOP;
		//MSG("ChageToStop\r\n");
		return;
	}
	
#if SCALER1_PATH_EN == 1
	scaler_wrap_path_restart();
#else
	if(gpVidEnc->csi_input == CSI_INTERFACE) {
		gpVidEnc->done_flag = 0;
		scaler_wrap_path_restart();
	}
	else if(gpVidEnc->csi_input == CDSP_INTERFACE) {
		if(scaler0up_en_flag==0)
		{
			gpVidEnc->done_flag = SCALER_DONE;
		}
		else
			gpVidEnc->done_flag = 0;
	}
	else {
		return;
	}
#endif	
}

static void videnc_cdsp_eof_handle(void)
{
	//R_IOD_DIR |= 1 << 8;
	//R_IOD_ATT |= 1 << 8;
	//R_IOD_O_DATA ^= (1 << 8);
	if(gpVidEnc->done_flag == SCALER_DONE) {
		gpVidEnc->done_flag = 0;
		scaler_wrap_path_restart();
	}

	if(gpVidEnc->state == STATE_CHANGE_ENCODE) {
		(*((volatile INT32U *) 0xC0130004)) = ((INT32U)0x1E << 16) | ((INT32U)0xBF << 24);
		drv_l1_wrap_path_set(WRAP_CSI2SCA, 1, 1); //path-sr enable and path-o enable
		gpVidEnc->state = STATE_ENCODE;
	}
	

	
}

static void videnc_cdsp_overflow_handle(void)
{
	MSG("overflow=%d\r\n", gpVidEnc->state);
	
	(*((volatile INT32U *) 0xC0130004)) = (0xFF<<16);
	
	gpVidEnc->done_flag = 0;
	scaler_wrap_path_stop();
	scaler_wrap_path_restart();
	
}

static void videnc_csi_eof_handle(INT32U event)
{
	if(event == CSI_SENSOR_FIFO_OVERFLOW_EVENT) {
		videnc_cdsp_overflow_handle();
	}
	
	if(event == CSI_SENSOR_FRAME_END_EVENT) {
		videnc_cdsp_eof_handle();
	}
}

INT32S videnc_scaler0up_on(VidEncCtrl_t *pVidEnc)
{

	INT32U xfactor, yfactor;
	SCALER_MAS scaler0_mas;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	gpVidEnc = pVidEnc;
	scaler0up_en_flag = 1;
	
	scaler0up_AB_fifo_flag = 0;
	
	gp_memset((INT8S*)&scaler0_mas, 0x00, sizeof(SCALER_MAS));
	drv_l1_scaler_lock(SCALER_0);
	drv_l1_scaler_init(SCALER_0);
	drv_l1_scaler_isr_callback_set(videnc_scaler_isr_handle);
	
	// cdsp over flow register
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		drv_l1_register_csi_cbk(videnc_csi_eof_handle);
	}
	else if(pVidEnc->csi_input == CDSP_INTERFACE) {
		drv_l2_CdspIsrRegister(C_ISR_EOF, videnc_cdsp_eof_handle);
		drv_l2_CdspIsrRegister(C_ISR_OVERFLOW, videnc_cdsp_overflow_handle);
	}
	drv_l1_conv420_init(); 
	drv_l1_conv420_reset();
	drv_l1_conv420_path(CONV420_TO_SCALER0);
	drv_l1_conv420_input_A_addr_set(buffer_addrs);
	drv_l1_conv420_input_pixels_set(pVidEnc->target_w);
	drv_l1_conv420_convert_enable(1); //420 -> 422
	drv_l1_conv420_start();

	// scaler 0 for display size
	scaler0_mas.mas_1 = MAS_EN_READ;
	scaler0_mas.mas_0 = MAS_EN_WRITE;	
	//scaler0_mas.mas_0 = MAS_EN_WRITE | MAS_EN_READ;
	
	
	drv_l1_scaler_mas_set(SCALER_0, &scaler0_mas);
	
	drv_l1_scaler_input_pixels_set(SCALER_0, pVidEnc->target_w, pVidEnc->target_h);
	//drv_l1_scaler_input_visible_pixels_set(SCALER_0, pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_scaler_input_offset_set(SCALER_0, 0, 0);
	
	xfactor = (pVidEnc->target_w << 16) / SCALOR_UP_W;//pVidEnc->disp_w;
	yfactor = (pVidEnc->target_h << 16) / SCALOR_UP_H;//pVidEnc->disp_h;
	
	//drv_l1_scaler_output_pixels_set(SCALER_0, xfactor, yfactor, pVidEnc->disp_w, pVidEnc->disp_h);
	drv_l1_scaler_output_pixels_set(SCALER_0, xfactor, yfactor, SCALOR_UP_W, SCALOR_UP_H);
	drv_l1_scaler_output_offset_set(SCALER_0, 0);
	
	//drv_l1_scaler_input_A_addr_set(SCALER_0, DUMMY_BUFFER_ADDRS, 0, 0);
	drv_l1_scaler_input_A_addr_set(SCALER_0, buffer_addrs, 0, 0);
	
	drv_l1_scaler_output_addr_set(SCALER_0, sca_fifo_buf[0] , 0, 0); 
	
	drv_l1_scaler_input_format_set(SCALER_0, C_SCALER_CTRL_IN_YUYV); 
	drv_l1_scaler_output_format_set(SCALER_0, C_SCALER_CTRL_OUT_YUYV);
	
	drv_l1_scaler_fifo_line_set(SCALER_0, C_SCALER_CTRL_FIFO_DISABLE);
	drv_l1_scaler_output_fifo_line_set(SCALER_0, C_SCALER_CTRL_OUT_FIFO_16LINE);
	//drv_l1_scaler_output_fifo_line_set(SCALER_0, C_SCALER_CTRL_OUT_FIFO_DISABLE);
	drv_l1_scaler_out_of_boundary_mode_set(SCALER_0, 1);	
	drv_l1_scaler_out_of_boundary_color_set(SCALER_0, 0x000000);
	 
	// wait frame end and change the path
	MSG("wait v blinking.\r\n");
	OS_ENTER_CRITICAL();
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		R_TGR_IRQ_STATUS = MASK_CSI_FRAME_END_FLAG;
		while(1) {
			if(R_TGR_IRQ_STATUS & MASK_CSI_FRAME_END_ENABLE) {
				R_TGR_IRQ_STATUS = MASK_CSI_FRAME_END_FLAG;
				drv_l2_sensor_set_path(0);
				break;
			}
		}
	} else {
		R_CDSP_INT = CDSP_EOF;
		while(1) {
			if(R_CDSP_INT & CDSP_EOF) {
				R_CDSP_INT = CDSP_EOF;
				drv_l2_sensor_set_path(1);
				break;
			}
		}
	}	
	
	pVidEnc->state = STATE_ENCODE;
	OS_EXIT_CRITICAL();
	
	OSSchedLock();
	drv_l1_scaler_start(SCALER_0, 0);
	OSSchedUnlock();

	MSG("scaler  start.\r\n");
	return STATUS_OK;
}

INT32S videnc_scaler0up_off(VidEncCtrl_t *pVidEnc)
{
	MSG("%s.\r\n", __func__);
	
	if(gpVidEnc->csi_input == CSI_INTERFACE) {
			drv_l2_sensor_set_path(1);
		}
		else if(gpVidEnc->csi_input == CDSP_INTERFACE) {
			drv_l2_sensor_set_path(0);
		}
		
		drv_l1_wrap_path_set(WRAP_CSIMUX, 0, 0); //path-sr disable, path-o disable
		
		drv_l1_wrap_filter_flush(WRAP_CSIMUX);
		drv_l1_wrap_filter_flush(WRAP_CSI2SCA);
		
		drv_l1_wrap_filter_enable(WRAP_CSIMUX,0);
		drv_l1_wrap_filter_enable(WRAP_CSI2SCA,0);
		
		drv_l1_wrap_protect_enable(WRAP_CSIMUX, 0);
		drv_l1_wrap_protect_enable(WRAP_CSI2SCA,0);
		
	#if SCALER1_PATH_EN	== 1
		drv_l1_scaler_stop(SCALER_1);
	#endif	
		drv_l1_scaler_stop(SCALER_0);
		gpVidEnc->state = STATE_STOP;
	
	drv_l1_scaler_stop(SCALER_0);
	drv_l1_scaler_unlock(SCALER_0);

	return STATUS_OK;
}

INT32S videnc_preview_on(VidEncCtrl_t *pVidEnc)
{
	INT8U err;
	INT32U i, filter_addrs_size;
	INT32U xfactor, yfactor;
#if SCALER1_PATH_EN == 1
	SCALER_MAS scaler1_mas;
#endif
	SCALER_MAS scaler0_mas;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	// post display buffer
	OSQFlush(pVidEnc->DispBufQ);
	for(i=0; i<DISP_BUF_NUM; i++) {
		err= OSQPost(pVidEnc->DispBufQ, (void *)pVidEnc->disp_buf[i]);
		if(err != OS_NO_ERR) {
			MSG("DispBufQFail\r\n");
		}
	}
	
	gpVidEnc = pVidEnc;
	scaler0up_en_flag = 0;

#if SCALER1_PATH_EN == 1
	gp_memset((INT8S*)&scaler1_mas, 0x00, sizeof(SCALER_MAS));
	drv_l1_scaler_lock(SCALER_1);
	drv_l1_scaler_init(SCALER_1);
#endif
	gp_memset((INT8S*)&scaler0_mas, 0x00, sizeof(SCALER_MAS));
	drv_l1_scaler_lock(SCALER_0);
	drv_l1_scaler_init(SCALER_0);
	drv_l1_scaler_isr_callback_set(videnc_scaler_isr_handle);
	
	// cdsp over flow register
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		drv_l1_register_csi_cbk(videnc_csi_eof_handle);
	}
	else if(pVidEnc->csi_input == CDSP_INTERFACE) {
		drv_l2_CdspIsrRegister(C_ISR_EOF, videnc_cdsp_eof_handle);
		drv_l2_CdspIsrRegister(C_ISR_OVERFLOW, videnc_cdsp_overflow_handle);
	}
		
	// WRAP_CSIMUX 
	filter_addrs_size = pVidEnc->sensor_w * pVidEnc->sensor_h * 2;
	drv_l1_wrap_addr_set(WRAP_CSIMUX, DUMMY_BUFFER_ADDRS);
	drv_l1_wrap_filter_addr_set(WRAP_CSIMUX, DUMMY_BUFFER_ADDRS, filter_addrs_size);	
#if SCALER1_PATH_EN	== 1
	drv_l1_wrap_path_set(WRAP_CSIMUX, 1, 0); //path-sr enable, path-o disable
#else
	drv_l1_wrap_path_set(WRAP_CSIMUX, 0, 1); //path-sr disable, path-o enable
#endif	
	drv_l1_wrap_filter_enable(WRAP_CSIMUX, 1);
	drv_l1_wrap_filter_flush(WRAP_CSIMUX);
	
	// protect mechanism
#if SCALER1_PATH_EN == 1
	(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);		
#else
	(*((volatile INT32U *) 0xC0130004)) = (0x5E<<16);
#endif
	
	drv_l1_wrap_protect_enable(WRAP_CSIMUX, 1);
	drv_l1_wrap_protect_pixels_set(WRAP_CSIMUX, pVidEnc->sensor_w, pVidEnc->sensor_h);
	
	// WRAP_CSI2SCA 
	drv_l1_wrap_addr_set(WRAP_CSI2SCA, DUMMY_BUFFER_ADDRS);
	drv_l1_wrap_path_set(WRAP_CSI2SCA, 1, 0);	//path-sr enable, path-o disable
	drv_l1_wrap_filter_enable(WRAP_CSI2SCA, 1);
	drv_l1_wrap_filter_flush(WRAP_CSI2SCA);	
	
	// scaler 1 for target size
#if SCALER1_PATH_EN == 1
	scaler1_mas.mas_2 = MAS_EN_READ;
	scaler1_mas.mas_3 = MAS_EN_WRITE;
	drv_l1_scaler_mas_set(SCALER_1, &scaler1_mas);
	
	drv_l1_scaler_input_pixels_set(SCALER_1, pVidEnc->sensor_w, pVidEnc->sensor_h);
	drv_l1_scaler_input_visible_pixels_set(SCALER_1, pVidEnc->sensor_w, pVidEnc->sensor_h);
	drv_l1_scaler_input_offset_set(SCALER_1, 0, 0);
	
	xfactor = (pVidEnc->sensor_w << 16) / pVidEnc->target_w;
	yfactor = (pVidEnc->sensor_h << 16) / pVidEnc->target_h;
	drv_l1_scaler_output_pixels_set(SCALER_1, xfactor, yfactor, pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_scaler_output_offset_set(SCALER_1, 0);
	
	drv_l1_scaler_input_A_addr_set(SCALER_1, DUMMY_BUFFER_ADDRS, 0, 0);
	drv_l1_scaler_output_addr_set(SCALER_1, DUMMY_BUFFER_ADDRS, 0, 0); 
	drv_l1_scaler_input_format_set(SCALER_1, pVidEnc->csi_mux_fmt); //C_SCALER_CTRL_IN_YUYV
	drv_l1_scaler_output_format_set(SCALER_1, C_SCALER_CTRL_OUT_YUYV);
	drv_l1_scaler_fifo_line_set(SCALER_1, C_SCALER_CTRL_FIFO_DISABLE);
	
	drv_l1_scaler_out_of_boundary_mode_set(SCALER_1, 1);	
	drv_l1_scaler_out_of_boundary_color_set(SCALER_1, 0x008080);
#else
	if((pVidEnc->sensor_w != pVidEnc->target_w) || (pVidEnc->sensor_h != pVidEnc->target_h)) {
		MSG("target size != sensor size\r\n");
		pVidEnc->target_w = pVidEnc->sensor_w;
		pVidEnc->target_h = pVidEnc->sensor_h;
	}
#endif
	
	// scaler 0 for display size
	scaler0_mas.mas_0 = MAS_EN_WRITE;
	scaler0_mas.mas_2 = MAS_EN_READ;
	drv_l1_scaler_mas_set(SCALER_0, &scaler0_mas);
	
	drv_l1_scaler_input_pixels_set(SCALER_0, pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_scaler_input_visible_pixels_set(SCALER_0, pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_scaler_input_offset_set(SCALER_0, 0, 0);
	
	xfactor = (pVidEnc->target_w << 16) / pVidEnc->disp_w;
	yfactor = (pVidEnc->target_h << 16) / pVidEnc->disp_h;
	
	drv_l1_scaler_output_pixels_set(SCALER_0, xfactor, yfactor, pVidEnc->disp_w, pVidEnc->disp_h);
	drv_l1_scaler_output_offset_set(SCALER_0, 0);
	
	drv_l1_scaler_input_A_addr_set(SCALER_0, DUMMY_BUFFER_ADDRS, 0, 0);

	pVidEnc->disp_cur_buf = (INT32U)OSQAccept(pVidEnc->DispBufQ, &err);
	if((pVidEnc->disp_cur_buf == 0) || (err != OS_NO_ERR)) {
		return STATUS_FAIL;
	}
	
	MSG("disp_cur_buf = 0x%x\r\n", pVidEnc->disp_cur_buf);
	drv_l1_scaler_output_addr_set(SCALER_0, pVidEnc->disp_cur_buf, 0, 0); 
#if SCALER1_PATH_EN == 1
	drv_l1_scaler_input_format_set(SCALER_0, C_SCALER_CTRL_IN_YUYV);
#else
	drv_l1_scaler_input_format_set(SCALER_0, pVidEnc->csi_mux_fmt); //C_SCALER_CTRL_IN_YUYV
#endif
	drv_l1_scaler_output_format_set(SCALER_0, pVidEnc->disp_fmt); //C_SCALER_CTRL_OUT_RGB565	
	drv_l1_scaler_fifo_line_set(SCALER_0, C_SCALER_CTRL_FIFO_DISABLE);
	//drv_l1_scaler_output_fifo_line_set(SCALER_0, C_SCALER_CTRL_OUT_FIFO_DISABLE);
	drv_l1_scaler_out_of_boundary_mode_set(SCALER_0, 1);	
	drv_l1_scaler_out_of_boundary_color_set(SCALER_0, 0x000000);
	 
	// wait frame end and change the path
	MSG("wait v blinking.\r\n");
	OS_ENTER_CRITICAL();
	if(pVidEnc->csi_input == CSI_INTERFACE) {
		R_TGR_IRQ_STATUS = MASK_CSI_FRAME_END_FLAG;
		while(1) {
			if(R_TGR_IRQ_STATUS & MASK_CSI_FRAME_END_ENABLE) {
				R_TGR_IRQ_STATUS = MASK_CSI_FRAME_END_FLAG;
				drv_l2_sensor_set_path(0);
				break;
			}
		}
	} else {
		R_CDSP_INT = CDSP_EOF;
		while(1) {
			if(R_CDSP_INT & CDSP_EOF) {
				R_CDSP_INT = CDSP_EOF;
				drv_l2_sensor_set_path(1);
				break;
			}
		}
	}	
	
	pVidEnc->state = STATE_PREVIEW;
	OS_EXIT_CRITICAL();
	
	OSSchedLock();
#if SCALER1_PATH_EN == 1
	drv_l1_scaler_start(SCALER_1, 0);
#endif
	drv_l1_scaler_start(SCALER_0, 0);
	OSSchedUnlock();

	MSG("preview start.\r\n");
	return STATUS_OK;
}

INT32S videnc_preview_off(VidEncCtrl_t *pVidEnc)
{
	INT32S i;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	OS_ENTER_CRITICAL();
	pVidEnc->state = STATE_CHANGE_STOP;
	OS_EXIT_CRITICAL();
	
	MSG("wait to stop preview.\r\n");
	for(i=0; i<ACK_TIMEOUT; i++){
		OSTimeDly(1);
		if(pVidEnc->state == STATE_STOP) {
			break;
		}
	}
	
#if SCALER1_PATH_EN == 1
	drv_l1_scaler_stop(SCALER_1);
	drv_l1_scaler_unlock(SCALER_1);
#endif
	drv_l1_scaler_stop(SCALER_0);
	drv_l1_scaler_unlock(SCALER_0);
	
	if(i == ACK_TIMEOUT) {
		MSG("preview stop fail.\r\n");
		return STATUS_FAIL;
	}
	
	MSG("preview stop ok.\r\n");
	return STATUS_OK;
}

static void videnc_conv422_isr_handle(INT32U event)
{
	INT8U err = 0;
	INT32U fifo_ready, fifo_empty;
	
	if(event & CONV422_CHANGE_FIFO) {
		// get empty fifo
		fifo_empty = (INT32U)OSQAccept(gpVidEnc->FifoBufQ, &err);
		if(fifo_empty == 0 || (err != OS_NO_ERR)) {
			goto __exit;
		}
		
		// check a or b buffer ready
		if(event & CONV422_IRQ_BUF_A) {
			fifo_ready = drv_l1_conv422_output_A_addr_get();
			drv_l1_conv422_output_A_addr_set(fifo_empty);
		} else {
			fifo_ready = drv_l1_conv422_output_B_addr_get();
			drv_l1_conv422_output_B_addr_set(fifo_empty);
		}
		
		// check and set frame end
		if(event & CONV422_FRAME_END) {
			fifo_ready |= FLAG_FRAME_END;
		}
		
		// post to video task
		err = OSQPost(gpVidEnc->VideoQ, (void *)fifo_ready);
		if(err != OS_NO_ERR) {
			goto __exit;
		}
	}
	//MSG("*");
	return;
	
__exit:
	MSG("Conv422Fail=0x%x, 0x%x\r\n", event, err);	
}

static void videnc_conv422_to_framebuf_isr_handle(INT32U event)
{
	INT8U err = 0;
	INT32U fifo_ready;
	
	fifo_ready = buffer_addrs;
	if(event & CONV422_FRAME_END) {
		err = OSQPost(gpVidEnc->VidReadyBufQ, (void *)fifo_ready);
	}
	return;

}

INT32S videnc_fifo_to_framebuf(VidEncCtrl_t *pVidEnc)
{
	INT32S i;
	INT32U fifo_a_buf;
	INT32U fifo_b_buf;
	OS_CPU_SR cpu_sr;
	//pVidEnc_sca = pVidEnc;
	
	// init flag
	pVidEnc->done_flag = 0;
	fifo_a_buf = buffer_addrs;
	fifo_b_buf = buffer_addrs;

	// conv422 init
	drv_l1_conv422_register_callback(videnc_conv422_to_framebuf_isr_handle);
	drv_l1_conv422_init();
	
	drv_l1_conv422_input_pixels_set(pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_conv422_input_fmt_set(CONV422_FMT_UYVY);
	//drv_l1_conv422_fifo_line_set(pVidEnc->fifo_line_num);
	drv_l1_conv422_fifo_line_set(pVidEnc->target_h);//frame mode, fifo_line = target_h
	drv_l1_conv422_output_A_addr_set(fifo_a_buf);
	drv_l1_conv422_output_B_addr_set(fifo_b_buf);
	switch(pVidEnc->conv422_fmt)
	{
	case JPEG_YUV422:
		drv_l1_conv422_output_format_set(CONV422_FMT_422);
		break;
		
	case JPEG_GPYUV420:
		drv_l1_conv422_output_format_set(CONV422_FMT_420);
		break;
		
	default:
		return STATUS_FAIL;
	}
	
	drv_l1_conv422_mode_set(CONV422_FIFO_MODE);//none useful
	drv_l1_conv422_bypass_enable(DISABLE);
	drv_l1_conv422_irq_enable(ENABLE);
	drv_l1_conv422_reset();
	
	OS_ENTER_CRITICAL();
	pVidEnc->state = STATE_CHANGE_ENCODE;
	OS_EXIT_CRITICAL();
	
	MSG("wait 422 en start\r\n");
	for(i=0; i<ACK_TIMEOUT; i++) {
		OSTimeDly(1);
		if(pVidEnc->state == STATE_ENCODE) {
			//DBG_PRINT("chg to en\r\n");
			break;
		}
	}
	
	if(i == ACK_TIMEOUT) {
		MSG("422en start fail.\r\n");
		return STATUS_FAIL;
	}
	
	MSG("start ok.\r\n");
	return STATUS_OK;
}

INT32S videnc_fifo_to_framebuf_off(VidEncCtrl_t *pVidEnc)
{
	INT32S i;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	OS_ENTER_CRITICAL();
	pVidEnc->state = STATE_CHANGE_PREVIEW;
	OS_EXIT_CRITICAL();
	
	MSG("wait to stop 422en.\r\n");
	for(i=0; i<ACK_TIMEOUT; i++) {
		OSTimeDly(1);
		if(pVidEnc->state == STATE_PREVIEW) {
			break;
		}
	}

	if(i == ACK_TIMEOUT) {
		MSG("422en stop fail.\r\n");
		return STATUS_FAIL;
	}
	
	//MSG("422en stop .\r\n");
	return STATUS_OK;
}

INT32S videnc_encode_on(VidEncCtrl_t *pVidEnc)
{
	INT8U err;
	INT32S i;
	INT32U fifo_a_buf;
	INT32U fifo_b_buf;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	// init flag
	pVidEnc->done_flag = 0;
		
	// post fifo
	fifo_a_buf = pVidEnc->fifo_buf[0];
	fifo_b_buf = pVidEnc->fifo_buf[1];
	MSG("fifo_a_buf = 0x%x\r\n", fifo_a_buf);
	MSG("fifo_b_buf = 0x%x\r\n", fifo_b_buf);
	
	OSQFlush(pVidEnc->FifoBufQ); 
	for(i=2; i<pVidEnc->fifo_buff_num; i++) {
		err = OSQPost(pVidEnc->FifoBufQ, (void *)pVidEnc->fifo_buf[i]);
		if(err != OS_NO_ERR) {
			MSG("DispBufQFail\r\n");
		}
	}
	
	// conv422 init
	drv_l1_conv422_register_callback(videnc_conv422_isr_handle);
	drv_l1_conv422_init();
	drv_l1_conv422_input_pixels_set(pVidEnc->target_w, pVidEnc->target_h);
	drv_l1_conv422_input_fmt_set(CONV422_FMT_UYVY);
	drv_l1_conv422_fifo_line_set(pVidEnc->fifo_line_num);
	drv_l1_conv422_output_A_addr_set(fifo_a_buf);
	drv_l1_conv422_output_B_addr_set(fifo_b_buf);
	switch(pVidEnc->conv422_fmt)
	{
	case JPEG_YUV422:
		drv_l1_conv422_output_format_set(CONV422_FMT_422);
		break;
		
	case JPEG_GPYUV420:
		drv_l1_conv422_output_format_set(CONV422_FMT_420);
		break;
		
	default:
		return STATUS_FAIL;
	}
	
	drv_l1_conv422_mode_set(CONV422_FIFO_MODE);
	drv_l1_conv422_bypass_enable(DISABLE);
	drv_l1_conv422_irq_enable(ENABLE);
	drv_l1_conv422_reset();
	
	OS_ENTER_CRITICAL();
	pVidEnc->state = STATE_CHANGE_ENCODE;
	OS_EXIT_CRITICAL();
	
	MSG("wait to start encode.\r\n");
	for(i=0; i<ACK_TIMEOUT; i++) {
		OSTimeDly(1);
		if(pVidEnc->state == STATE_ENCODE) {
			break;
		}
	}
	
	if(i == ACK_TIMEOUT) {
		MSG("encode start fail.\r\n");
		return STATUS_FAIL;
	}
	
	MSG("encode start ok.\r\n");
	return STATUS_OK;
}

INT32S videnc_encode_off(VidEncCtrl_t *pVidEnc)
{
	INT32S i;
	OS_CPU_SR cpu_sr;
	
	MSG("%s.\r\n", __func__);
	
	OS_ENTER_CRITICAL();
	pVidEnc->state = STATE_CHANGE_PREVIEW;
	OS_EXIT_CRITICAL();
	
	MSG("wait to stop encode.\r\n");
	for(i=0; i<ACK_TIMEOUT; i++) {
		OSTimeDly(1);
		if(pVidEnc->state == STATE_PREVIEW) {
			break;
		}
	}

	if(i == ACK_TIMEOUT) {
		MSG("encode stop fail.\r\n");
		return STATUS_FAIL;
	}
	
	MSG("encode stop ok.\r\n");
	return STATUS_OK;
}

// jpeg once encode
INT32U videnc_jpeg_encode_once(MJpegPara_t *pJpegEnc)
{
	INT32S ret;
	
	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(pJpegEnc->Qvalue);	// Load default qunatization table(quality)
	gplib_jpeg_default_huffman_table_load();						// Load default huffman table 
	ret = jpeg_encode_input_size_set(pJpegEnc->input_width, pJpegEnc->input_height);
	if(ret < 0) {
		goto __exit;
	}
	
	jpeg_encode_input_format_set(C_JPEG_FORMAT_YUYV);				// C_JPEG_FORMAT_YUYV / C_JPEG_FORMAT_YUV_SEPARATE
	switch(pJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		ret = jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
		break;
		
	case JPEG_GPYUV420:
		ret = drv_l1_jpeg_yuv_sampling_mode_set(C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;
		
	default:
		ret = STATUS_FAIL;
	}
	
	if(ret < 0) {
		goto __exit;
	}
	
	ret = jpeg_encode_output_addr_set((INT32U) pJpegEnc->output_buffer);
	if(ret < 0) {
		goto __exit;
	}
	
	ret = jpeg_encode_once_start(pJpegEnc->input_buffer, 0, 0);
	if(ret < 0) {
		goto __exit;
	}
	
	while(1) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// Get encode length
			ret = pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get();
			cache_invalid_range(pJpegEnc->output_buffer, ret);
			break;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODING) {
			continue;
		} else {
			MSG("JPEG encode error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
	}
	
__exit:	
	jpeg_encode_stop();
	return ret;
}

// jpeg fifo encode
INT32S videnc_jpeg_encode_fifo_start(MJpegPara_t *pJpegEnc)
{
	INT32S ret;
	
	jpeg_encode_init();
	gplib_jpeg_default_quantization_table_load(pJpegEnc->Qvalue);		// Load default qunatization table(quality=50)
	gplib_jpeg_default_huffman_table_load();			        				// Load default huffman table 
	ret = jpeg_encode_input_size_set(pJpegEnc->input_width, pJpegEnc->input_height);
	if(ret < 0) {
		goto __exit;
	}
	
	jpeg_encode_input_format_set(C_JPEG_FORMAT_YUYV);
	switch(pJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		ret = jpeg_encode_yuv_sampling_mode_set(C_JPG_CTRL_YUV422);
		break;
		
	case JPEG_GPYUV420:
		ret = drv_l1_jpeg_yuv_sampling_mode_set(C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;
		
	default:
		ret = STATUS_FAIL;
	}
	
	if(ret < 0) {
		goto __exit;
	}
	
	ret = jpeg_encode_output_addr_set((INT32U)pJpegEnc->output_buffer);
	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer, 0, 0, pJpegEnc->input_len);
	if(ret < 0) {
		goto __exit;
	}
	
	// wait jpeg block encode done
	pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);	 
	if(pJpegEnc->jpeg_status & (C_JPG_STATUS_TIMEOUT | C_JPG_STATUS_INIT_ERR |C_JPG_STATUS_RST_MARKER_ERR)) {
		ret = STATUS_FAIL;
		goto __exit;
	}
	
__exit:	
	return ret;
}

INT32S videnc_jpeg_encode_fifo_once(MJpegPara_t *pJpegEnc)
{
	INT32S ret = STATUS_OK;

	while(1) {	 
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			ret = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer, 0, 0,  pJpegEnc->input_len);
			if(ret < 0) {
				goto __exit;
			}
			
			// wait jpeg block encode done 
			pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);	
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
			MSG("\r\njpeg encode is not started!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
			MSG("\r\njpeg encode execution timeout!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
			MSG("\r\njpeg encode init error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else {
			MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

__exit:	
	return ret;
}

INT32S videnc_jpeg_encode_fifo_stop(MJpegPara_t *pJpegEnc)
{
	INT32S ret = STATUS_OK;
			
	while(1) {	
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get();	// get jpeg encode size
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			ret = jpeg_encode_on_the_fly_start(pJpegEnc->input_buffer, 0, 0, pJpegEnc->input_len);
			if(ret < 0) {
				goto __exit;
			}
			
			pJpegEnc->jpeg_status = jpeg_encode_status_query(TRUE);	//wait jpeg block encode done 
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
			MSG("\r\njpeg encode is not started!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
			MSG("\r\njpeg encode execution timeout!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
			MSG("\r\njpeg encode init error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		} else {
			MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
			ret = STATUS_FAIL;
			goto __exit;
		}
	}
	
__exit:
	jpeg_encode_stop();	
	return ret;
}

INT32S videnc_jpeg_encode_stop(void)
{
	jpeg_encode_stop();	
	return STATUS_OK;
}

INT32S wrap_save_jpeg_file(INT16S fd, INT32U encode_frame, INT32U encode_size)
{
	INT32S nRet;
	
	if(gpVidEnc->source_type == SOURCE_TYPE_FS) {	
		nRet = write(fd, encode_frame, encode_size);
		if(nRet != encode_size) {
			return STATUS_FAIL;
		}
			
		nRet = close(fd);
		if(nRet < 0) {
			return STATUS_FAIL;
		}
		
		fd = -1;
	} else {
	//to be implemented
	}
	
	nRet = STATUS_OK;	

	return nRet;
}
