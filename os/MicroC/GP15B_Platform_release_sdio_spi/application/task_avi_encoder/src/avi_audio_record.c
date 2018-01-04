#include "drv_l1_cache.h"
#include "avi_audio_record.h"
#include "drv_l1_dma.h"
#include "drv_l1_i2s.h"
#include "drv_l1_adc.h"
#include "wav_enc.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/ 
#define C_AVI_AUDIO_RECORD_STACK_SIZE	512
#define C_AVI_AUD_ACCEPT_MAX			5

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/ 
typedef enum
{
	AVI_AUDIO_DMA_DONE = C_DMA_STATUS_DONE,
	AVI_AUDIO_RECORD_START = 0x4000,
	AVI_AUDIO_RECORD_STOP,
	AVI_AUDIO_RECORD_RESTART,
	AVI_AUDIO_RECORD_EXIT
} AVI_ENCODE_AUDIO_ENUM;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32S avi_audio_handle_data(INT32U pcm_addr, INT32U pcm_cwlen);
#if AVI_AUDIO_ENCODE_EN == 1
static void avi_audio_record_entry(void *parm);
static INT32S avi_wave_encode_start(void);
static INT32S avi_wave_encode_stop(void);
static INT32S avi_wave_encode_once(INT16S *pcm_input_addr);
#endif

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
#if AVI_AUDIO_ENCODE_EN == 1
INT32U	AviAudioRecordStack[C_AVI_AUDIO_RECORD_STACK_SIZE];
OS_EVENT *avi_aud_q;
OS_EVENT *avi_aud_ack_m;
void *avi_aud_q_stack[C_AVI_AUD_ACCEPT_MAX];
#endif


// function
INT32S avi_adc_record_task_create(INT8U pori)
{	
#if AVI_AUDIO_ENCODE_EN == 1
	INT8U  err;
	INT32S nRet;
	
	avi_aud_q = OSQCreate(avi_aud_q_stack, C_AVI_AUD_ACCEPT_MAX);
	if(!avi_aud_q) RETURN(STATUS_FAIL);
	
	avi_aud_ack_m = OSMboxCreate(NULL);
	if(!avi_aud_ack_m) RETURN(STATUS_FAIL);
	
	err = OSTaskCreate(avi_audio_record_entry, NULL, (void *)&AviAudioRecordStack[C_AVI_AUDIO_RECORD_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) RETURN(STATUS_FAIL);
	
	nRet = STATUS_OK;
Return:
    return nRet;
#else
	return STATUS_OK;
#endif
}

INT32S avi_adc_record_task_del(void)
{
#if AVI_AUDIO_ENCODE_EN == 1
	INT8U  err;
	INT32U nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(avi_aud_q, AVI_AUDIO_RECORD_EXIT, avi_aud_ack_m, 5000, msg, err);
Return:	
	OSQFlush(avi_aud_q);
   	OSQDel(avi_aud_q, OS_DEL_ALWAYS, &err);
	OSMboxDel(avi_aud_ack_m, OS_DEL_ALWAYS, &err);
	return nRet;
#else
	return STATUS_OK;
#endif
}

#if AVI_AUDIO_ENCODE_EN == 1
INT32S avi_audio_record_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(avi_aud_q, AVI_AUDIO_RECORD_START, avi_aud_ack_m, 5000, msg, err);
Return:		
	return nRet;	
}

INT32S avi_audio_record_restart(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(avi_aud_q, AVI_AUDIO_RECORD_RESTART, avi_aud_ack_m, 5000, msg, err);
Return:
	return nRet;
}

INT32S avi_audio_record_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;
	POST_MESSAGE(avi_aud_q, AVI_AUDIO_RECORD_STOP, avi_aud_ack_m, 5000, msg, err);
Return:	
	if(nRet < 0) {
		avi_adc_hw_stop();
		avi_audio_memory_free();
	}
	return nRet;	
}
#endif //AVI_AUDIO_ENCODE_EN == 1

#if AVI_AUDIO_ENCODE_EN == 1
static void avi_audio_record_entry(void *parm)
{
	INT8U   err, audio_flag=0;
	INT32S  ret, audio_stream, encode_size;
	INT32U  msg_id, pcm_addr=0, pcm_cwlen=0, put_cnt;
	INT32U  i, ready_addr=0, next_addr=0;

	INT32S (*aud_rec_dbf_put)(INT16S *data, INT32U cwlen, OS_EVENT *os_q);
	INT32S (*aud_rec_dbf_set)(INT16S *data, INT32U cwlen);
	INT32S (*aud_rec_dma_status_get)(void);
	INT32S (*aud_rec_dbf_status_get)(void);
	void (*aud_rec_dbf_free)(void);
	OS_CPU_SR cpu_sr;
	  	
	while(1) 
	{
		msg_id = (INT32U) OSQPend(avi_aud_q, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}

		switch(msg_id)
		{
			case AVI_AUDIO_DMA_DONE:
				pcm_addr = ready_addr;
				ready_addr = next_addr;		
				next_addr = avi_audio_get_next_buffer(); 		
				aud_rec_dbf_set((INT16S*)next_addr, pcm_cwlen);
				
				encode_size = avi_audio_handle_data(pcm_addr, pcm_cwlen);
				audio_stream = encode_size + 8 + 2*16;
				
				// when restart, wait pcm frame end
				if(audio_flag == 1) {
					audio_flag = 0;
					OSMboxPost(avi_aud_ack_m, (void*)ACK_OK);
				}
				
				if((avi_encode_get_status()&C_AVI_ENCODE_START) == 0) {
					break;
				}
				
				if(!avi_encode_disk_size_is_enough(audio_stream)) {
					avi_enc_storage_full();
					continue;
				}
				
				if(encode_size > 0) {
					ret = pfn_avi_encode_put_data(	pAviEncPara->AviPackerCur->avi_workmem, 
													*(long*)"01wb", 
													encode_size, 
													(INT16U*)pAviEncAudPara->pack_buffer_addr, 
													encode_size/pAviEncPara->AviPackerCur->p_avi_wave_info->nBlockAlign, 
													AVIIF_KEYFRAME);
					if(ret >= 0) {	
						put_cnt++;
						OS_ENTER_CRITICAL();
						pAviEncPara->ta += pAviEncPara->delta_ta;
						OS_EXIT_CRITICAL();
						DEBUG_MSG("A");
					} else {
						avi_encode_disk_size_is_enough(-audio_stream);
						DEBUG_MSG("AudPutData = %x, size = %d!!!\r\n", ret-0x80000000, pcm_cwlen<<1);	
					}
				}
				break;
			
			case AVI_AUDIO_RECORD_START:
				DEBUG_MSG("[AVI_AUDIO_RECORD_START]\r\n");
				audio_flag = 0;
				put_cnt = 0;
				
				// audio encode set
				ret = avi_wave_encode_start();
				if(ret < 0) {
					goto AUDIO_RECORD_START_FAIL;
				}
					
				pcm_cwlen = pAviEncAudPara->pcm_input_size * C_WAVE_ENCODE_TIMES;
				encode_size = pAviEncAudPara->pack_size * C_WAVE_ENCODE_TIMES;
				ret = avi_audio_memory_allocate(pcm_cwlen<<1);
				if(ret < 0) {
					goto AUDIO_RECORD_START_FAIL;
				}
				
				// post process set
			#if APP_LPF_ENABLE == 1
            	LPF_init(pAviEncAudPara->audio_sample_rate, 3);
			#endif	
			
				// analog input set
			#if MIC_INPUT_SRC == C_ADC_LINE_IN
				aud_rec_dbf_put = drv_l1_adc_dbf_put;
				aud_rec_dbf_set = drv_l1_adc_dbf_set;
				aud_rec_dma_status_get = drv_l1_adc_dma_status_get;
				aud_rec_dbf_status_get = drv_l1_adc_dbf_status_get;
				aud_rec_dbf_free = drv_l1_adc_dbf_free;
			#elif MIC_INPUT_SRC == C_BUILDIN_MIC_IN || MIC_INPUT_SRC == C_LINE_IN_LR
				aud_rec_dbf_put = drv_l1_i2s_rx_dbf_put;
				aud_rec_dbf_set = drv_l1_i2s_rx_dbf_set;
				aud_rec_dma_status_get = drv_l1_i2s_rx_dma_status_get;
				aud_rec_dbf_status_get = drv_l1_i2s_rx_dbf_status_get;
				aud_rec_dbf_free = drv_l1_i2s_rx_dbf_free;
			#endif
			 
				ready_addr = avi_audio_get_next_buffer();
				next_addr = avi_audio_get_next_buffer();
				ret = aud_rec_dbf_put((INT16S*)ready_addr, pcm_cwlen, avi_aud_q);
				if(ret < 0) {
					goto AUDIO_RECORD_START_FAIL;
				}
				
				ret = aud_rec_dbf_set((INT16S*)next_addr, pcm_cwlen);
				if(ret < 0) {
					goto AUDIO_RECORD_START_FAIL;
				}
				
				avi_adc_hw_start(pAviEncAudPara->audio_sample_rate);
				pAviEncPara->delta_ta = (INT64S)pAviEncVidPara->dwRate * pcm_cwlen;
				if(pAviEncAudPara->channel_no == 2) {
					pAviEncPara->delta_ta >>= 1;
				}
				OSMboxPost(avi_aud_ack_m, (void*)ACK_OK);
				break;
				
AUDIO_RECORD_START_FAIL:
				avi_adc_hw_stop();
				aud_rec_dbf_free();
				avi_wave_encode_stop();
				avi_audio_memory_free();
				DBG_PRINT("AudEncStartFail!!!\r\n");
				OSMboxPost(avi_aud_ack_m, (void*)ACK_FAIL);
				break;	
				
			case AVI_AUDIO_RECORD_STOP:
				DEBUG_MSG("[AVI_AUDIO_RECORD_STOP]\r\n");
				// wait dma stop
				while(aud_rec_dbf_status_get() == 1 || aud_rec_dma_status_get() == 1) {
					OSTimeDly(2);
				}
				
				avi_adc_hw_stop();
				aud_rec_dbf_free();
				
				// handle the final data
				for(i=0; i<2; i++) { 
					if(i == 0) {
						pcm_addr = ready_addr; 
						ready_addr = 0;
					} else {
						pcm_addr = next_addr;
						next_addr = 0;
					}
					
					if(pcm_addr == 0) {
						break;
					}
				
					encode_size = avi_audio_handle_data(pcm_addr, pcm_cwlen);
					if(encode_size <= 0) {
						break;
					}
					
					ret = pfn_avi_encode_put_data(pAviEncPara->AviPackerCur->avi_workmem, 
												*(long*)"01wb", 
												encode_size, 
												(INT16U*)pAviEncAudPara->pack_buffer_addr, 
												encode_size/pAviEncPara->AviPackerCur->p_avi_wave_info->nBlockAlign, 
												AVIIF_KEYFRAME);
					if(ret >= 0) {	
						OS_ENTER_CRITICAL();
						pAviEncPara->ta += pAviEncPara->delta_ta;
						OS_EXIT_CRITICAL();
						DEBUG_MSG("A");
					}							
				}	
				
				avi_wave_encode_stop();
				avi_audio_memory_free();
				OSMboxPost(avi_aud_ack_m, (void*)ACK_OK);
				break;
		
			case AVI_AUDIO_RECORD_RESTART:
				DEBUG_MSG("[AVI_AUDIO_RECORD_RESTART]\r\n");
				audio_flag = 1;
				break;
			
			case AVI_AUDIO_RECORD_EXIT:
				DEBUG_MSG("[AVI_AUDIO_RECORD_EXIT]\r\n");
				OSQFlush(avi_aud_q);
				OSMboxPost(avi_aud_ack_m, (void*)ACK_OK);
				OSTaskDel(OS_PRIO_SELF);
				break;
		}	
	}
}

static INT32S avi_audio_handle_data(INT32U pcm_addr, INT32U pcm_cwlen)
{
#if (MIC_INPUT_SRC == C_ADC_LINE_IN) 
	INT16U *pData = (INT16U *)pcm_addr; 
	INT16U t, mask = 0x8000;
	INT32U i;
	
	// invalid cache
	cache_invalid_range(pcm_addr, pcm_cwlen << 1);
	
	// unsigned to signed
	for(i=0; i<pcm_cwlen; i++) 
	{
		t = *pData;
		t ^= mask;
	#if APP_LPF_ENABLE == 1
		t = LPF_process(t);
	#endif
		*pData++ = t;
	}
	
#elif (MIC_INPUT_SRC == C_BUILDIN_MIC_IN) || (MIC_INPUT_SRC == C_LINE_IN_LR)
#if APP_LPF_ENABLE == 1

	INT16U *pData = (INT16U *)pcm_addr;
	INT16U t;
	INT32U i;
	
	for(i=0; i<pcm_cwlen; i++) 
	{
		t = *pData;		
		t = LPF_process(t);
		*pData++ = t;
	}

#endif	
#endif	
	
#if AUDIO_SFX_HANDLE == 1
	pcm_addr = (INT16U *)video_encode_audio_sfx((INT16U *)pcm_addr, pcm_cwlen<<1);
#endif
	
	// wave encode		
	return avi_wave_encode_once((INT16S*)pcm_addr);
} 

static INT32S avi_wave_encode_start(void)
{
#if APP_WAV_CODEC_EN == 1
	INT32S nRet, size;
	INT32U audio_format;
	
	size = wav_enc_get_mem_block_size();
	pAviEncAudPara->work_mem = (INT8U *)gp_malloc_align(size, 4);
	if(!pAviEncAudPara->work_mem) {
		RETURN(STATUS_FAIL);
	}
	
	gp_memset((INT8S*)pAviEncAudPara->work_mem, 0, size);
	if(pAviEncAudPara->audio_format == WAV) {
		audio_format = WAVE_FORMAT_PCM;
	} else if(pAviEncAudPara->audio_format == MICROSOFT_ADPCM) {
		audio_format = WAVE_FORMAT_ADPCM;
	} else if(pAviEncAudPara->audio_format == IMA_ADPCM) {
		audio_format = WAVE_FORMAT_IMA_ADPCM;
	} else if(pAviEncAudPara->audio_format == ALAW) {
		audio_format = WAVE_FORMAT_ALAW;
	} else if(pAviEncAudPara->audio_format == ALAW) {
		audio_format = WAVE_FORMAT_MULAW;
	} else {
		RETURN(STATUS_FAIL);
	}
	
	nRet = wav_enc_Set_Parameter( pAviEncAudPara->work_mem, 
								  pAviEncAudPara->channel_no, 
								  pAviEncAudPara->audio_sample_rate, 
								  audio_format);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = wav_enc_init(pAviEncAudPara->work_mem);
	if(nRet < 0) {
		RETURN(STATUS_FAIL);
	}
	
	pAviEncAudPara->pcm_input_size = wav_enc_get_SamplePerFrame(pAviEncAudPara->work_mem);
	switch(pAviEncAudPara->audio_format)
	{
	case WAV:
		pAviEncAudPara->pack_size = pAviEncAudPara->pcm_input_size;	
		pAviEncAudPara->pack_size *= 2;
		break;
	
	case ALAW:
	case MULAW:	
	case MICROSOFT_ADPCM:
	case IMA_ADPCM:
		pAviEncAudPara->pack_size = wav_enc_get_BytePerPackage(pAviEncAudPara->work_mem);	
		break;
	}
	
	nRet = STATUS_OK;
Return:	
	return nRet;	
#else
	return STATUS_FAIL;
#endif
}

static INT32S avi_wave_encode_stop(void)
{
	if(pAviEncAudPara->work_mem) {
		gp_free((void*)pAviEncAudPara->work_mem);
		pAviEncAudPara->work_mem = 0;
	}
	return STATUS_OK;
}

static INT32S avi_wave_encode_once(INT16S *pcm_input_addr)
{
#if APP_WAV_CODEC_EN == 1
	INT8U *encode_output_addr;
	INT32S nRet, encode_size, N;
	
	encode_size = 0;
	N = C_WAVE_ENCODE_TIMES;
	encode_output_addr = (INT8U*)pAviEncAudPara->pack_buffer_addr;
	while(N--) {
		nRet = wav_enc_run(pAviEncAudPara->work_mem, (short *)pcm_input_addr, encode_output_addr);
		if(nRet < 0) {		
			return STATUS_FAIL;
		}
		
		encode_size += nRet;
		pcm_input_addr += pAviEncAudPara->pcm_input_size;
		encode_output_addr += pAviEncAudPara->pack_size;
	}
	
	return encode_size;
#else
	return  STATUS_FAIL;
#endif
}
#endif //AVI_AUDIO_ENCODE_EN
