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
#include "audio_record.h"
#include <stdlib.h>
#include "drv_l1_adc.h"
#include "drv_l1_i2s.h"
#include "drv_l1_dma.h"
#include "drv_l1_timer.h"
#include "drv_l1_gpio.h"
#include "drv_l1_cache.h"
#include "drv_l1_dac.h"
#include "wav_dec.h"
#include "wav_enc.h"
#if APP_A1800_ENCODE_EN == 1
#include "A1800Enc.h"
#endif
#if APP_MP3_ENCODE_EN == 1
#include "mp3enc.h"
#endif
#if APP_DOWN_SAMPLE_EN == 1
#include "DownSample_api.h"
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// error message
#define AUD_RECORD_STATUS_OK			0x00000000
#define AUD_RECORD_STATUS_ERR			0x80000001
#define AUD_RECORD_INIT_ERR				0x80000002
#define AUD_RECORD_DMA_ERR				0x80000003
#define AUD_RECORD_RUN_ERR				0x80000004
#define AUD_RECORD_FILE_WRITE_ERR		0x80000005
#define AUD_RECORD_MEMORY_ALLOC_ERR		0x80000006

// audio record configure
#define ADC_RECORD_STACK_SIZE			1024
#define AUD_ENC_QACCEPT_MAX				5

#define C_AUD_PCM_BUFFER_NO				3						//pcm buffer number
#define C_A1800_RECORD_BUFFER_SIZE		A18_ENC_FRAMESIZE		//PCM buffer size, fix 320
#define C_WAVE_RECORD_BUFFER_SIZE		1024*16					//PCM buffer size, depend on SR=16KHz 	
#define C_BS_BUFFER_SIZE				1024*64					//file buffer size, fix 64Kbyte 	

#define A1800_TIMES						30						//a1800 encode times, 320*30, <80*30, depend on SR=16KHz
#define ADPCM_TIMES						16						//adpcm encode times, 500*16, 256*16, depend on SR=16KHz
#define MP3_TIME						15						//MP3 encode times, 1152*15

// for adc and mic use
#define C_ADC_USE_TIMER					ADC_AS_TIMER_C			//adc:ADC_AS_TIMER_C ~ F, mic: ADC_AS_TIMER_C ~ F
#define C_ADC_USE_CHANNEL				ADC_LINE_1				//adc channel: 0 ~ 3 

// pcm energy detect threshold 
#define PCM_GLOBAL_THR 					0x800					//Frame Energy Threshold to judge as active							
#define PCM_LOCAL_THR  					0x800 					//Local Energy Threshold of each sample  

// adc record and dac play at same time 
#define RECORD_WITH_DAC_OUT_EN			0						//1: enable, 0: disable				

#define ENERGY_DETECT_EN				0

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DBG		DBG_PRINT
#else
	#define DBG(...)
#endif
 
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/ 
typedef enum
{
	MSG_ADC_DMA_DONE = C_DMA_STATUS_DONE,
	MSG_AUDIO_ENCODE_START = 0x10000000,
	MSG_AUDIO_ENCODE_STOP,
	MSG_AUDIO_ENCODE_ERR,
	MSG_AUDIO_ENCODE_EXIT
} AUDIO_RECORD_ENUM;

typedef struct 
{
	INT8U	RIFF_ID[4];	//= {'R','I','F','F'};
	INT32U 	RIFF_len;	//file size -8
	INT8U 	type_ID[4];	// = {'W','A','V','E'};
	INT8U	fmt_ID[4];	// = {'f', 'm','t',' '};
	INT32U  fmt_len;	//16 + extern format byte
	INT16U  format;		// = 1; 	//pcm
	INT16U	channel;	// = 1;	// mono
	INT32U  sample_rate;// = 8000;
	INT32U  avg_byte_per_sec;// = 8000*2;	//AvgBytesPerSec = SampleRate * BlockAlign 
	INT16U	Block_align;// = (16 / 8*1) ;				//BlockAlign = SignificantBitsPerSample / 8 * NumChannels 
	INT16U	Sign_bit_per_sample;// = 16;		//8, 16, 24 or 32
	INT8U	data_ID[4];// = {'d','a','t','a'};
	INT32U	data_len; //extern format byte
} AUD_ENC_WAVE_HEADER;

typedef struct AudRecCtrl_s 
{
	OS_EVENT *aud_rec_q;
	OS_EVENT *aud_rec_m;
	OS_EVENT *aud_dac_q;
	void	*aud_rec_q_buf[AUD_ENC_QACCEPT_MAX];
	void	*aud_dac_q_buf[AUD_ENC_QACCEPT_MAX];	
	INT32U  task_stack[ADC_RECORD_STACK_SIZE];
	INT32U  task_running;		// task running flag
	INT32U  lock;				// encode loack
	
	AudEncInfo_t info;			// audio encode information
	INT32U  Status;				// audio encode status		
	INT32U  FileLenth;			// byte 
	INT32U  NumSamples;			// sample
	INT64U  disk_free_size;		// allow record size
	
#if	APP_DOWN_SAMPLE_EN	
	INT8U 	*DNSR;				// down sample work memory
	INT8U   down_sample_en;		// down sample enable
	INT8U   down_sample_factor; // down sample factor
#endif
		
	// audio library encode
	INT8U 	*work_mem;			// audio encode library wrok memory
	INT8U   *bitstream_buf;		// encode bitstream output buffer
	INT32U  ri;					// bitstream read index
	INT32U	pack_size;			// encode bitstream output size
	INT8U   *pack_buf;			// a1800 and wav lib use
	INT32U	PCMInFrameSize;		// pcm input buffer, short
	INT32U  OnePCMFrameSize; 	// N * PCMInFrameSize
	
	INT32U  ring_buf;
	INT32U  pcm_buf[C_AUD_PCM_BUFFER_NO];
	INT32U  buf_idx;
	INT32U  ready_buf;
	INT32U  next_buf;
} AudRecCtrl_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void audio_record_entry(void *param);
static void aud_rec_lock(AudRecCtrl_t *pAudRec);
static void aud_rec_unlock(AudRecCtrl_t *pAudRec);
static INT32S aud_rec_send_message(AudRecCtrl_t *pAudRec, INT32U message);
static INT64U aud_rec_set_free_size(INT16S fd);
static void aud_rec_mem_free(AudRecCtrl_t *pAudRec);

static INT32S adc_start(AudRecCtrl_t *pAudRec);
static void adc_stop(void);
static INT32U adc_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec);

static INT32S mic_start(AudRecCtrl_t *pAudRec);
static void mic_stop(void);
static INT32U mic_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec);

static INT32S line_in_lr_start(AudRecCtrl_t *pAudRec);
static void line_in_lr_stop(void);
static INT32U line_in_lr_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec);

#if RECORD_WITH_DAC_OUT_EN == 1
static void dac_output_set(INT32U state, AudRecCtrl_t *pAudRec, INT32U pcm_buf, INT32U cwlen);
#endif

#if ENERGY_DETECT_EN == 1
static INT32S pcm_energy_detect(INT16S* buffer_addr, INT32U pcm_size);
#endif

static INT32S save_buffer_to_storage(AudRecCtrl_t *pAudRec);
static INT32S save_final_data_to_storage(AudRecCtrl_t *pAudRec); 
 
//wave
static void audenc_RIFF_init(AUD_ENC_WAVE_HEADER *pHeader, INT32U samplerate);
static INT32S wave_encode_start(void *rec_wm);
static INT32S wave_encode_stop(void *rec_wm);
static int wave_encode_once(void *workmem, const short* buffer_addr, int cwlen);

#if APP_A1800_ENCODE_EN	== 1
static INT32S a1800_encode_start(void *rec_wm);
static INT32S a1800_encode_stop(void *rec_wm);
static int a1800_encode_once(void *workmem, const short* buffer_addr, int cwlen);
#endif

#if APP_WAV_CODEC_EN == 1
static INT32S wave_encode_lib_start(void *rec_wm);
static INT32S wave_encode_lib_stop(void *rec_wm);
static int wave_encode_lib_once(void *workmem, const short* buffer_addr, int cwlen);
#endif

#if APP_MP3_ENCODE_EN == 1
static INT32S mp3_encode_start(void *rec_wm);
static INT32S mp3_encode_stop(void *rec_wm);
static int mp3_encode_once(void *workmem, const short* buffer_addr, int cwlen);
#endif
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/ 
static OS_EVENT *AudRec_Sema;
static AudRecCtrl_t *g_audio_rec;
static AUD_ENC_WAVE_HEADER WaveHeadPara;
INT32S (*audenc_user_write)(INT32U device, INT8U bHeader, INT32U buffer_addr, INT32U cbLen);


/**
 * @brief   create audio record task
 * @param   priority[in]: task priority
 * @param   type[in]: analig input device
 * @return 	workmem
 */
void *audio_record_task_create(INT8U priority)
{
	INT8U err;
	AudRecCtrl_t *pAudRec;
	
	if(AudRec_Sema == 0) {
		AudRec_Sema = OSSemCreate(1);
	}
	
	pAudRec = (AudRecCtrl_t *)gp_malloc_align(sizeof(AudRecCtrl_t), 4);
	if(pAudRec == 0) {
		return 0;
	}

	gp_memset((INT8S *)pAudRec, 0x00, sizeof(AudRecCtrl_t));
	pAudRec->aud_rec_q = OSQCreate(pAudRec->aud_rec_q_buf, AUD_ENC_QACCEPT_MAX);
	if(pAudRec->aud_rec_q == 0) {
		goto __fail;
	}

	pAudRec->aud_rec_m = OSMboxCreate(NULL);
	if(pAudRec->aud_rec_m == 0) {
		goto __fail;
	}
	
	pAudRec->aud_dac_q = OSQCreate(pAudRec->aud_dac_q_buf, AUD_ENC_QACCEPT_MAX);
	if(pAudRec->aud_dac_q == 0) {
		goto __fail;
	}
	
	err = OSTaskCreate(audio_record_entry, (void *)pAudRec, (void *)&pAudRec->task_stack[ADC_RECORD_STACK_SIZE - 1], priority);
	if(err != OS_NO_ERR) {
		goto __fail;
	} 
	
	pAudRec->task_running = 1;
	return (void *)pAudRec;
	
__fail:
	audio_record_task_delete((void *)pAudRec);
	return 0;	
}

/**
 * @brief   delete audio record task
 * @param   work_mem[in]: work memory
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_record_task_delete(void *work_mem)
{
	INT8U err;
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)work_mem;
	
	if(pAudRec->task_running) {
		aud_rec_send_message(pAudRec, MSG_AUDIO_ENCODE_EXIT);
		pAudRec->task_running = 0;
	}
	
	if(pAudRec->aud_rec_q) {
		OSQFlush(pAudRec->aud_rec_q);
		OSQDel(pAudRec->aud_rec_q, OS_DEL_ALWAYS, &err);
	}
	
	if(pAudRec->aud_rec_m) {
		OSMboxDel(pAudRec->aud_rec_q, OS_DEL_ALWAYS, &err);
	}
	
	if(pAudRec->aud_dac_q) {
		OSQFlush(pAudRec->aud_dac_q);
		OSQDel(pAudRec->aud_dac_q, OS_DEL_ALWAYS, &err);
	}
	
	gp_free((void *)pAudRec);
	return STATUS_OK;
}

/**
 * @brief   start audio record
 * @param   work_mem[in]: work memory
 * @param   pInfo[in]: audio information
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S adc_record_task_start(void *work_mem, AudEncInfo_t *pInfo)
{
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)work_mem;
	
	if(pAudRec->task_running) {
		gp_memcpy((INT8S *)&pAudRec->info, (INT8S *)pInfo, sizeof(AudEncInfo_t));
		aud_rec_send_message(pAudRec, MSG_AUDIO_ENCODE_START);
	}
		
	return STATUS_OK;
}

/**
 * @brief   stop audio record
 * @param   work_mem[in]: work memory
 * @param   pInfo[in]: audio information
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S adc_record_task_stop(void *work_mem)
{
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)work_mem;
	
	if(pAudRec->task_running) {
		aud_rec_send_message(pAudRec, MSG_AUDIO_ENCODE_STOP);
	}
		
	return STATUS_OK;
}

/**
 * @brief   audio record get status
 * @param   work_mem[in]: work memory
 * @return 	status
 */
INT32U audio_record_get_status(void *work_mem)
{
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)work_mem;
	
	return pAudRec->Status;
}

/**
 * @brief   audio record set down sample
 * @param   work_mem[in]: work memory
 * @param   down_sample_en[in]: down sample enable or disable
 * @param   down_sample_factor[in]: 2~4
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S audio_record_set_down_sample(void *work_mem, INT32U down_sample_en, INT32U down_sample_factor)
{
#if	APP_DOWN_SAMPLE_EN == 1
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)work_mem;
	pAudRec->down_sample_en = down_sample_en;
	pAudRec->down_sample_factor = down_sample_factor;
	return STATUS_OK;
#else
	return STATUS_FAIL;
#endif
}

static void audio_record_entry(void *param)
{

#if RECORD_WITH_DAC_OUT_EN == 1
	INT8U dac_flag;
#endif
	INT8U ack_flag=0;
	INT8U err;
	INT32U msg_id;
	INT32U pcm_buf;
	INT32S i, ret;
	
	AudRecCtrl_t *pAudRec = (AudRecCtrl_t *)param;
	AudEncInfo_t *pInfo = &pAudRec->info;
	OS_EVENT *aud_rec_q = pAudRec->aud_rec_q;
	OS_EVENT *aud_rec_m = pAudRec->aud_rec_m;
	OS_EVENT *aud_dac_q = pAudRec->aud_dac_q;
	
	INT32S (*device_start)(AudRecCtrl_t *pAudRec);
	void (*device_stop)(void)=NULL;
	INT32U (*device_handle_data)(INT32U stop_en, AudRecCtrl_t *pAudRec)=NULL;
	
	void *hPP=NULL;
	INT32S (*audenc_init)(void *rec_wm);
	INT32S (*audenc_stop)(void *rec_wm)=NULL;
	INT32S (*audenc_once)(void *workmem, const short* buffer_addr, int cwlen)=NULL;
	
	while(1) 
	{
		msg_id = (INT32U) OSQPend(aud_rec_q, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}
		
		switch(msg_id)
		{
		case MSG_ADC_DMA_DONE:
			pcm_buf = device_handle_data(0, pAudRec);
			if(pcm_buf == 0) {
				DBG("pcm_buf = 0, Post MSG_AUDIO_ENCODE_ERR.\r\n");
				OSQPost(aud_rec_q, (void *) MSG_AUDIO_ENCODE_ERR);	
				continue;
			}
			
		#if RECORD_WITH_DAC_OUT_EN == 1	
			if(dac_flag == 0) {	
				dac_output_set(0, pAudRec, pcm_buf, pAudRec->PCMInFrameSize);
				dac_flag = 1;
			} else if(dac_flag == 1) {
				dac_output_set(1, pAudRec, pcm_buf, pAudRec->PCMInFrameSize);
				dac_flag = 2;
			} else {
				dac_output_set(2, pAudRec, pcm_buf, pAudRec->PCMInFrameSize);
			} 
		#endif
			
			// energy detect
		#if ENERGY_DETECT_EN == 1	
			pcm_energy_detect((INT16S *)pcm_buf, pAudRec->PCMInFrameSize);
		#endif
		
			// encode pcm wave 
			DBG(".");
			aud_rec_lock(pAudRec);
			ret = audenc_once(hPP, (const short*)pcm_buf, pAudRec->PCMInFrameSize); 
			aud_rec_unlock(pAudRec);
			
			if(ret < 0) {
				DBG("audenc_once < 0, Post MSG_AUDIO_ENCODE_ERR.\r\n");
				OSQPost(aud_rec_q, (void *) MSG_AUDIO_ENCODE_ERR);	
				continue;
			}
			
			// check storage is full
			if(pInfo->source_type == C_GP_FS) {
				if(pAudRec->FileLenth >= pAudRec->disk_free_size) {
					DBG("disk full, Post MSG_AUDIO_ENCODE_ERR.\r\n");
					OSQPost(aud_rec_q, (void *) MSG_AUDIO_ENCODE_ERR);
				}
			}
			break;
		
		case MSG_AUDIO_ENCODE_START:
			DBG("[MSG_AUDIO_ENCODE_START]\r\n");
			#if RECORD_WITH_DAC_OUT_EN == 1
			dac_flag = 0;
			#endif
			ack_flag = 0;
			hPP = 0;
			
			// check storage free size
			if(pInfo->source_type == C_GP_FS) {
				pAudRec->disk_free_size = aud_rec_set_free_size(pInfo->fd);
			}
			
			// audio format set
			switch(pInfo->audio_format)
			{
			#if APP_A1800_ENCODE_EN	== 1
			case A1800:
				audenc_init = a1800_encode_start;
				audenc_once = a1800_encode_once;
				audenc_stop = a1800_encode_stop;
				break;
			#endif
			case PCM:
				audenc_init = wave_encode_start;
				audenc_once = wave_encode_once;
				audenc_stop = wave_encode_stop;
				break;
			#if	APP_WAV_CODEC_EN == 1	
			case WAV:	
			case IMA_ADPCM:
			case MICROSOFT_ADPCM:
			case ALAW:
			case MULAW:
				audenc_init = wave_encode_lib_start;
				audenc_once = wave_encode_lib_once;
				audenc_stop = wave_encode_lib_stop;
				break;
			#endif	
			#if APP_MP3_ENCODE_EN == 1
			case MP3:
				audenc_init = mp3_encode_start;	
				audenc_once = mp3_encode_once;
				audenc_stop = mp3_encode_stop;
				break;
			#endif
			default:
				DBG("audio encode fmt err.\r\n");
				goto __START_FAIL;
			}
			
			ret = audenc_init(pAudRec);
			if(ret < 0) {
				DBG("audio encode start fail.\r\n");
				goto __START_FAIL;
			}
			
			DBG("Dev=0x%x, Ch=%d, SR=%d\r\n", pInfo->input_device, pInfo->channel_no, pInfo->sample_rate);
			
			// post process init
			hPP = pAudRec->work_mem;			
		#if	APP_DOWN_SAMPLE_EN == 1
			if(pAudRec->down_sample_en) {
				if(pAudRec->down_sample_factor * pInfo->sample_rate > 48000) {
					pAudRec->down_sample_factor = 48000 / pInfo->sample_rate;
				}
					
				if(pAudRec->down_sample_factor >= 2) {
					pAudRec->DNSR = DownSample_Create(pAudRec->PCMInFrameSize, pAudRec->down_sample_factor);	
					
					DownSample_Link(pAudRec->DNSR,	
									hPP, 
									audenc_once, 
									pInfo->sample_rate,
									pInfo->channel_no, 
									pAudRec->down_sample_factor);
					
					hPP = pAudRec->DNSR;
					audenc_once = DownSample_PutData;
					pInfo->sample_rate = DownSample_GetSampleRate(hPP);
					pInfo->channel_no = DownSample_GetChannel(hPP);
					pAudRec->PCMInFrameSize *= pAudRec->down_sample_factor; 	
				}
			}
		#endif
		#if	APP_LPF_ENABLE == 1			
			LPF_init(pInfo->sample_rate, 3);
		#endif	
			
			// input device set
			switch(pInfo->input_device)
			{
			case ADC_LINE_IN:
				if(pInfo->channel_no != 1) {
					DBG("ChErr.\r\n");
					goto __START_FAIL;
				}
				
				device_start = adc_start;
				device_stop = adc_stop;
				device_handle_data = adc_handle_data;
				break;
			case MIC_LINE_IN:
				if(pInfo->channel_no != 1) {
					DBG("ChErr.\r\n");
					goto __START_FAIL;
				}
				
				device_start = mic_start;
				device_stop = mic_stop;
				device_handle_data = mic_handle_data;
				break;
			case LR_LINE_IN:
				if(pInfo->channel_no != 2) {
					DBG("ChErr.\r\n");
					goto __START_FAIL;
				}
				
				device_start = line_in_lr_start;
				device_stop = line_in_lr_stop;
				device_handle_data = line_in_lr_handle_data;
				break;
			default:
				DBG("InputDeviceErr.\r\n");
				goto __START_FAIL;
			}
			
			ret = device_start(pAudRec);
			if(ret < 0) {
				goto __START_FAIL;
			}
			
			pAudRec->Status = C_START_RECORD;
			OSMboxPost(aud_rec_m, (void *)ACK_OK);
			break;
			
__START_FAIL:
			DBG("AudioEncodeStartFail!!!\r\n");
			aud_rec_mem_free(pAudRec);
			pAudRec->Status = C_STOP_RECORD;
			OSMboxPost(aud_rec_m, (void *)ACK_FAIL);
			break;			
		
		case MSG_AUDIO_ENCODE_STOP:
			DBG("[MSG_AUDIO_ENCODE_STOP]\r\n");
			ack_flag = 1;
		case MSG_AUDIO_ENCODE_ERR:
			device_stop();
			for(i=0; i<2; i++) {
				pcm_buf = device_handle_data(1, pAudRec);
				if(pcm_buf == 0) {
					break;
				}
				
				aud_rec_lock(pAudRec);
				ret = audenc_once(hPP, (const short*)pcm_buf, pAudRec->PCMInFrameSize);
				aud_rec_unlock(pAudRec);
			}
			
			// stop
			audenc_stop(pAudRec);
						
		#if RECORD_WITH_DAC_OUT_EN == 1
			dac_output_set(3, pAudRec, 0, 0);	
		#endif
		
			aud_rec_mem_free(pAudRec);			
			pAudRec->Status = C_STOP_RECORD;
			OSQFlush(aud_rec_q);
			OSQFlush(aud_dac_q);
			if(ack_flag) {
				OSMboxPost(aud_rec_m, (void *)ACK_OK);
				ack_flag = 0;
			}
			break;
			
		case MSG_AUDIO_ENCODE_EXIT:
			OSMboxPost(aud_rec_m, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;	
		}
	}
}

static void aud_rec_lock(AudRecCtrl_t *pAudRec)
{
	INT8U err;
	
	OSSemPend(AudRec_Sema, 0, &err);
	pAudRec->lock = 1;
	g_audio_rec = pAudRec;
}

static void aud_rec_unlock(AudRecCtrl_t *pAudRec)
{
	if(pAudRec->lock) {
		g_audio_rec = 0;
		pAudRec->lock = 0;
		OSSemPost(AudRec_Sema);
	}
}

static INT32S aud_rec_send_message(AudRecCtrl_t *pAudRec, INT32U message)
{
	INT8U err;
	INT32S ack_msg;

	err = OSQPost(pAudRec->aud_rec_q, (void *)message);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	ack_msg = (INT32S)OSMboxPend(pAudRec->aud_rec_m, 5000, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return ack_msg;
}

static INT64U aud_rec_set_free_size(INT16S fd)
{
	INT8U  fs_disk;
	INT64U disk_free = 0;
	
	if(fd >= 0) {
		fs_disk = GetDiskOfFile(fd);
		disk_free = vfsFreeSpace(fs_disk);
		return (disk_free - 10*1024); //reserved 10K
	}
	
	return 0;
}

static void aud_rec_mem_free(AudRecCtrl_t *pAudRec)
{
	INT32U i;

#if APP_DOWN_SAMPLE_EN == 1
	if(pAudRec->DNSR) {
		DownSample_Del(pAudRec->DNSR);
		pAudRec->DNSR = 0;
		pAudRec->down_sample_en = 0;
		pAudRec->down_sample_factor = 0;
	}
#endif	
	
	if(pAudRec->pcm_buf[0]) {
		gp_free((void *)pAudRec->pcm_buf[0]);
	}
	
	for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
		pAudRec->pcm_buf[i] = 0;
	}
	
	if(pAudRec->work_mem) {
		gp_free((void *)pAudRec->work_mem);
		pAudRec->work_mem = 0;
	}
				
	if(pAudRec->bitstream_buf) {
		gp_free((void *)pAudRec->bitstream_buf);
		pAudRec->bitstream_buf = 0;
	}
				
	if(pAudRec->pack_buf) {
		gp_free((void *)pAudRec->pack_buf);
		pAudRec->pack_buf = 0;
	}
}

static INT32S adc_start(AudRecCtrl_t *pAudRec)
{
	INT32S i;
	INT32U size;
	
	// allocate memory
	size = (pAudRec->PCMInFrameSize << 1);
	pAudRec->pcm_buf[0] =(INT32U) gp_malloc_align(size * C_AUD_PCM_BUFFER_NO, 4);
	if(pAudRec->pcm_buf[0] == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S *)pAudRec->pcm_buf[0], 0x00, size * C_AUD_PCM_BUFFER_NO);			
	for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
		pAudRec->pcm_buf[i] = pAudRec->pcm_buf[0] + size * i;
		DBG("pcm_buf[%d] = 0x%x\r\n", i, pAudRec->pcm_buf[i]);
	}
				
	// set dma 			
	pAudRec->ready_buf = pAudRec->pcm_buf[0];
	pAudRec->next_buf = pAudRec->pcm_buf[1];
	pAudRec->buf_idx = 1;
	drv_l1_adc_dbf_put((INT16S*)pAudRec->ready_buf, pAudRec->PCMInFrameSize, pAudRec->aud_rec_q);
	drv_l1_adc_dbf_set((INT16S*)pAudRec->next_buf, pAudRec->PCMInFrameSize);
				
	// set adc 
	drv_l1_adc_fifo_clear();
	drv_l1_adc_user_line_in_en(C_ADC_USE_CHANNEL, ENABLE);
	drv_l1_adc_auto_ch_set(C_ADC_USE_CHANNEL);
	drv_l1_adc_fifo_level_set(4);
	drv_l1_adc_auto_sample_start();
	
	OSTimeDly(50); //wait bais stable
	drv_l1_adc_sample_rate_set(C_ADC_USE_TIMER, pAudRec->info.sample_rate);
	return STATUS_OK;
}

static void adc_stop(void)
{
	while(drv_l1_adc_dbf_status_get() == 1 || drv_l1_adc_dma_status_get() == 1) {
		OSTimeDly(2);
	}
	
	drv_l1_adc_dbf_free();
	drv_l1_adc_timer_stop(C_ADC_USE_TIMER);
}

static INT32U adc_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec)
{
	INT16U *pSrc;
	INT16U i, temp;
	INT32U ready_buf;
	INT32U mask = 0x8000;
	INT32U frame_size = pAudRec->PCMInFrameSize;
	
	if(pAudRec->ready_buf == 0) {
		return 0;
	}
	
	ready_buf = pAudRec->ready_buf;
	pAudRec->ready_buf = pAudRec->next_buf;

	// stop encode
	if(stop_en == 0) {
		pAudRec->buf_idx++;
		if(pAudRec->buf_idx >= C_AUD_PCM_BUFFER_NO) {
			pAudRec->buf_idx = 0;
		}
	
		pAudRec->next_buf = pAudRec->pcm_buf[pAudRec->buf_idx];
		drv_l1_adc_dbf_set((INT16S *)pAudRec->next_buf, frame_size);
	} else {
		pAudRec->next_buf = 0;
	}
							
	// invalid cache
	cache_invalid_range(ready_buf, frame_size << 1);
				
	// unsigned to signed
	pSrc = (INT16U *)ready_buf;
	for(i=0; i<frame_size; i++)
	{
		temp = *pSrc;
		temp ^= mask;
	#if APP_LPF_ENABLE == 1
		temp = (INT16U)LPF_process(temp);		
	#endif
		*pSrc++ = temp;
	}
	
	return ready_buf;
}

static INT32S mic_start(AudRecCtrl_t *pAudRec)
{
	INT32S i;
	INT32U size;
	
	// allocate memory
	size = (pAudRec->PCMInFrameSize << 1);
	pAudRec->pcm_buf[0] =(INT32U) gp_malloc_align(size * C_AUD_PCM_BUFFER_NO, 4);
	if(pAudRec->pcm_buf[0] == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S *)pAudRec->pcm_buf[0], 0x00, size * C_AUD_PCM_BUFFER_NO);			
	for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
		pAudRec->pcm_buf[i] = pAudRec->pcm_buf[0] + size * i;
		DBG("pcm_buf[%d] = 0x%x\r\n", i, pAudRec->pcm_buf[i]);
	}
				
	// set dma 			
	pAudRec->ready_buf = pAudRec->pcm_buf[0];
	pAudRec->next_buf = pAudRec->pcm_buf[1];
	pAudRec->buf_idx = 1;
	drv_l1_i2s_rx_dbf_put((INT16S*)pAudRec->ready_buf, pAudRec->PCMInFrameSize, pAudRec->aud_rec_q);
	drv_l1_i2s_rx_dbf_set((INT16S*)pAudRec->next_buf, pAudRec->PCMInFrameSize);
				
	// set i2s
	drv_l1_i2s_rx_set_input_path(MIC_INPUT);
	drv_l1_i2s_rx_init();
	drv_l1_i2s_rx_sample_rate_set(pAudRec->info.sample_rate);
	drv_l1_i2s_rx_mono_ch_set();
	drv_l1_i2s_rx_start();
	return STATUS_OK;
}

static void mic_stop(void)
{
	while(drv_l1_i2s_rx_dma_status_get() == 1 || drv_l1_i2s_rx_dbf_status_get() == 1) {
		OSTimeDly(2);
	}
	
	drv_l1_i2s_rx_dbf_free();
	drv_l1_i2s_rx_stop();
	drv_l1_i2s_rx_exit();
}

static INT32U mic_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec)
{
#if APP_LPF_ENABLE == 1
	INT16U *pSrc;
	INT16U i, temp;
#endif	
	
	INT32U ready_buf;
	INT32U frame_size = pAudRec->PCMInFrameSize;
	
	if(pAudRec->ready_buf == 0) {
		return 0;
	}
	
	ready_buf = pAudRec->ready_buf;
	pAudRec->ready_buf = pAudRec->next_buf;
	
	// stop encode
	if(stop_en == 0) {
		pAudRec->buf_idx++;
		if(pAudRec->buf_idx >= C_AUD_PCM_BUFFER_NO) {
			pAudRec->buf_idx = 0;
		}
		
		pAudRec->next_buf = pAudRec->pcm_buf[pAudRec->buf_idx];
		drv_l1_i2s_rx_dbf_set((INT16S *)pAudRec->next_buf, frame_size);
	} else {
		pAudRec->next_buf = 0;
	}	
						
	// invalid cache
	cache_invalid_range(ready_buf, frame_size << 1);
				
	// unsigned to signed
#if APP_LPF_ENABLE == 1	
	pSrc = (INT16U *)ready_buf;
	for(i=0; i<frame_size; i++)
	{
		temp = *pSrc;
		temp = (INT16U)LPF_process(temp);		
		*pSrc++ = temp;
	}
#endif	
	
	return ready_buf;
}

static INT32S line_in_lr_start(AudRecCtrl_t *pAudRec)
{
	INT32S i;
	INT32U size, frame_size;
	
	// allocate memory 
	frame_size = pAudRec->PCMInFrameSize;
	size = (frame_size << 1);
	pAudRec->pcm_buf[0] =(INT32U) gp_malloc_align(size * C_AUD_PCM_BUFFER_NO, 4);
	if(pAudRec->pcm_buf[0] == 0) {
		return STATUS_FAIL;
	}
	
	gp_memset((INT8S *)pAudRec->pcm_buf[0], 0x00, size * C_AUD_PCM_BUFFER_NO);			
	for(i=0; i<C_AUD_PCM_BUFFER_NO; i++) {
		pAudRec->pcm_buf[i] = pAudRec->pcm_buf[0] + size * i;
		DBG("pcm_buf[%d] = 0x%x\r\n", i, pAudRec->pcm_buf[i]);
	}
				
	// set dma 			
	pAudRec->ready_buf = pAudRec->pcm_buf[0];
	pAudRec->next_buf = pAudRec->pcm_buf[1];
	pAudRec->buf_idx = 1;
	
	drv_l1_i2s_rx_dbf_put((INT16S*)pAudRec->ready_buf, frame_size, pAudRec->aud_rec_q);
	drv_l1_i2s_rx_dbf_set((INT16S*)pAudRec->next_buf, frame_size);
				
	// set i2s
	drv_l1_i2s_rx_set_input_path(LINE_IN_LR);
	drv_l1_i2s_rx_init();
	drv_l1_i2s_rx_sample_rate_set(pAudRec->info.sample_rate);
	drv_l1_i2s_rx_start();
	return STATUS_OK;
}

static void line_in_lr_stop(void)
{
	while(drv_l1_i2s_rx_dma_status_get() == 1 || drv_l1_i2s_rx_dbf_status_get() == 1) {
		OSTimeDly(2);
	}
	
	drv_l1_i2s_rx_dbf_free();
	drv_l1_i2s_rx_stop();
	drv_l1_i2s_rx_exit();
}

static INT32U line_in_lr_handle_data(INT32U stop_en, AudRecCtrl_t *pAudRec)
{
#if APP_LPF_ENABLE == 1	
	INT16U *pSrc;
	INT16U i, temp;
#endif	
	
	INT32U ready_buf;
	INT32U frame_size = pAudRec->PCMInFrameSize;
	
	if(pAudRec->ready_buf == 0) {
		return 0;
	}
	
	ready_buf = pAudRec->ready_buf;
	pAudRec->ready_buf = pAudRec->next_buf;
	
	// stop encode
	if(stop_en == 0) {
		pAudRec->buf_idx++;
		if(pAudRec->buf_idx >= C_AUD_PCM_BUFFER_NO) {
			pAudRec->buf_idx = 0;
		}
		
		pAudRec->next_buf = pAudRec->pcm_buf[pAudRec->buf_idx];
		drv_l1_i2s_rx_dbf_set((INT16S *)pAudRec->next_buf, frame_size);
	} else {
		pAudRec->next_buf = 0;
	}	
						
	// invalid cache
	cache_invalid_range(ready_buf, frame_size << 1);
				
#if APP_LPF_ENABLE == 1	
	pSrc = (INT16U *)ready_buf;
	for(i=0; i<frame_size; i++)
	{
		temp = *pSrc;
		temp = (INT16U)LPF_process(temp);		
		*pSrc++ = temp;
	}
#endif	
	
	return ready_buf;
}

#if RECORD_WITH_DAC_OUT_EN == 1
static void dac_output_ramp_down(INT32U channel)
{
	#define RAMPDOWN_STEP		4 
	#define RAMPDOWN_STEP_HOLD	4
	INT16S  last_ldata,last_rdata;
	INT16S  i, temp;
	
	// ramp down
	temp = 0 - RAMPDOWN_STEP;
	last_ldata = R_DAC_CHA_DATA;
	last_rdata = R_DAC_CHB_DATA;
	
	//unsigned to signed 
	last_ldata ^= 0x8000;
	if(channel == 2) {
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
			last_ldata -= RAMPDOWN_STEP;
		}
		else if (last_ldata < 0x0) {
			last_ldata += RAMPDOWN_STEP;
		}
			
		if ((last_ldata < RAMPDOWN_STEP)&&(last_ldata > temp)) { 
			last_ldata = 0;
		}

		if (channel == 2) {
			if (last_rdata > 0x0) {
				last_rdata -= RAMPDOWN_STEP;
		    }
			else if (last_rdata < 0x0) {
				last_rdata += RAMPDOWN_STEP;
		    }
		        
		    if ((last_rdata < RAMPDOWN_STEP)&&(last_rdata > temp)) {  
				last_rdata = 0;
			}
		}
		    
		for(i=0;i<RAMPDOWN_STEP_HOLD;i++) {
			if (channel == 2){
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

static void dac_output_set(INT32U state, AudRecCtrl_t *pAudRec, INT32U pcm_buf, INT32U cwlen)
{
	INT8U err;
	INT32U i, volume;
	INT32U msg_id;
	
	switch(state)
	{
	case 0:
		drv_l1_dac_cha_dbf_put((INT16S *)pcm_buf, cwlen, pAudRec->aud_dac_q);
		break;
		
	case 1:		
		drv_l1_dac_cha_dbf_set((INT16S *)pcm_buf, cwlen);
		
		// mute
		volume = drv_l1_dac_pga_get();
		drv_l1_dac_pga_set(0);	
		if(pAudRec->info.channel_no == 1) {
			drv_l1_dac_mono_set();
		} else {
			drv_l1_dac_stereo_set();
		}
		
		drv_l1_dac_sample_rate_set(pAudRec->info.sample_rate);
		
		// volume up
		for (i=0; i<=volume; i++) {
			drv_l1_dac_pga_set(i);
		}
		break;
		
	case 2:
		msg_id = (INT32U) OSQPend(pAudRec->aud_dac_q, 0, &err);
		if (msg_id == MSG_ADC_DMA_DONE) {
			drv_l1_dac_cha_dbf_set((INT16S *)pcm_buf, cwlen);
		} 
		break;	

	case 3:
		// wait dma stop
		while(drv_l1_dac_dbf_status_get() == 1 || drv_l1_dac_dma_status_get() == 1) {
			OSTimeDly(1);
		}
	
		drv_l1_dac_dbf_free();
		drv_l1_dac_timer_stop();
		dac_output_ramp_down(pAudRec->info.channel_no);		
		break;
		
	default:
		return;	
	}
}
#endif

#if ENERGY_DETECT_EN == 1
static INT32S pcm_energy_detect(INT16S* buffer_addr, INT32U pcm_size)
{
	INT16S temp;
	INT16U avg_value;
	INT64U temp_total;
	INT32S i, cnt, local_cnt;
	
	temp_total = 0;
	cnt = 0;
	for (i = 0; i < pcm_size; i++) {
		temp = abs(*(buffer_addr + i));
		temp_total += (INT64U)temp;
		if (temp > PCM_LOCAL_THR) {
			cnt += 1; 
		}
	}
	
	// average
	avg_value = (INT64U)temp_total / pcm_size;
	DBG_PRINT("temp_total = 0x%x\r\n", temp_total);
	DBG_PRINT("avg_value = 0x%x, cnt = %d\r\n", avg_value, cnt);
	
	//samples above Local_Thr to judge as active
	local_cnt = pcm_size / 6;
	
	if((avg_value > PCM_GLOBAL_THR)||(cnt>local_cnt)) {
		return 0;	//active
	} else {
		return 1; 	//not active
	}
}
#endif

// ring buffer handle api
static INT32S save_buffer_to_storage(AudRecCtrl_t *pAudRec)
{
	INT8U  *addr;
	INT32S size, write_cblen=0;
	
	if(pAudRec->ring_buf == 0) {
		if(pAudRec->ri > C_BS_BUFFER_SIZE/2) {
			pAudRec->ring_buf = 1;
			addr = pAudRec->bitstream_buf;
			size = C_BS_BUFFER_SIZE/2;
			if(pAudRec->info.source_type == C_GP_FS) {
				write_cblen = write(pAudRec->info.fd, (INT32U)addr, size);
			} else {
				if(audenc_user_write) {
					write_cblen = audenc_user_write(pAudRec->info.input_device, 0, (INT32U)addr, size);
				}
			}
				
			if(write_cblen != size) {
				return 	AUD_RECORD_FILE_WRITE_ERR;
			}	
		}
	} else {
		if(pAudRec->ri < C_BS_BUFFER_SIZE/2) {
			pAudRec->ring_buf = 0;
			addr = pAudRec->bitstream_buf + C_BS_BUFFER_SIZE/2;
			size = C_BS_BUFFER_SIZE/2;
			if(pAudRec->info.source_type == C_GP_FS) {
				write_cblen = write(pAudRec->info.fd, (INT32U)addr, size);
			} else {
				if(audenc_user_write) {
					write_cblen = audenc_user_write(pAudRec->info.input_device, 0, (INT32U)addr, size);
				}
			}
				
			if(write_cblen != size) {
				return 	AUD_RECORD_FILE_WRITE_ERR;
			}	
		}
	}
	
	return AUD_RECORD_STATUS_OK;	
}

static INT32S save_final_data_to_storage(AudRecCtrl_t *pAudRec)
{
	INT8U *addr;
	INT32S size, write_cblen=0;
	
	if(pAudRec->ri > C_BS_BUFFER_SIZE/2) {
		addr = pAudRec->bitstream_buf + C_BS_BUFFER_SIZE/2;
		size = pAudRec->ri - C_BS_BUFFER_SIZE/2;
		if(pAudRec->info.source_type == C_GP_FS) {
			write_cblen = write(pAudRec->info.fd, (INT32U)addr, size);
		} else {
			if(audenc_user_write) {
				write_cblen = audenc_user_write(pAudRec->info.input_device, 0, (INT32U)addr, size);
			}
		}
		
		if(write_cblen != size) {
			return 	AUD_RECORD_FILE_WRITE_ERR;
		}
	} else {
		addr = pAudRec->bitstream_buf;
		size = pAudRec->ri;
		if(pAudRec->info.source_type == C_GP_FS) {
			write_cblen = write(pAudRec->info.fd, (INT32U)addr, size);
		} else {
			if(audenc_user_write) {
				write_cblen = audenc_user_write(pAudRec->info.input_device, 0, (INT32U)addr, size);
			}
		}
		
		if(write_cblen != size) {
			return 	AUD_RECORD_FILE_WRITE_ERR;
		}
	}
	
	return AUD_RECORD_STATUS_OK;
}

static void audenc_RIFF_init(AUD_ENC_WAVE_HEADER *pHeader, INT32U samplerate)
{
	pHeader->RIFF_ID[0] ='R';
	pHeader->RIFF_ID[1] ='I';
	pHeader->RIFF_ID[2] ='F';
	pHeader->RIFF_ID[3] ='F';
	pHeader->RIFF_len = 0;
	
	pHeader->type_ID[0] = 'W';
	pHeader->type_ID[1] = 'A';
	pHeader->type_ID[2] = 'V';
	pHeader->type_ID[3] = 'E';
	
	pHeader->fmt_ID[0] = 'f';
	pHeader->fmt_ID[1] = 'm';
	pHeader->fmt_ID[2] = 't';
	pHeader->fmt_ID[3] = ' ';
	
	pHeader->fmt_len = 16;
	pHeader->format = 1;
	pHeader->channel = 1;
	pHeader->sample_rate = samplerate;
	
	//8, 16, 24 or 32
	pHeader->Sign_bit_per_sample = 16;
	//BlockAlign = SignificantBitsPerSample / 8 * NumChannels 
	pHeader->Block_align = 16/8*1;
	//AvgBytesPerSec = SampleRate * BlockAlign 
	pHeader->avg_byte_per_sec = samplerate*2;
	
	pHeader->data_ID[0] = 'd';
	pHeader->data_ID[1] = 'a'; 
	pHeader->data_ID[2] = 't';
	pHeader->data_ID[3] = 'a';
	
	pHeader->data_len = 0;
}

static INT32S wave_encode_start(void *rec_wm)
{
	INT32S cbLen;
	AUD_ENC_WAVE_HEADER *pWaveHeadPara = &WaveHeadPara;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm; 

	audio_rec->work_mem = NULL;
	audio_rec->bitstream_buf = (INT8U *)gp_malloc_align(C_BS_BUFFER_SIZE, 4);
	if(audio_rec->bitstream_buf == 0) {
		gp_free((void *)pWaveHeadPara);
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	//copy header 	
	cbLen = sizeof(AUD_ENC_WAVE_HEADER);
	audenc_RIFF_init(pWaveHeadPara, audio_rec->info.sample_rate);
	gp_memcpy((INT8S*)audio_rec->bitstream_buf, (INT8S*)pWaveHeadPara, cbLen);
	
	audio_rec->ri = cbLen;
	audio_rec->FileLenth = cbLen;
	audio_rec->pack_size = C_WAVE_RECORD_BUFFER_SIZE<<1;
	audio_rec->pack_buf = NULL;
	audio_rec->PCMInFrameSize = C_WAVE_RECORD_BUFFER_SIZE;
	audio_rec->OnePCMFrameSize = C_WAVE_RECORD_BUFFER_SIZE;
	
	gp_free((void *)pWaveHeadPara);
	DBG("work_mem = 0x%x\r\n", audio_rec->work_mem);
	DBG("bitstream_buf = 0x%x\r\n", audio_rec->bitstream_buf);
	DBG("pack_buf = 0x%x\r\n", audio_rec->pack_buf);
	return AUD_RECORD_STATUS_OK;
}

static INT32S wave_encode_stop(void *rec_wm)
{
	INT32S nRet;
	AUD_ENC_WAVE_HEADER *pWaveHeadPara = &WaveHeadPara;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	nRet = save_final_data_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
	
	//write header
	audio_rec->FileLenth -= sizeof(AUD_ENC_WAVE_HEADER);
	pWaveHeadPara->RIFF_len = sizeof(AUD_ENC_WAVE_HEADER) + audio_rec->FileLenth - 8;//file size -8
	pWaveHeadPara->data_len = audio_rec->FileLenth;	
	if(audio_rec->info.source_type == C_GP_FS) {
		lseek(audio_rec->info.fd, 0, SEEK_SET);
		write(audio_rec->info.fd, (INT32U)pWaveHeadPara, sizeof(AUD_ENC_WAVE_HEADER));
		close(audio_rec->info.fd);
		audio_rec->info.fd = -1;
	} else if(audio_rec->info.source_type == C_USER_DEFINE) {	
		if(audenc_user_write) {
			audenc_user_write(audio_rec->info.input_device, 1, (INT32U)pWaveHeadPara, sizeof(AUD_ENC_WAVE_HEADER));
		}
	}
	
	//free memory
	return AUD_RECORD_STATUS_OK;
}

static int wave_encode_once(void *workmem, const short* buffer_addr, int cwlen)
{
	INT8U  *dest_addr;
	INT32S nRet, temp;
	INT32U cblen, size;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)g_audio_rec;
	
	aud_rec_unlock(audio_rec);
	cblen = cwlen<<1;
	temp = audio_rec->ri + cblen;
	if(temp > C_BS_BUFFER_SIZE) {
		size = C_BS_BUFFER_SIZE - audio_rec->ri;
		dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, size);
		
		temp = cblen - size;		//remain size
		dest_addr = audio_rec->bitstream_buf;
		buffer_addr += (size>>1); 	//word address
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, temp);
		audio_rec->ri = temp;
	} else {
		dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
		gp_memcpy((INT8S*)dest_addr, (INT8S*)buffer_addr, cblen);
		audio_rec->ri += cblen;
	}
	
	audio_rec->FileLenth += cblen;
	nRet = save_buffer_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
	
	return cwlen;  
}

// a1800 encode api
#if APP_A1800_ENCODE_EN == 1
static INT32S a1800_encode_start(void *rec_wm)
{
	INT32S nRet;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	audio_rec->work_mem = (INT8U *)gp_malloc(A18_ENC_MEMORY_SIZE);
	if(audio_rec->work_mem == NULL) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	gp_memset((INT8S*)audio_rec->work_mem, 0x00, A18_ENC_MEMORY_SIZE);
	audio_rec->bitstream_buf = (INT8U *)gp_malloc_align(C_BS_BUFFER_SIZE, 4);
	if(!audio_rec->bitstream_buf) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	gp_memset((INT8S*)audio_rec->bitstream_buf, 0, 6);
	audio_rec->ri = 6;
	audio_rec->FileLenth = 6;

	A18_enc_set_BitRate((unsigned char *)audio_rec->work_mem, audio_rec->info.bit_rate);
	nRet = A18_enc_get_BitRate((unsigned char *)audio_rec->work_mem);
	if(nRet != audio_rec->info.bit_rate) {		
		return AUD_RECORD_RUN_ERR;
	}
	
	nRet = A18_enc_init((unsigned char *)audio_rec->work_mem);
	if(nRet != A18_ENC_NO_ERROR) {	
		return AUD_RECORD_RUN_ERR;
	}
	
	audio_rec->pack_size = A18_enc_get_PackageSize((unsigned char *)audio_rec->work_mem);	
	audio_rec->pack_buf = (INT8U *)gp_malloc_align(audio_rec->pack_size, 4);
	if(audio_rec->pack_buf == 0) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
		
	audio_rec->PCMInFrameSize = C_A1800_RECORD_BUFFER_SIZE * A1800_TIMES;
	audio_rec->OnePCMFrameSize = C_A1800_RECORD_BUFFER_SIZE;
	
	DBG("work_mem = 0x%x\r\n", audio_rec->work_mem);
	DBG("bitstream_buf = 0x%x\r\n", audio_rec->bitstream_buf);
	DBG("pack_buf = 0x%x\r\n", audio_rec->pack_buf);	
	return AUD_RECORD_STATUS_OK;
}

static INT32S a1800_encode_stop(void *rec_wm)
{
	INT8U  A1800Header[6];
	INT32S nRet;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	nRet = save_final_data_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
	
	// write header
	audio_rec->FileLenth -= 4;	//header length not include file size(4 byte)
	A1800Header[0] = audio_rec->FileLenth & 0xFF;			//file length 
	A1800Header[1] = (audio_rec->FileLenth >> 8) & 0xFF;	//file length
	A1800Header[2] = (audio_rec->FileLenth >> 16) & 0xFF;	//file length 
	A1800Header[3] = (audio_rec->FileLenth >> 24) & 0xFF;	//file length
	A1800Header[4] =  audio_rec->info.bit_rate & 0xFF;		//bit rate
	A1800Header[5] = (audio_rec->info.bit_rate >> 8) & 0xFF; //bit rate
	
	if(audio_rec->info.source_type == C_GP_FS) {
		lseek(audio_rec->info.fd, 0, SEEK_SET);
		write(audio_rec->info.fd, (INT32U)A1800Header, 6);
		close(audio_rec->info.fd);
		audio_rec->info.fd = -1;
	} else if(audio_rec->info.source_type == C_USER_DEFINE) {
		if(audenc_user_write) {
			audenc_user_write(audio_rec->info.input_device, 1, (INT32U)A1800Header, 4);
		}
	}
	
	return  AUD_RECORD_STATUS_OK;
}

static int a1800_encode_once(void *workmem, const short* buffer_addr, int cwlen)
{
	INT8U  *dest_addr;
	INT32U N;
	INT32S nRet, cblen, temp, size;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)g_audio_rec;
	
	aud_rec_unlock(audio_rec);
	cblen = 0;
	N = cwlen;
	
	while(N >= audio_rec->OnePCMFrameSize)
	{
		nRet = A18_enc_run((unsigned char *)workmem, (short *)buffer_addr, (unsigned char *)audio_rec->pack_buf);
		if(nRet != A18_ENC_NO_ERROR) {	
			return AUD_RECORD_RUN_ERR;
		}
			
		buffer_addr += audio_rec->OnePCMFrameSize;
		cblen += audio_rec->pack_size;
		N -= audio_rec->OnePCMFrameSize;
		
		//copy to bit stream buffer
		temp = audio_rec->ri + audio_rec->pack_size;
		if(temp > C_BS_BUFFER_SIZE) {
			size = C_BS_BUFFER_SIZE - audio_rec->ri;
			dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)audio_rec->pack_buf, size);
		
			temp = audio_rec->pack_size - size;		//remain size
			dest_addr = audio_rec->bitstream_buf;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)(audio_rec->pack_buf + size), temp);
			audio_rec->ri = temp;
		} else {
			dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)audio_rec->pack_buf, audio_rec->pack_size);
			audio_rec->ri += audio_rec->pack_size;
		}
	}
	
	audio_rec->FileLenth += cblen;
	nRet = save_buffer_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
			
	return cwlen;
}
#endif 

// wave encode api
#if APP_WAV_CODEC_EN == 1
static INT32S wave_encode_lib_start(void *rec_wm)
{
	INT32S nRet, size;
	INT32U format;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	size = wav_enc_get_mem_block_size();
	audio_rec->work_mem = (INT8U *)gp_malloc(size);
	if(audio_rec->work_mem == NULL) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	gp_memset((INT8S*)audio_rec->work_mem, 0, size);
	audio_rec->bitstream_buf = (INT8U *)gp_malloc_align(C_BS_BUFFER_SIZE, 4);
	if(audio_rec->bitstream_buf == 0) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	if(audio_rec->info.audio_format == WAV) {
		format = WAVE_FORMAT_PCM;
	} else if(audio_rec->info.audio_format == IMA_ADPCM) {
		format = WAVE_FORMAT_IMA_ADPCM;
	} else if(audio_rec->info.audio_format == MICROSOFT_ADPCM) {
		format = WAVE_FORMAT_ADPCM;
	} else if(audio_rec->info.audio_format == ALAW) {
		format = WAVE_FORMAT_ALAW;
	} else if(audio_rec->info.audio_format == MULAW) {
		format = WAVE_FORMAT_MULAW;
	} else {
		return AUD_RECORD_INIT_ERR;
	}
		
	nRet = wav_enc_Set_Parameter( audio_rec->work_mem, 
								  audio_rec->info.channel_no, 
								  audio_rec->info.sample_rate, 
								  format);
	if(nRet < 0) {	
		return AUD_RECORD_RUN_ERR;
	}
	
	nRet = wav_enc_init(audio_rec->work_mem);
	if(nRet < 0) {	
		return AUD_RECORD_RUN_ERR;
	}
		
	//copy header
	size = wav_enc_get_HeaderLength(audio_rec->work_mem);	
	gp_memset((INT8S*)audio_rec->bitstream_buf, 0, size);
	audio_rec->ri = size;
	audio_rec->FileLenth = size;
	audio_rec->NumSamples = 0;
		
	size = wav_enc_get_SamplePerFrame(audio_rec->work_mem);
	audio_rec->PCMInFrameSize = size * ADPCM_TIMES;
	audio_rec->OnePCMFrameSize = size;

#if 1
	audio_rec->pack_size = size * 2;
#else	
	audio_rec->pack_size = wav_enc_get_BytePerPackage(audio_rec->work_mem);
#endif	
	
	audio_rec->pack_buf = (INT8U *)gp_malloc_align(audio_rec->pack_size, 4);
	if(!audio_rec->pack_buf) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	DBG("work_mem = 0x%x\r\n", audio_rec->work_mem);
	DBG("bitstream_buf = 0x%x\r\n", audio_rec->bitstream_buf);
	DBG("pack_buf = 0x%x\r\n", audio_rec->pack_buf);
	return AUD_RECORD_STATUS_OK;
}

static INT32S wave_encode_lib_stop(void *rec_wm)
{
	INT8U *pHeader;
	INT32S cbLen, nRet;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	nRet = save_final_data_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
	
	//write header
	cbLen = wav_enc_get_HeaderLength(audio_rec->work_mem);
	pHeader = (INT8U *)gp_malloc(cbLen);
	if(pHeader == 0) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	nRet = wav_enc_get_header(audio_rec->work_mem, pHeader, audio_rec->FileLenth, audio_rec->NumSamples);
	if(audio_rec->info.source_type == C_GP_FS) {
		lseek(audio_rec->info.fd, 0, SEEK_SET);
		write(audio_rec->info.fd, (INT32U)pHeader, cbLen);
		close(audio_rec->info.fd);
		audio_rec->info.fd = -1;
	} else if(audio_rec->info.source_type == C_USER_DEFINE) {
		if(audenc_user_write) {
			audenc_user_write(audio_rec->info.input_device, 1, (INT32U)pHeader, cbLen);
		}
	}
	
	//free memory
	gp_free((void *)pHeader);
	return	AUD_RECORD_STATUS_OK;
}

static int wave_encode_lib_once(void *workmem, const short* buffer_addr, int cwlen)
{
	INT8U  *dest_addr;
	INT32U N, PackSize;
	INT32S nRet, cblen, temp, size;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)g_audio_rec;
	
	aud_rec_unlock(audio_rec);
	cblen = 0;
	N = cwlen;
	
	while(N >= audio_rec->OnePCMFrameSize)
	{
		nRet = wav_enc_run(workmem, (short *)buffer_addr, audio_rec->pack_buf);
		if(nRet < 0) {		
			return  AUD_RECORD_RUN_ERR;
		}
		
		PackSize = nRet;
		audio_rec->NumSamples += wav_enc_get_SamplePerFrame(workmem);
		buffer_addr += audio_rec->OnePCMFrameSize;
		cblen += PackSize;
		N -= audio_rec->OnePCMFrameSize;
		
		//copy to bit stream buffer
		temp = audio_rec->ri + PackSize;
		if(temp > C_BS_BUFFER_SIZE) {
			size = C_BS_BUFFER_SIZE - audio_rec->ri;
			dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)audio_rec->pack_buf, size);
		
			temp = PackSize - size;		//remain size
			dest_addr = audio_rec->bitstream_buf;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)(audio_rec->pack_buf + size), temp);
			audio_rec->ri = temp;
		} else {
			dest_addr = audio_rec->bitstream_buf + audio_rec->ri;
			gp_memcpy((INT8S*)dest_addr, (INT8S*)audio_rec->pack_buf, PackSize);
			audio_rec->ri += PackSize;
		}
	}
	
	audio_rec->FileLenth += cblen;
	nRet = save_buffer_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
			
	return cwlen;
}
#endif

// mp3 encode api
#if APP_MP3_ENCODE_EN == 1
/**
 * @sample rate: 48k, 44.1K, 32k, 24K, 22.05k, 16K, 12K, 11.025K, 8k  
 * @bit rate: 	 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 kbps
 * @bit rate:	 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 kbps
 */
static INT32S mp3_encode_start(void *rec_wm)
{
	INT32S nRet, size;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	size = mp3enc_GetWorkMemSize();
	audio_rec->work_mem = (INT8U *)gp_malloc(size);
	if(audio_rec->work_mem == 0) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	gp_memset((INT8S*)audio_rec->work_mem, 0, size);	
	audio_rec->bitstream_buf = (INT8U *)gp_malloc_align(C_BS_BUFFER_SIZE, 4);	//ring buffer 
	if(audio_rec->bitstream_buf == 0) {
		return AUD_RECORD_MEMORY_ALLOC_ERR;
	}
	
	nRet = mp3enc_init(	audio_rec->work_mem, 
						audio_rec->info.channel_no, 
						audio_rec->info.sample_rate,
						(audio_rec->info.bit_rate/1000), 
						0, //copyright
						(CHAR *)audio_rec->bitstream_buf,
						C_BS_BUFFER_SIZE, 
						0); //RingWI, for ID3 header skip. 
	if(nRet<0) {
		return AUD_RECORD_INIT_ERR;
	}
	
	audio_rec->ri = 0;						
	audio_rec->pack_size = 0;
	audio_rec->pack_buf = NULL;
	
	//size = nRet * MP3_TIME * pAudio_Encode_Para->Channel;
	size = nRet * MP3_TIME;
	audio_rec->PCMInFrameSize = size;
	//size = nRet*pAudio_Encode_Para->Channel;
	size = nRet;
	audio_rec->OnePCMFrameSize = size; 
	
	DBG("work_mem = 0x%x\r\n", audio_rec->work_mem);
	DBG("bitstream_buf = 0x%x\r\n", audio_rec->bitstream_buf);
	DBG("pack_buf = 0x%x\r\n", audio_rec->pack_buf);	
	return AUD_RECORD_STATUS_OK;
}

static INT32S mp3_encode_stop(void *rec_wm)
{
	INT32S nRet;
	INT32U old_index;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)rec_wm;
	
	old_index = audio_rec->ri;
	nRet = mp3enc_end((void *)audio_rec->work_mem);
	if(nRet > 0) {
		audio_rec->ri = nRet;
		nRet = audio_rec->ri - old_index;
		if(nRet <0) {
			nRet += C_BS_BUFFER_SIZE;
		}
		
		audio_rec->FileLenth += nRet;	
		nRet = save_buffer_to_storage(audio_rec);
		if(nRet < 0) {
			return AUD_RECORD_FILE_WRITE_ERR;
		}
	}
	
	nRet = save_final_data_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
	
	if(audio_rec->info.source_type == C_GP_FS) {
		close(audio_rec->info.fd);
		audio_rec->info.fd = -1;
	} else {
		if(audenc_user_write) {
			audenc_user_write(audio_rec->info.input_device, 1, NULL, 0);
		}
	}
	
	return  AUD_RECORD_STATUS_OK;
}

static int mp3_encode_once(void *workmem, const short* buffer_addr, int cwlen)
{
	INT32U N, old_index;
	INT32S nRet, cblen;
	AudRecCtrl_t *audio_rec = (AudRecCtrl_t *)g_audio_rec;
	
	aud_rec_unlock(audio_rec);
	cblen = 0;
	old_index = audio_rec->ri;
	N = cwlen;
	
	while(N >= audio_rec->OnePCMFrameSize)
	{
		nRet =  mp3enc_encframe((void *)workmem, (short *)buffer_addr);
		if(nRet<0) {	
			return AUD_RECORD_RUN_ERR;
		}
		
		audio_rec->ri = nRet;
		buffer_addr += audio_rec->OnePCMFrameSize;
		N -= audio_rec->OnePCMFrameSize;
	}
		
	cblen = audio_rec->ri - old_index;
	if(cblen <0) {
		cblen += C_BS_BUFFER_SIZE;
	}
		
	audio_rec->FileLenth += cblen;	
	nRet = save_buffer_to_storage(audio_rec);
	if(nRet < 0) {
		return AUD_RECORD_FILE_WRITE_ERR;
	}
		
	return cwlen;
}
#endif //APP_MP3_ENCODE_EN
