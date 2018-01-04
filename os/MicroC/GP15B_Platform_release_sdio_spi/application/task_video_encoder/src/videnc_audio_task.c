#include "videnc_api.h"
#include "drv_l1_cache.h"
#include "drv_l1_dma.h"
#include "drv_l1_i2s.h"
#include "drv_l1_adc.h"
#include "drv_l1_timer.h"
#include "wav_enc.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define PCM_BUF_NO						3
#define WAVE_ENCODE_TIMES				8

#define ADC_USE_TIMER					ADC_AS_TIMER_F	// ADC_AS_TIMER_C ~ F
#define ADC_USE_CHANNEL					ADC_LINE_1		// ADC_LINE_0 or ADC_LINE_1

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	SOUND_ADC_IN = 0,
	SOUND_MIC_IN,
	SOUND_LINE_IN_LR,
	SOUND_IN_MAX	
} SOUND_INPUT;
 
typedef enum
{
	MSG_AUDIO_ENC_START = 0x300,
	MSG_AUDIO_ENC_STOP,
	MSG_AUDIO_ENC_EXIT	
} MSG_AUDIO_ENC;

typedef struct VidEncAudCtrl_s
{
	void *work_mem;
	INT32U pcm_buf[PCM_BUF_NO];
	INT32U pack_buf;
	INT16U pcm_input_size; 
	INT16U pack_size; 
	INT16U pcm_cwlen;
	INT16U encode_size;
	
	INT8U buf_idx;
	INT8U channel_no;	// 1: mono, 2: stereo
    INT16U sample_rate;	// sample rate 
    INT32U audio_format;// audio format
} VidEncAudCtrl_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32S videnc_wave_encode_malloc(VidEncAudCtrl_t *pAudCtrl);
static void videnc_wave_encode_mem_free(VidEncAudCtrl_t *pAudCtrl);
static INT32S videnc_audio_start(VidEncAudCtrl_t *pAudCtrl, INT32U ready_buf, INT32U next_buf, OS_EVENT *AudioQ);
static INT32S videnc_audio_handle_data(VidEncAudCtrl_t *pAudCtrl, INT32U next_buf, INT32U pcm_buf);
static INT32S videnc_audio_stop(VidEncAudCtrl_t *pAudCtrl);
static INT32S videnc_wave_encode_start(VidEncAudCtrl_t *pAudCtrl);
static INT32S videnc_wave_encode(VidEncAudCtrl_t *pAudCtrl, INT16S *pcm_buf);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern INT32S (*packer_put_data)(void *workmem, unsigned long fourcc, long cbLen, const void *ptr, int nSamples, int ChunkFlag);


INT32S videnc_aud_task_start(VidEncCtrl_t *pVidEnc)
{
	return videnc_send_msg(pVidEnc->AudioQ, pVidEnc->AudioM, MSG_AUDIO_ENC_START);
}

INT32S videnc_aud_task_stop(VidEncCtrl_t *pVidEnc)
{
	return videnc_send_msg(pVidEnc->AudioQ, pVidEnc->AudioM, MSG_AUDIO_ENC_STOP);
}

INT32S videnc_aud_task_exit(VidEncCtrl_t *pVidEnc)
{
	return videnc_send_msg(pVidEnc->AudioQ, pVidEnc->AudioM, MSG_AUDIO_ENC_EXIT);
}

void videnc_aud_task_entry(void *p_arg)
{
	INT8U err, index=0;
	INT32U msg;
	INT32S i, ret, encode_size;
	INT32U ready_buf=0, next_buf=0, pcm_buf=0;
	VidEncCtrl_t *pVidEnc = (VidEncCtrl_t *)p_arg;
	OS_EVENT *AudioQ = pVidEnc->AudioQ;
	OS_EVENT *AudioM = pVidEnc->AudioM;
	VidEncAudCtrl_t AudCtrl;
	PackerCtrl_t *pPackerCtrl;
	OS_CPU_SR cpu_sr;
	
	
	while(1) 
	{
		msg = (INT32U) OSQPend(AudioQ, 0, &err);
		if(err != OS_NO_ERR) {
			MSG("%s, AudioQPendFail = %d\r\n", __func__, err);
			continue;
		}
		
		switch(msg)
		{
		case C_DMA_STATUS_DONE:
			index ++;
			if(index >= PCM_BUF_NO) {
				index = 0;
			}
			
			pcm_buf = ready_buf;
			ready_buf = next_buf;
			next_buf = AudCtrl.pcm_buf[index];
			
			encode_size = videnc_audio_handle_data(&AudCtrl, next_buf, pcm_buf);
			if(encode_size > 0) {
				pPackerCtrl = pVidEnc->PackerCtrl;
				ret = packer_put_data(pPackerCtrl->WorkMem, 
									*(long*)"01wb", 
									encode_size, 
									(INT16U*)AudCtrl.pack_buf, 
									encode_size / pPackerCtrl->WaveInfo.nBlockAlign, 
									AVIIF_KEYFRAME);
				if(ret >= 0) {
					MSG("A");
					pVidEnc->record_total_size += encode_size + 8 + 32;
					OS_ENTER_CRITICAL();
					pVidEnc->ta += pVidEnc->delta_ta;
					OS_EXIT_CRITICAL();
				}										
			} else {
				OSQPost(AudioQ, (void *)MSG_AUDIO_ENC_STOP);
			}
			break;
		
		case MSG_AUDIO_ENC_START:
			MSG("MSG_AUDIO_ENC_START\r\n");
			gp_memset((INT8S *)&AudCtrl, 0x00, sizeof(VidEncAudCtrl_t));
			AudCtrl.channel_no = pVidEnc->channel_no;
			AudCtrl.sample_rate = pVidEnc->sample_rate;
			AudCtrl.audio_format = pVidEnc->audio_format;
			
			ret = videnc_wave_encode_start(&AudCtrl);
			if(ret < 0) {
				videnc_wave_encode_mem_free(&AudCtrl);
				OSMboxPost(AudioM, (void *)ACK_FAIL);
				continue;
			}
			
			ret = videnc_wave_encode_malloc(&AudCtrl);
			if(ret < 0) {
				videnc_wave_encode_mem_free(&AudCtrl);
				OSMboxPost(AudioM, (void *)ACK_FAIL);
				continue;
			}
			
			index = 1;
			ready_buf = AudCtrl.pcm_buf[0];
			next_buf = AudCtrl.pcm_buf[1];
			ret = videnc_audio_start(&AudCtrl, ready_buf, next_buf, AudioQ);
			if(ret < 0) {
				videnc_wave_encode_mem_free(&AudCtrl);
				OSMboxPost(AudioM, (void *)ACK_FAIL);
				continue;
			}
			
			pVidEnc->delta_ta = (INT64S)pVidEnc->dwRate * AudCtrl.pcm_cwlen;
			if(AudCtrl.channel_no == 2) {
				pVidEnc->delta_ta >>= 1;
			}
			OSMboxPost(AudioM, (void *)ACK_OK);
			break;
			
		case MSG_AUDIO_ENC_STOP:
			MSG("MSG_AUDIO_ENC_STOP\r\n");	
			videnc_audio_stop(&AudCtrl);
			
			// handle the final data
			for(i=0; i<2; i++) { 
				if(i == 0) {
					pcm_buf = ready_buf; 
					ready_buf = 0;
				} else {
					pcm_buf = next_buf;
					next_buf = 0;
				}
				
				if(pcm_buf == 0) {
					break;
				}
			
				encode_size = videnc_audio_handle_data(&AudCtrl, 0x00, pcm_buf);
				if(encode_size <= 0) {
					break;
				}
				
				pPackerCtrl = pVidEnc->PackerCtrl;
				ret = packer_put_data(pPackerCtrl->WorkMem, 
									*(long*)"01wb", 
									encode_size, 
									(INT16U*)AudCtrl.pack_buf, 
									encode_size / pPackerCtrl->WaveInfo.nBlockAlign, 
									AVIIF_KEYFRAME);
				if(ret >= 0) {
					MSG("A");	
					pVidEnc->record_total_size += encode_size + 8 + 32;
					OS_ENTER_CRITICAL();
					pVidEnc->ta += pVidEnc->delta_ta;
					OS_EXIT_CRITICAL();
				}
			}
			
			videnc_wave_encode_mem_free(&AudCtrl);
			OSMboxPost(AudioM, (void *)ACK_OK);
			break;
			
		case MSG_AUDIO_ENC_EXIT:
			MSG("MSG_AUDIO_ENC_EXIT\r\n");
			OSMboxPost(AudioM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
			
		default:
			break;	
		}	
	}
	while(1);
}

static INT32S videnc_wave_encode_malloc(VidEncAudCtrl_t *pAudCtrl)
{
	INT32U i, size;
	
	pAudCtrl->pcm_cwlen = pAudCtrl->pcm_input_size * WAVE_ENCODE_TIMES * pAudCtrl->channel_no;
	pAudCtrl->encode_size = pAudCtrl->pack_size * WAVE_ENCODE_TIMES * pAudCtrl->channel_no;

	size = pAudCtrl->pcm_cwlen * 2;
	pAudCtrl->pcm_buf[0] = (INT32U)gp_malloc_align(size * PCM_BUF_NO, 4);
	if(pAudCtrl->pcm_buf[0] == 0) {
		return STATUS_FAIL;
	}
	
	for(i=0; i<PCM_BUF_NO; i++) {
		pAudCtrl->pcm_buf[i] = pAudCtrl->pcm_buf[0] + i * size;
		MSG("PcmBuf[%d] = 0x%x (%d)\r\n", i, pAudCtrl->pcm_buf[i], size);
	}

	pAudCtrl->pack_buf = (INT32U)gp_malloc_align(pAudCtrl->encode_size, 4);
	if(pAudCtrl->pack_buf == 0) {
		return STATUS_FAIL;
	}
	
	MSG("AudPackBuf = 0x%x (%d)\r\n", pAudCtrl->pack_buf, pAudCtrl->encode_size);
	return STATUS_OK;
}

static void videnc_wave_encode_mem_free(VidEncAudCtrl_t *pAudCtrl)
{
	INT32U i;
	
	if(pAudCtrl->work_mem) {
		gp_free((void *)pAudCtrl->work_mem);
		pAudCtrl->work_mem = 0;
	}
	
	if(pAudCtrl->pcm_buf[0]) {
		gp_free((void *)pAudCtrl->pcm_buf[0]);
		for(i=0; i<PCM_BUF_NO; i++) {
			pAudCtrl->pcm_buf[i] = 0;
		}
	}
	
	if(pAudCtrl->pack_buf) {
		gp_free((void *)pAudCtrl->pack_buf);
		pAudCtrl->pack_buf = 0;
	}
}

static INT32S videnc_audio_start(VidEncAudCtrl_t *pAudCtrl, INT32U ready_buf, INT32U next_buf, OS_EVENT *AudioQ)
{
	switch(pAudCtrl->channel_no)
	{
	case SOUND_ADC_IN:
		drv_l1_adc_dbf_put((INT16S*)ready_buf, pAudCtrl->pcm_cwlen, AudioQ);
		drv_l1_adc_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		
		// set adc 
		drv_l1_adc_fifo_clear();
		drv_l1_adc_user_line_in_en(ADC_USE_CHANNEL, ENABLE);
		drv_l1_adc_auto_ch_set(ADC_USE_CHANNEL);
		drv_l1_adc_fifo_level_set(4);
		drv_l1_adc_auto_sample_start();
		
		OSTimeDly(50); //wait bais stable
		drv_l1_adc_sample_rate_set(ADC_USE_TIMER, pAudCtrl->sample_rate);
		break;
				
	case SOUND_MIC_IN: 
		drv_l1_i2s_rx_dbf_put((INT16S*)ready_buf, pAudCtrl->pcm_cwlen, AudioQ);
		drv_l1_i2s_rx_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		
		// set i2s
		drv_l1_i2s_rx_set_input_path(MIC_INPUT);
		drv_l1_i2s_rx_init();
		drv_l1_i2s_rx_sample_rate_set(pAudCtrl->sample_rate);
		drv_l1_i2s_rx_mono_ch_set();
		drv_l1_i2s_rx_start();
		break;
		
	case SOUND_LINE_IN_LR: 
		drv_l1_i2s_rx_dbf_put((INT16S*)ready_buf, pAudCtrl->pcm_cwlen, AudioQ);
		drv_l1_i2s_rx_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		
		// set i2s
		drv_l1_i2s_rx_set_input_path(LINE_IN_LR);
		drv_l1_i2s_rx_init();
		drv_l1_i2s_rx_sample_rate_set(pAudCtrl->sample_rate);
		drv_l1_i2s_rx_start();
		break;
		
	default:
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

static INT32S videnc_audio_handle_data(VidEncAudCtrl_t *pAudCtrl, INT32U next_buf, INT32U pcm_buf)
{
	INT16U *pData = (INT16U *)pcm_buf; 
	INT16U t, mask = 0x8000;
	INT32U i;
	
	switch(pAudCtrl->channel_no)
	{
	case SOUND_ADC_IN:
		if(next_buf) {
			drv_l1_adc_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		}
		
		// invalid cache
		cache_invalid_range(pcm_buf, pAudCtrl->pcm_cwlen << 1);
		
		// unsigned to signed
		for(i=0; i<pAudCtrl->pcm_cwlen; i++) 
		{
			t = *pData;
			t ^= mask;
		#if APP_LPF_ENABLE == 1
			t = LPF_process(t);
		#endif
			*pData++ = t;
		}
		break;
		
	case SOUND_MIC_IN:
		if(next_buf) {
			drv_l1_i2s_rx_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		}
		
	#if APP_LPF_ENABLE == 1
		for(i=0; i<pAudCtrl->pcm_cwlen; i++) 
		{
			t = *pData;		
			t = LPF_process(t);
			*pData++ = t;
		}
	#endif
		break;
		
	case SOUND_LINE_IN_LR:
		if(next_buf) {
			drv_l1_i2s_rx_dbf_set((INT16S*)next_buf, pAudCtrl->pcm_cwlen);
		}
		
	#if APP_LPF_ENABLE == 1
		for(i=0; i<pAudCtrl->pcm_cwlen; i++) 
		{
			t = *pData;		
			t = LPF_process(t);
			*pData++ = t;
		}
	#endif	
		break;
		
	default:
		return STATUS_FAIL;	
	}
	
	// wave encode		
	return videnc_wave_encode(pAudCtrl, (INT16S*)pcm_buf);
}

static INT32S videnc_audio_stop(VidEncAudCtrl_t *pAudCtrl)
{
	switch(pAudCtrl->channel_no)
	{
	case SOUND_ADC_IN:
		while(drv_l1_adc_dbf_status_get() == 1 || drv_l1_adc_dma_status_get() == 1) {
			OSTimeDly(2);
		}
		
		drv_l1_adc_dbf_free();
		drv_l1_adc_timer_stop(ADC_USE_TIMER);
		break;
		
	case SOUND_MIC_IN:
		while(drv_l1_i2s_rx_dma_status_get() == 1 || drv_l1_i2s_rx_dbf_status_get() == 1) {
			OSTimeDly(2);
		}
		
		drv_l1_i2s_rx_dbf_free();
		drv_l1_i2s_rx_stop();
		drv_l1_i2s_rx_exit();
		break;	
		
	case SOUND_LINE_IN_LR:
		while(drv_l1_i2s_rx_dma_status_get() == 1 || drv_l1_i2s_rx_dbf_status_get() == 1) {
			OSTimeDly(2);
		}
		
		drv_l1_i2s_rx_dbf_free();
		drv_l1_i2s_rx_stop();
		drv_l1_i2s_rx_exit();
		break;
		
	default:
		return STATUS_FAIL;		
	}
	
	return STATUS_OK;
}

static INT32S videnc_wave_encode_start(VidEncAudCtrl_t *pAudCtrl)
{
#if APP_WAV_CODEC_EN == 1
	INT32S ret, size;
	INT32U audio_format;
	
	size = wav_enc_get_mem_block_size();
	pAudCtrl->work_mem = (INT8U *)gp_malloc_align(size, 4);
	if(pAudCtrl->work_mem == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S*)pAudCtrl->work_mem, 0, size);
	if(pAudCtrl->audio_format == WAV) {
		audio_format = WAVE_FORMAT_PCM;
	} else if(pAudCtrl->audio_format == MICROSOFT_ADPCM) {
		audio_format = WAVE_FORMAT_ADPCM;
	} else if(pAudCtrl->audio_format == IMA_ADPCM) {
		audio_format = WAVE_FORMAT_IMA_ADPCM;
	} else if(pAudCtrl->audio_format == ALAW) {
		audio_format = WAVE_FORMAT_ALAW;
	} else if(pAudCtrl->audio_format == ALAW) {
		audio_format = WAVE_FORMAT_MULAW;
	} else {
		return STATUS_FAIL;
	}
	
	ret = wav_enc_Set_Parameter(pAudCtrl->work_mem, 
								pAudCtrl->channel_no, 
								pAudCtrl->sample_rate, 
								audio_format);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	ret = wav_enc_init(pAudCtrl->work_mem);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	pAudCtrl->pcm_input_size = wav_enc_get_SamplePerFrame(pAudCtrl->work_mem);
	switch(pAudCtrl->audio_format)
	{
	case WAV:
		pAudCtrl->pack_size = pAudCtrl->pcm_input_size << 1;	
		break;
	
	case ALAW:
	case MULAW:	
	case MICROSOFT_ADPCM:
	case IMA_ADPCM:
		pAudCtrl->pack_size = wav_enc_get_BytePerPackage(pAudCtrl->work_mem);	
		break;
	}
	return STATUS_OK;	
#else
	return STATUS_FAIL;
#endif
}

static INT32S videnc_wave_encode(VidEncAudCtrl_t *pAudCtrl, INT16S *pcm_buf)
{
#if APP_WAV_CODEC_EN == 1
	INT8U *encode_output_addr;
	INT32S ret, encode_size, N;
	
	encode_size = 0;
	N = WAVE_ENCODE_TIMES;
	encode_output_addr = (INT8U*)pAudCtrl->pack_buf;
	while(N--) {
		ret = wav_enc_run(pAudCtrl->work_mem, (short *)pcm_buf, encode_output_addr);
		if(ret < 0) {		
			return STATUS_FAIL;
		}
		
		encode_size += ret;
		pcm_buf += pAudCtrl->pcm_input_size;
		encode_output_addr += pAudCtrl->pack_size;
	}
	
	return encode_size;
#else
	return  STATUS_FAIL;
#endif
}
