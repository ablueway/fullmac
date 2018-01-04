#include "turnkey_audio_dac_task.h"
#include "audio_codec_device.h"
#include "drv_l1_dma.h"
#include "drv_l1_dac.h"
#include "drv_l1_I2S.h"
#include "drv_l1_spu.h"
#include "drv_l1_spu_api.h"
#include "drv_l1_spu_midi.h"
#include "drv_l1_spu_speech.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define AUD_OUT_STACK_SIZE			512 
 
#define RAMP_DOWN_STEP 				4
#define RAMP_DOWN_STEP_HOLD 		4
#define RAMP_DOWN_STEP_LOW_SR		(4*16)

#define BUF_TIMEOUT					0
#define ACK_TIMEOUT					5000

#define SPU_LEFT_CH					(SPEECH_CH_START + 1)
#define SPU_RIGHT_CH				(SPEECH_CH_START)
#define SPU_PCM_BUF_SIZE			(4096*2)
#define SPU_PCM_BUF_NUM				2
#define SPU_OUTPUT_TO_I2S			0	// 0: dac output, 1: i2s output. 

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG	DBG_PRINT
#else
	#define DEBUG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
    MSG_AUD_DMA_DBF_DONE = 1,
    MSG_AUD_DMA_DBF_START = MSG_AUDIO_DAC_BASE,
    MSG_AUD_DMA_PAUSE,
    MSG_AUD_DMA_RESUME,
    MSG_AUD_DMA_STOP,
    MSG_AUD_DMA_VOLUME,
    MSG_AUD_DMA_EXIT,
    MSG_AUD_DAC_MAX
} MSG_AUD_DAC_ENUM;

typedef struct AudOutCtrl_s
{
	INT32U task_stack[AUD_OUT_STACK_SIZE];
	OS_EVENT *hAudioDacTaskQ;			// task queue
	OS_EVENT *hAudioDacTaskM;			// task queue ack
	OS_EVENT *pcm_ready_q;				// ready pcm buffer
	OS_EVENT *pcm_ready_len_q;			// ready pcm buffer length
	OS_EVENT *pcm_empty_q;				// empty pcm buffer
	void *hAudOutQBuf[MAX_DAC_BUFFERS];
	void *pcm_ready_q_buf[MAX_DAC_BUFFERS];
	void *pcm_ready_len_q_buf[MAX_DAC_BUFFERS];
	void *pcm_empty_q_buf[MAX_DAC_BUFFERS];
	INT16S *pcm_addr[MAX_DAC_BUFFERS];
	INT32U pcm_cwlen[MAX_DAC_BUFFERS];
	INT16S *last_buf;
	INT32U last_buf_len;
	
	INT8U running_flag;
	INT8U volume;	
	INT8U RSD;
	INT8U channel_no;
	INT32U sample_rate;
} AudOutCtrl_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32S audio_out_send_message(void *handle, INT32U message);
static void audio_dac_task_entry(void *p_arg);
static void audio_i2s_task_entry(void *p_arg);
static void audio_spu_task_entry(void *p_arg);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static OS_EVENT *hSpeechDoneQ;
static const INT8U SPU_PCM_HEADER[48] = {
	//A18 decode
	0x00,0xff,0x00,0xff,0x47,0x45,0x4e,0x45,0x52,0x41,0x4c,0x50,0x4c,0x55,0x53,0x20,
	0x53,0x50,0x00,0x00,0xfe,0x02,0x83,0x00,0x10,0x00, 0x80,0x3e,0x00,0x00,0x00,0xfa,
	0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


static INT32S audio_out_send_message(void *workmem, INT32U message)
{
	INT8U err;
	INT32U ack_msg;
	AudOutCtrl_t *pAudioOutCtrl = (AudOutCtrl_t *)workmem;
	
	// clear ack mbox
	OSMboxAccept(pAudioOutCtrl->hAudioDacTaskM);
	
	// post message
	err = OSQPost(pAudioOutCtrl->hAudioDacTaskQ, (void *)message);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	// wait ack mbox
	ack_msg = (INT32U)OSMboxPend(pAudioOutCtrl->hAudioDacTaskM, ACK_TIMEOUT, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return ack_msg;
}

/**
 * @brief   open audio output device
 * @param   type[in]: output type
 * @return 	work memory
 */
void *audio_out_open(AUDIO_CHANNEL_TYPE type)
{
	INT8U error;
	AudOutCtrl_t *pAudOutCtrl;

	pAudOutCtrl = (AudOutCtrl_t *)gp_malloc_align(sizeof(AudOutCtrl_t), 4);
	if(pAudOutCtrl == 0) {
		return 0;
	}
	
	DEBUG("audio_out_workmem[%d] = 0x%x\r\n", type, pAudOutCtrl);
	gp_memset((INT8S *)pAudOutCtrl, 0x00, sizeof(AudOutCtrl_t));
	pAudOutCtrl->hAudioDacTaskQ = OSQCreate(pAudOutCtrl->hAudOutQBuf, MAX_DAC_BUFFERS);
	if(pAudOutCtrl->hAudioDacTaskQ == 0) {
		goto __fail;
	}

	pAudOutCtrl->hAudioDacTaskM = OSMboxCreate(NULL);
	if(pAudOutCtrl->hAudioDacTaskM == 0) {
		goto __fail;
	}
	
	pAudOutCtrl->pcm_ready_q = OSQCreate(pAudOutCtrl->pcm_ready_q_buf, MAX_DAC_BUFFERS);
	if(pAudOutCtrl->pcm_ready_q == 0) {
		goto __fail;
	}
	
	pAudOutCtrl->pcm_ready_len_q = OSQCreate(pAudOutCtrl->pcm_ready_len_q_buf, MAX_DAC_BUFFERS);
	if(pAudOutCtrl->pcm_ready_len_q == 0) {
		goto __fail;
	}

	pAudOutCtrl->pcm_empty_q = OSQCreate(pAudOutCtrl->pcm_empty_q_buf, MAX_DAC_BUFFERS);
	if(pAudOutCtrl->pcm_empty_q == 0) {
		goto __fail;
	}
		
	switch(type)
	{
	case AUDIO_CHANNEL_SPU:
		error = OSTaskCreate(audio_spu_task_entry, (void *)pAudOutCtrl, &pAudOutCtrl->task_stack[AUD_OUT_STACK_SIZE - 1], SPU_PRIORITY);
		break;
	case AUDIO_CHANNEL_DAC:
		error = OSTaskCreate(audio_dac_task_entry, (void *)pAudOutCtrl, &pAudOutCtrl->task_stack[AUD_OUT_STACK_SIZE - 1], DAC_PRIORITY);
		break;	
	case AUDIO_CHANNEL_I2S:	
		error = OSTaskCreate(audio_i2s_task_entry, (void *)pAudOutCtrl, &pAudOutCtrl->task_stack[AUD_OUT_STACK_SIZE - 1], I2S_PRIORITY);
		break;
	default:
		while(1);	
	}
	
	if(error != OS_NO_ERR) {
		goto __fail;
	}
	
	pAudOutCtrl->running_flag = 1;
	return (void *)pAudOutCtrl;
	
__fail:
	DEBUG("%s Fail\r\n", __func__);
	audio_out_close(pAudOutCtrl);
	return 0;
}

/**
 * @brief   close audio output device
 * @param   workmem[in]: work memory
 * @return 	none
 */
void audio_out_close(void *workmem)
{
	INT8U err;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	if(pAudOutCtrl->running_flag) {
		audio_out_send_message(workmem, MSG_AUD_DMA_STOP);		
	}

	if(pAudOutCtrl->hAudioDacTaskQ) {
		OSQDel(pAudOutCtrl->hAudioDacTaskQ, 0, &err);
	}

	if(pAudOutCtrl->hAudioDacTaskM) {
		OSMboxDel(pAudOutCtrl->hAudioDacTaskM, 0, &err);
	}
	
	if(pAudOutCtrl->pcm_ready_q) {
		OSQDel(pAudOutCtrl->pcm_ready_q, 0, &err);
	}
	
	if(pAudOutCtrl->pcm_ready_len_q) {
		OSQDel(pAudOutCtrl->pcm_ready_len_q, 0, &err);
	}
	
	if(pAudOutCtrl->pcm_empty_q) {
		OSQDel(pAudOutCtrl->pcm_empty_q, 0, &err);
	}
	
	if(pAudOutCtrl->pcm_addr[0]) {
		gp_free((void *)pAudOutCtrl->pcm_addr[0]);
	}
	
	if(pAudOutCtrl) {
		gp_free((void *)pAudOutCtrl);
	}	
}

/**
 * @brief   send the ready pcm buffer
 * @param   workmem[in]: work memory
 * @param   pcm_addr[in]: pcm buffer address
 * @param   pcm_cwlen[in]: pcm length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_send_ready_buf(void *workmem, INT16S *pcm_addr, INT32U pcm_cwlen)
{
	INT8U err;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	if((pcm_addr == 0) || (pcm_cwlen == 0)) {
		return STATUS_FAIL;
	}
	
	err = OSQPost(pAudOutCtrl->pcm_ready_q, (void *)pcm_addr);
	err = OSQPost(pAudOutCtrl->pcm_ready_len_q, (void *)pcm_cwlen);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;	
}

/**
 * @brief   send the empty pcm buffer
 * @param   workmem[in]: work memory
 * @param   pcm_addr[in]: pcm buffer address
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_send_empty_buf(void *workmem, INT32U pcm_addr)
{
	INT8U err;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	if(pcm_addr == 0) {
		return STATUS_FAIL;
	}
	
	err = OSQPost(pAudOutCtrl->pcm_empty_q, (void *)pcm_addr);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;	
}

/**
 * @brief   get the empty pcm buffer
 * @param   workmem[in]: work memory
 * @return 	pcm buffer address, 0 is fail.
 */
INT16S *audio_out_get_empty_buf(void *workmem)
{
	INT8U err;
	INT16S *pcm_addr;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	pcm_addr = (INT16S *)OSQPend(pAudOutCtrl->pcm_empty_q, BUF_TIMEOUT, &err);
	if(pcm_addr && (err == OS_NO_ERR)) {
		return pcm_addr;
	}
	
	return 0;
}

/**
 * @brief   get the ready pcm buffer
 * @param   workmem[in]: work memory
 * @param   pcm_addr[out]: pcm buffer address
 * @param   pcm_cwlen[out]: pcm buffer length
 * @return  STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_get_ready_buf(void *workmem, INT32U *pcm_addr, INT32U *pcm_cwlen)
{
	INT8U err;
	INT32U addr, length;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	addr = (INT32U)OSQPend(pAudOutCtrl->pcm_ready_q, BUF_TIMEOUT, &err);
	if((addr == 0) || (err != OS_NO_ERR)) {
		return STATUS_FAIL;
	}
	
	length = (INT32S)OSQPend(pAudOutCtrl->pcm_ready_len_q, BUF_TIMEOUT, &err);
	if((length == 0) || (err != OS_NO_ERR)) {
		return STATUS_FAIL;
	}
	
	*pcm_addr = addr;
	*pcm_cwlen = length;
	return STATUS_OK;
}

/**
 * @brief   allocate pcm buffer  
 * @param   workmem[in]: work memory
 * @param   pcm size[in]: pcm buffer size
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_init(void *workmem, INT32U pcm_size)
{
	INT32U i;
	INT32U pcm_addr;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	pcm_size <<= 2;	// 16bit and 2 channel
	pcm_addr = (INT32U)gp_malloc_align(pcm_size * MAX_DAC_BUFFERS, 4);
	if(pcm_addr == 0) {
		return STATUS_FAIL;
	}
	
	DEBUG("pcm_addr = 0x%x (%d)\r\n", pcm_addr, pcm_size * MAX_DAC_BUFFERS);
	gp_memset((INT8S *)pcm_addr, 0x00, pcm_size * MAX_DAC_BUFFERS);
	for(i=0; i<MAX_DAC_BUFFERS; i++) {
		pAudOutCtrl->pcm_addr[i] = (INT16S *)(pcm_addr + (pcm_size * i));
		DEBUG("pcm buffer[%d] = 0x%x (%d)\r\n", i, pAudOutCtrl->pcm_addr[i], pcm_size);
	}

	for(i=0; i<MAX_DAC_BUFFERS; i++) {
		OSQPost(pAudOutCtrl->pcm_empty_q, pAudOutCtrl->pcm_addr[i]);
	}
	
	return STATUS_OK;
}

/**
 * @brief   start audio output device
 * @param   workmem[in]: work memory
 * @param   channel_no[in]: mono or stereo
 * @param   sample_rate[in]: sample rate
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_start(void *workmem, INT32U channle_no, INT32U sample_rate)
{
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	pAudOutCtrl->channel_no = channle_no;
	pAudOutCtrl->sample_rate = sample_rate;
	return audio_out_send_message(workmem, MSG_AUD_DMA_DBF_START);
}

/**
 * @brief   pause audio output device
 * @param   workmem[in]: work memory
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_pause(void *workmem)
{
	return audio_out_send_message(workmem, MSG_AUD_DMA_PAUSE);
}

/**
 * @brief   resume audio output device
 * @param   workmem[in]: work memory
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_resume(void *workmem)
{
	return audio_out_send_message(workmem, MSG_AUD_DMA_RESUME);
}

/**
 * @brief   set audio output device volume
 * @param   workmem[in]: work memory
 * @param   volume[in]: sound volume
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_volume(void *workmem, INT32U volume)
{
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	pAudOutCtrl->volume = volume;
	return audio_out_send_message(workmem, MSG_AUD_DMA_VOLUME);
}

/**
 * @brief   stop audio output device
 * @param   workmem[in]: work memory
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_out_stop(void *workmem)
{
	INT32S ret, i;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	ret = audio_out_send_message(workmem, MSG_AUD_DMA_STOP);
	if(pAudOutCtrl->pcm_addr[0]) {
		gp_free((void *)pAudOutCtrl->pcm_addr[0]);
	}	
	
	for(i=0; i<MAX_DAC_BUFFERS; i++) {
		pAudOutCtrl->pcm_addr[i] = 0;
	}
	
	return ret;
}

/**
 * @brief   get the audio output final pcm buffer and length
 * @param   workmem[in]: work memory
 * @param   cwlen[out]: pcm length
 * @return 	pcm buffer
 */
INT16S *audio_out_prepare_ramp_down(void *workmem, INT32U *cwlen)
{
	INT8U err;
	INT32U pcm_buf, len;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)workmem;
	
	while(1) {
		pcm_buf = (INT32U)OSQAccept(pAudOutCtrl->pcm_ready_q, &err);
		len = (INT32U)OSQAccept(pAudOutCtrl->pcm_ready_len_q, &err);
		if(pcm_buf == 0 || len == 0 || err != OS_NO_ERR) {
			break;
		}
		
		DEBUG("pcm_ready = 0x%x (%d)\r\n", pcm_buf, len);
		OSQPost(pAudOutCtrl->pcm_empty_q, (void *)pcm_buf);
	}
	
	DEBUG("final_pcm_buf = 0x%x (%d)\r\n", pAudOutCtrl->last_buf, pAudOutCtrl->last_buf_len);
	*cwlen = pAudOutCtrl->last_buf_len;
	return pAudOutCtrl->last_buf;
}

static void dac_ramp_down_rapid(INT32U channel_no)
{
	INT16S  last_ldata,last_rdata;
	INT16S  i, temp;
	
	// wait double buffer dma stop
	while(drv_l1_dac_dbf_status_get() == 1 || drv_l1_dac_dma_status_get() == 1) {
		OSTimeDly(2);
	}
			
	drv_l1_dac_dbf_free();	
	
	// change to cpu polling dac mode	
	temp = 0 - RAMP_DOWN_STEP;
	last_ldata = R_DAC_CHA_DATA;
	last_rdata = R_DAC_CHB_DATA;
	
	// unsigned to signed 
	last_ldata ^= 0x8000;
	if(channel_no == 2) {
		last_rdata ^= 0x8000;
	}
	else {
		last_rdata = 0x0;
	}
	
	//change timer to 44100
	drv_l1_dac_sample_rate_set(44100);
	
	while(1)
	{
		if (last_ldata > 0x0) {
			last_ldata -= RAMP_DOWN_STEP;
		}
		else if (last_ldata < 0x0) {
			last_ldata += RAMP_DOWN_STEP;
		}
			
		if ((last_ldata < RAMP_DOWN_STEP)&&(last_ldata > temp)) { 
			last_ldata = 0;
		}

		if (channel_no == 2) {
			if (last_rdata > 0x0) {
				last_rdata -= RAMP_DOWN_STEP;
		    }
			else if (last_rdata < 0x0) {
				last_rdata += RAMP_DOWN_STEP;
		    }
		        
		    if ((last_rdata < RAMP_DOWN_STEP)&&(last_rdata > temp)) {  
				last_rdata = 0;
			}
		}
		    
		for(i=0;i<RAMP_DOWN_STEP_HOLD;i++) {
			if (channel_no == 2){
				while(R_DAC_CHA_FIFO & 0x8000);
				R_DAC_CHA_DATA = last_ldata;
				while(R_DAC_CHB_FIFO & 0x8000);
				R_DAC_CHB_DATA = last_rdata;
			} else {
				while(R_DAC_CHA_FIFO & 0x8000);
				R_DAC_CHA_DATA = last_ldata;
			}
		}
		
		if ((last_ldata == 0x0) && (last_rdata == 0x0)) {
			break;
		}
	}
	
	drv_l1_dac_timer_stop();
}

static void audio_dac_task_entry(void *p_arg)
{
	INT8U err;
	INT8U pause = 0;
	INT32U size=0, msg;
	INT32U ready_addr = 0;
	INT32U next_addr = 0;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)p_arg;
	OS_EVENT *hAudioDacTaskQ = pAudOutCtrl->hAudioDacTaskQ;
	OS_EVENT *hAudioDacTaskM = pAudOutCtrl->hAudioDacTaskM;
	
	while(1) {
		msg = (INT32S) OSQPend(hAudioDacTaskQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}

		switch(msg) 
		{
		case C_DMA_STATUS_DONE:
			//DEBUG(".");
			if(pause) {
				continue;
			}
			
			if(drv_l1_dac_dbf_status_get() == 0 && drv_l1_dac_dma_status_get() == 0) {
				DEBUG("DacUnderRun.\r\n");
			}
			
			audio_out_send_empty_buf((void *)pAudOutCtrl, ready_addr);
			ready_addr = next_addr;
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			pAudOutCtrl->last_buf = (INT16S *)next_addr;
			pAudOutCtrl->last_buf_len = size;
			drv_l1_dac_cha_dbf_set((INT16S *)next_addr, size);
			break;
			
		case MSG_AUD_DMA_DBF_START:
			DEBUG("MSG_AUD_DMA_DBF_START\r\n");	
			pause = 0;
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &ready_addr, &size);
			drv_l1_dac_cha_dbf_put((INT16S *)ready_addr, size, hAudioDacTaskQ);
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			drv_l1_dac_cha_dbf_set((INT16S *)next_addr, size);
			
			// dac timer start 
			if(pAudOutCtrl->channel_no == 1) {
				drv_l1_dac_mono_set();			
			} else {
				drv_l1_dac_stereo_set();			
			}
			
			drv_l1_dac_sample_rate_set(pAudOutCtrl->sample_rate);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
				
		case MSG_AUD_DMA_PAUSE:
			DEBUG("MSG_AUD_DMA_PAUSE\r\n");	
			pause = 1;
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_RESUME:
			DEBUG("MSG_AUD_DMA_RESUME\r\n");	
			pause = 0;
			OSQPost(hAudioDacTaskQ, (void *)C_DMA_STATUS_DONE);
			OSQPost(hAudioDacTaskQ, (void *)C_DMA_STATUS_DONE);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_STOP:
			DEBUG("MSG_AUD_DMA_STOP\r\n");
			dac_ramp_down_rapid(pAudOutCtrl->channel_no);
			OSQFlush(hAudioDacTaskQ); 
			OSQFlush(pAudOutCtrl->pcm_ready_q);
			OSQFlush(pAudOutCtrl->pcm_ready_len_q);
			OSQFlush(pAudOutCtrl->pcm_empty_q);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK); 
			break;
		
		case MSG_AUD_DMA_VOLUME:
			DEBUG("MSG_AUD_DMA_VOLUME\r\n");
			if(pAudOutCtrl->volume > 63) {
				pAudOutCtrl->volume = 63;
			}
			
			drv_l1_dac_pga_set(pAudOutCtrl->volume);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
		
		case MSG_AUD_DMA_EXIT:
			DEBUG("MSG_AUD_DMA_EXIT\r\n");
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
				
		default:
			DEBUG("DAC Message Fail!!!\r\n");	
		}
	}
}

static void audio_i2s_task_entry(void *p_arg)
{
	INT8U err;
	INT8U pause = 0;
	INT32U size=0, msg;
	INT32U ready_addr = 0;
	INT32U next_addr = 0;
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)p_arg;
	OS_EVENT *hAudioDacTaskQ = pAudOutCtrl->hAudioDacTaskQ;
	OS_EVENT *hAudioDacTaskM = pAudOutCtrl->hAudioDacTaskM;
	
	if(AudCodec_device.init() < 0) {
		DEBUG("audio codec device init fail\r\n");
	}
	
	while(1) {
		msg = (INT32S) OSQPend(hAudioDacTaskQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}

		switch(msg) 
		{
		case C_DMA_STATUS_DONE:
			//DEBUG("-");	
			if(pause) {
				continue;
			}
			
			if(drv_l1_i2s_tx_dbf_status_get() == 0 && drv_l1_i2s_tx_dma_status_get() == 0) {
				DEBUG("I2SUnderRun.\r\n");
			}
			
			audio_out_send_empty_buf((void *)pAudOutCtrl, ready_addr);
			ready_addr = next_addr;
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			pAudOutCtrl->last_buf = (INT16S *)next_addr;
			pAudOutCtrl->last_buf_len = size;
			drv_l1_i2s_tx_dbf_set((INT16S *)next_addr, size);
			break;
			
		case MSG_AUD_DMA_DBF_START:
			DEBUG("MSG_AUD_I2S_DBF_START\r\n");	
			pause = 0;
			if(AudCodec_device.start(0) < 0) {
				OSMboxPost(hAudioDacTaskM, (void *)ACK_FAIL);
				continue;
			}
			
			// set 16bit and i2s mode
			drv_l1_i2s_tx_init();
			drv_l1_i2s_tx_fifo_clear();
			drv_l1_i2s_tx_set_firstframe(I2S_FIRST_FRAME_L);
			drv_l1_i2s_tx_set_framepolarity(I2S_FRAME_POLARITY_L);
			drv_l1_i2s_tx_set_edgemode(I2S_FALLING_EDGE);
			drv_l1_i2s_tx_set_sendmode(I2S_MSB_MODE);
			drv_l1_i2s_tx_set_align(I2S_ALIGN_LEFT);
			drv_l1_i2s_tx_set_validdata(I2S_VALID_16);
			drv_l1_i2s_tx_set_i2smode(I2S_MODE_I2S);
			drv_l1_i2s_tx_set_master(I2S_MASTER);
			drv_l1_i2s_tx_set_intpolarity(I2S_IRT_POLARITY_H);
			drv_l1_i2s_tx_set_over_write(I2S_OVERWRITE_DISABLE);
			drv_l1_i2s_tx_set_int(I2S_INT_DISABLE);
			drv_l1_i2s_tx_set_merge(I2S_MERGE_ENABLE);
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &ready_addr, &size);
			drv_l1_i2s_tx_dbf_put((INT16S *)ready_addr, size, hAudioDacTaskQ);
			
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			drv_l1_i2s_tx_dbf_set((INT16S *)next_addr, size);
			
			if(pAudOutCtrl->channel_no == 1) {
				drv_l1_i2s_tx_set_mono(I2S_MODE_MONO);		
			} else {
				drv_l1_i2s_tx_set_mono(I2S_MODE_STEREO);
			}
			
			drv_l1_i2s_tx_sample_rate_set(pAudOutCtrl->sample_rate);
			drv_l1_i2s_tx_start();
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
				
		case MSG_AUD_DMA_PAUSE:
			DEBUG("MSG_AUD_I2S_PAUSE\r\n");	
			pause = 1;
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_RESUME:
			DEBUG("MSG_AUD_I2S_RESUME\r\n");
			pause = 0;	
			OSQPost(hAudioDacTaskQ, (void *)C_DMA_STATUS_DONE);
			OSQPost(hAudioDacTaskQ, (void *)C_DMA_STATUS_DONE);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_STOP:
			DEBUG("MSG_AUD_I2S_STOP\r\n");
			while(drv_l1_i2s_tx_dbf_status_get() == 1 || drv_l1_i2s_tx_dma_status_get() == 1) {
				OSTimeDly(2);
			}
			
			drv_l1_i2s_tx_dbf_free();
			drv_l1_i2s_tx_stop();
			drv_l1_i2s_tx_exit();
			AudCodec_device.stop();
			
			OSQFlush(hAudioDacTaskQ); 
			OSQFlush(pAudOutCtrl->pcm_ready_q);
			OSQFlush(pAudOutCtrl->pcm_ready_len_q);
			OSQFlush(pAudOutCtrl->pcm_empty_q);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK); 
			break;
		
		case MSG_AUD_DMA_VOLUME:
			DEBUG("MSG_AUD_I2S_VOLUME\r\n");
			if(pAudOutCtrl->volume > 63) {
				pAudOutCtrl->volume = 63;
			}
			
			AudCodec_device.adjust_volume(pAudOutCtrl->volume);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
		
		case MSG_AUD_DMA_EXIT:
			DEBUG("MSG_AUD_I2S_EXIT\r\n");
			AudCodec_device.uninit();
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
				
		default:
			DEBUG("I2S Message Fail!!!\r\n");
		}
	}
}

static INT32S audio_spu_speech_end(INT8U ch)
{
	INT8U err;
	
	if(ch == SPU_LEFT_CH) {
		err = OSQPost(hSpeechDoneQ, (void *)C_DMA_STATUS_DONE);
		if(err != OS_NO_ERR) {
			DEBUG("aud spu Post Fail\r\n");
		}
	}
	
	if(ch == SPU_LEFT_CH || ch == SPU_RIGHT_CH) {
		return 0;	//not stop channel
	}
	
	return 1;	//stop channel
}

static void audio_spu_add_end_code(INT32U channel_no, INT32U size, INT16U *pData, INT16U *pDataL, INT16U *pDataR)
{
	INT16U temp;
	INT32U i;
	
	if(channel_no == 1) {
		// check end code
		for(i=0; i<size; i++) {
			temp = *pData++;
			if(temp == 0x8000) {
				temp = 0x8001;
			} 
			
			*pDataL++ = temp;
		}
		
		// set end code
		temp = *(pDataL - 1);
		*(pDataL - 1) = 0x8000;
		*pDataL = temp;
	} else {
		// check end code
		for(i=0; i<size; i++) {
			temp = *pData++;
			if(temp == 0x8000) {
				temp = 0x8001;
			} 
			
			if(i & 1) {
				*pDataR++ = temp; 	
			} else {
				*pDataL++ = temp; 
			}
		}
		
		// set end code at end
		temp = *(pDataL - 1);
		*(pDataL - 1) = 0x8000;
		*pDataL = temp;
		
		temp = *(pDataR - 1);
		*(pDataR - 1) = 0x8000;
		*pDataR = temp;
	}
}

static void audio_spu_task_entry(void *p_arg)
{
	INT8U err;
	INT8U channel_no = 1;
	INT8U buf_idx = 0;
	INT8U volume = 0;
	INT16U *pData;
	INT32U i, size, msg;
	INT32U ready_addr = 0;
	INT32U next_addr = 0;
	INT32U left_addr[SPU_PCM_BUF_NUM], right_addr[SPU_PCM_BUF_NUM];
	AudOutCtrl_t *pAudOutCtrl = (AudOutCtrl_t *)p_arg;
	OS_EVENT *hAudioDacTaskQ = pAudOutCtrl->hAudioDacTaskQ;
	OS_EVENT *hAudioDacTaskM = pAudOutCtrl->hAudioDacTaskM;
	
	// spu pcm buffer alloc
	size = SPU_PCM_BUF_SIZE * SPU_PCM_BUF_NUM;
	left_addr[0] = (INT32U)gp_malloc_align(size * 2, 4);
	if(left_addr[0] == 0) {
		DEBUG("audio spu malloc fail\r\n");
		while(1);
	}
	
	for(i=0; i<SPU_PCM_BUF_NUM; i++) {
		left_addr[i] = left_addr[0] + (SPU_PCM_BUF_SIZE * i);
		right_addr[i] = left_addr[0] + size + (SPU_PCM_BUF_SIZE * i);
		DEBUG("spu_pcm_addr[%d] = 0x%x, 0x%x (%d)\r\n", i, left_addr[i], right_addr[i], SPU_PCM_BUF_SIZE);
	}	
	
	SPU_Init();
	SPU_Speech_Register_Callback(NULL);
#if SPU_OUTPUT_TO_I2S == 1
	SPU_SetOutputToI2S(ENABLE);
	if(AudCodec_device.init() < 0) {
		DEBUG("audio codec device init fail\r\n");
	}
#endif	
	
	while(1) {
		msg = (INT32S) OSQPend(hAudioDacTaskQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}

		switch(msg) 
		{
		case C_DMA_STATUS_DONE:
			//DEBUG("+");
			audio_out_send_empty_buf((void *)pAudOutCtrl, ready_addr);
			ready_addr = next_addr;
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			
			buf_idx++;
			if(buf_idx >= SPU_PCM_BUF_NUM) {
				buf_idx = 0;
			}
			
			if(channel_no == 1) {
				// check end code
				audio_spu_add_end_code(1, size, 
									(INT16U *)next_addr, 
									(INT16U *)left_addr[buf_idx], 
									NULL);
				// set next buffer
				SPU_Speech_NextFrameAddrSet((INT8U *)left_addr[buf_idx], SPU_LEFT_CH);					
			} else {
				// check end code
				audio_spu_add_end_code(2, size, 
									(INT16U *)next_addr, 
									(INT16U *)left_addr[buf_idx], 
									(INT16U *)right_addr[buf_idx]);
				// set next buffer
				SPU_Speech_NextFrameAddrSet((INT8U *)left_addr[buf_idx], SPU_LEFT_CH);
				SPU_Speech_NextFrameAddrSet((INT8U *)right_addr[buf_idx], SPU_RIGHT_CH);
			}
			break;
			
		case MSG_AUD_DMA_DBF_START:
			DEBUG("MSG_AUD_SPU_DBF_START\r\n");
			channel_no = pAudOutCtrl->channel_no;
			buf_idx = 1;
					
		#if SPU_OUTPUT_TO_I2S == 1	
			if(AudCodec_device.start(0) < 0) {
				OSMboxPost(hAudioDacTaskM, (void *)ACK_FAIL);
				continue;
			}
			
			// set 16bit and i2s mode
			drv_l1_i2s_tx_init();
			drv_l1_i2s_tx_fifo_clear();
			drv_l1_i2s_tx_set_firstframe(I2S_FIRST_FRAME_L);
			drv_l1_i2s_tx_set_framepolarity(I2S_FRAME_POLARITY_L);
			drv_l1_i2s_tx_set_edgemode(I2S_FALLING_EDGE);
			drv_l1_i2s_tx_set_sendmode(I2S_MSB_MODE);
			drv_l1_i2s_tx_set_align(I2S_ALIGN_LEFT);
			drv_l1_i2s_tx_set_validdata(I2S_VALID_16);
			drv_l1_i2s_tx_set_i2smode(I2S_MODE_I2S);
			drv_l1_i2s_tx_set_master(I2S_MASTER);
			drv_l1_i2s_tx_set_intpolarity(I2S_IRT_POLARITY_H);
			drv_l1_i2s_tx_set_over_write(I2S_OVERWRITE_DISABLE);
			drv_l1_i2s_tx_set_int(I2S_INT_DISABLE);
			drv_l1_i2s_tx_set_merge(I2S_MERGE_ENABLE);
			
			drv_l1_i2s_tx_set_mono(I2S_MODE_STEREO);
			drv_l1_i2s_tx_sample_rate_set(48000);
			drv_l1_i2s_tx_spu_fs(48000);
			drv_l1_i2s_tx_start();
		#endif
			
			// first frame
			audio_out_get_ready_buf((void *)pAudOutCtrl, &ready_addr, &size);
			if(channel_no == 1) {
				// copy header
				gp_memcpy((INT8S *)left_addr[0], (INT8S *)SPU_PCM_HEADER, sizeof(SPU_PCM_HEADER));
				
				// set sample rate 
				pData = (INT16U *)left_addr[0];
				*(pData + 13) = pAudOutCtrl->sample_rate;				
				
				// check end code
				audio_spu_add_end_code(1, size, 
									(INT16U *)ready_addr, 
									(INT16U *)(left_addr[0] + sizeof(SPU_PCM_HEADER)), 
									NULL);
			} else {
				// copy header
				gp_memcpy((INT8S *)left_addr[0], (INT8S *)SPU_PCM_HEADER, sizeof(SPU_PCM_HEADER));
				
				// set sample rate 
				pData = (INT16U *)left_addr[0];
				*(pData + 13) = pAudOutCtrl->sample_rate;
				
				// copy header
				gp_memcpy((INT8S *)right_addr[0], (INT8S *)SPU_PCM_HEADER, sizeof(SPU_PCM_HEADER));
			
				// set sample rate 
				pData = (INT16U *)right_addr[0];
				*(pData + 13) = pAudOutCtrl->sample_rate;
			
				// check end code
				audio_spu_add_end_code(2, size, 
									(INT16U *)ready_addr, 
									(INT16U *)(left_addr[0] + sizeof(SPU_PCM_HEADER)), 
									(INT16U *)(right_addr[0] + sizeof(SPU_PCM_HEADER)));
			}
			
			// second frame
			audio_out_get_ready_buf((void *)pAudOutCtrl, &next_addr, &size);
			if(channel_no == 1) {
				// check end code
				audio_spu_add_end_code(1, size, 
									(INT16U *)next_addr, 
									(INT16U *)left_addr[1], 
									NULL);
			} else {
				// check end code
				audio_spu_add_end_code(2, size,
									(INT16U *)next_addr, 
									(INT16U *)left_addr[1], 
									(INT16U *)right_addr[1]);
			}
			
			// play start
			hSpeechDoneQ = hAudioDacTaskQ;
			SPU_Speech_SetVol(SPU_LEFT_CH, 0);
			if(channel_no == 2) {
				SPU_Speech_SetVol(SPU_RIGHT_CH, 0);
			} 
			
			SPU_Speech_Register_Callback(audio_spu_speech_end);
			if(channel_no == 1) {
				// get pcm format
				SPU_Speech_GetHeaderData((INT8U *)left_addr[0], SPU_LEFT_CH);
				
				// set next pcm buffer address
				SPU_Speech_NextFrameAddrSet((INT8U *)left_addr[1], SPU_LEFT_CH);
				
				// start play
				SPU_Speech_Play((INT8U *)left_addr[0], SPU_LEFT_CH);
			} else {
				OS_CPU_SR cpu_sr;
				
				// get pcm format
				SPU_Speech_GetHeaderData((INT8U *)left_addr[0], SPU_LEFT_CH);
				SPU_Speech_GetHeaderData((INT8U *)right_addr[0], SPU_RIGHT_CH);
				
				// set next pcm buffer address
				SPU_Speech_NextFrameAddrSet((INT8U *)left_addr[1], SPU_LEFT_CH);
				SPU_Speech_NextFrameAddrSet((INT8U *)right_addr[1], SPU_RIGHT_CH);
				
				// start play
				OS_ENTER_CRITICAL();
				SPU_Speech_Play((INT8U *)left_addr[0], SPU_LEFT_CH);
				SPU_Speech_Play((INT8U *)right_addr[0], SPU_RIGHT_CH);
				OS_EXIT_CRITICAL();
			
				// set pan
				SPU_SetChMixer(SPU_LEFT_CH, 0x02, DISABLE);
				SPU_SetChMixer(SPU_RIGHT_CH, 0x01, DISABLE);
			}
			
			// ramp up
			for(i=0; i<pAudOutCtrl->volume; i++) {
				SPU_Speech_SetVol(SPU_LEFT_CH, i*2);
				if(channel_no == 2) {
					SPU_Speech_SetVol(SPU_RIGHT_CH, i*2);
				} 
			}
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
				
		case MSG_AUD_DMA_PAUSE:
			DEBUG("MSG_AUD_SPU_PAUSE\r\n");	
			SPU_Speech_Pause(SPU_LEFT_CH);
			if(channel_no == 2) {
				SPU_Speech_Pause(SPU_RIGHT_CH);
			}
			
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_RESUME:
			DEBUG("MSG_AUD_SPU_RESUME\r\n");	
			SPU_Speech_Resume(SPU_LEFT_CH);
			if(channel_no == 2) {
				SPU_Speech_Resume(SPU_RIGHT_CH);
			}
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_AUD_DMA_STOP:
			DEBUG("MSG_AUD_SPU_STOP\r\n");
		#if SPU_OUTPUT_TO_I2S == 1
			drv_l1_i2s_tx_stop();
			drv_l1_i2s_tx_exit();
			AudCodec_device.stop();
		#endif
			
			SPU_Speech_Register_Callback(NULL);
			SPU_Speech_Stop(SPU_LEFT_CH);
			if(channel_no == 2) {
				SPU_Speech_Stop(SPU_RIGHT_CH);
			}
			
			OSQFlush(hAudioDacTaskQ); 
			OSQFlush(pAudOutCtrl->pcm_ready_q);
			OSQFlush(pAudOutCtrl->pcm_ready_len_q);
			OSQFlush(pAudOutCtrl->pcm_empty_q);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK); 
			break;
		
		case MSG_AUD_DMA_VOLUME:
			DEBUG("MSG_AUD_SPU_VOLUME\r\n");
			if(pAudOutCtrl->volume > 63) {
				volume = 127;
			} else {
				volume = pAudOutCtrl->volume << 1;
			}
			
			SPU_Speech_SetVol(SPU_LEFT_CH, volume);
			if(channel_no == 2) {
				SPU_Speech_SetVol(SPU_RIGHT_CH, volume);
			}
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			break;
		
		case MSG_AUD_DMA_EXIT:
			DEBUG("MSG_AUD_SPU_EXIT\r\n");
		#if SPU_OUTPUT_TO_I2S == 1
			SPU_SetOutputToI2S(DISABLE);
			AudCodec_device.uninit();
		#endif	
			gp_free((void *)left_addr[0]);
			OSMboxPost(hAudioDacTaskM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
				
		default:
			DEBUG("SPU Message Fail!!!\r\n");	
		}
	}
}

