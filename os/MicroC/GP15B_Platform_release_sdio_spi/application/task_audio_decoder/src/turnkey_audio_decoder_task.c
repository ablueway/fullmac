#include <string.h>
#include "turnkey_audio_decoder_task.h"
#include "turnkey_audio_dac_task.h"
#include "turnkey_filesrv_task.h"
#include "wav_dec.h"
#if APP_S880_DECODE_EN == 1
	#include "S880_dec.h"
#endif
#if APP_A1600_DECODE_EN == 1
	#include "A16_dec.h"
#endif
#if APP_A1800_DECODE_EN == 1
	#include "A1800dec.h"
#endif
#if APP_A6400_DECODE_EN == 1
#if GPLIB_MP3_HW_EN == 1
	#include "a6400_dec.h"
#else
	#include "a6400_dec_sw.h"
#endif	
#endif
#if APP_MP3_DECODE_EN == 1
#if GPLIB_MP3_HW_EN == 1
	#include "mp3_dec.h"
#else
	#include "mp3_dec_sw.h"
#endif
#endif
#if APP_WMA_DECODE_EN == 1
	#include "wma_dec.h"
	#include "wmaerror.h"
#endif

#if APP_OGG_DECODE_EN == 1
	#include "oggvorbis_dec.h"
#endif

#if APP_CONST_PITCH_EN == 1
	#include "ConstantPitch_API.h"
#endif
#if APP_ECHO_EN == 1
	#include "echo_api.h"
#endif
#if APP_VOICE_CHANGER_EN == 1
	#include "VoiceChanger.h"
#endif
#if APP_UP_SAMPLE_EN == 1
	#include "upsample_api.h"
#endif

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define AUD_QUEUE_MAX			5
#define AUDIO_FS_Q_SIZE  		1
#define AudTaskStackSize		2048

// audio library parser state
#define AUDIO_PARSER_STOP		0
#define AUDIO_PARSER_START		1

// file read status
#define AUDIO_READ_FAIL			(-2)
#define AUDIO_READ_PEND			(-3)
#define AUDIO_READ_WAIT			(-4)

// mp3 id3 parser
#define ID3_TAG_NONE			0
#define ID3_TAG_V1				1
#define ID3_TAG_V2				2
#define ID3_TAG_V2_FOOTER		3
#define ID3_TAG_FLAG_FOOTER		0x10

// ring buffer configure
#define RING_BUF_SIZE			4096

// ramp down setting
#define RAMP_DOWN_STEP			4
#define RAMP_DOWN_STEP_HOLD		4
#define RAMP_DOWN_STEP_LOW_SR	(4*16)

// file type paraser 
#define AUDIO_PASER_BUFFER_SIZE (2048*10)
#define AUDIO_PARSE_FAIL		(-1)
#define AUDIO_PARSE_SUCCS		0
#define AUDIO_MAX_TYPE			4

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define MAKEFOURCC(ch0, ch1, ch2, ch3)	\
	((INT32U)ch0		|	\
	((INT32U)ch1 << 8)	|	\
	((INT32U)ch2 << 16)	|	\
	((INT32U)ch3 << 24))

#define READSTRING(pData)	\
	(*(pData+0) << 0)	|	\
	(*(pData+1) << 8)	|	\
	(*(pData+2) << 16)	|	\
	(*(pData+3) << 24)

#if 1
#define MSG	DBG_PRINT
#else
#define MSG(...)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
	MSG_AUD_PARSER_START = MSG_AUDIO_BASE,
	MSG_AUD_PARSER_STOP,
	MSG_AUD_PLAY,
	MSG_AUD_PAUSE,
	MSG_AUD_RESUME,
	MSG_AUD_STOP,
	MSG_AUD_SEEK_PLAY,
	MSG_AUD_SET_VOLUME,
	MSG_AUD_DECODE_NEXT_FRAME,
	MSG_AUD_EXIT,
	MSG_AUD_MAX
} MSG_AUD_ENUM;

typedef struct 
{
	AUDIO_TYPE audio_file_real_type;
	INT8S (*parse_audio_file_head)(INT8S *p_audio_file_head);
} AUDIO_FILE_PARSE;

typedef struct 
{	
	INT8S  mpeg_version;
	INT8S  layer;
	INT8S  sample_rate;
	INT8S  RSD;
} MP3_FILE_HEAD;

typedef struct AudPostProc_s 
{
	INT32U  channel_no;
	INT32U  sample_rate;
	void 	*hSrc;				// post process workmem
	INT32S  (*pfnGetOutput)(void*, short*, int); //post process function
#if APP_CONST_PITCH_EN == 1
	void	*hConstPitch;		// constant pitch workmem
	INT8U	ConstPitchEn;		// constant pitch enable
	INT8U	Pitch_idx;			// constant pitch index 0 ~ 7
	INT16U 	cp_reserved;		
#endif
#if APP_ECHO_EN == 1
	void 	*hEcho;				// echo effect workmem
	INT8U	EchoEn;				// echo efffect enable
	INT8U 	weight_idx;			// echo weight, 0 ~ 2
	INT16U	delay_len;			// echo delay 		
#endif
#if APP_VOICE_CHANGER_EN == 1
	void 	*hVC;	 			// voice changer workmem
	INT8U 	VoiceChangerEn;		// voice change enable
	INT8U	Speed;				// voice changer speed, 0 ~ 24 
	INT8U	Pitch;				// voice changer pitch, 0 ~ 24
	INT8U	vc_reserved;	
#endif
#if APP_UP_SAMPLE_EN == 1
	void 	*hUpSample;			// up sample workmem
	INT32U	UpSampleEn; 		// upsample enable
	INT32U 	UpSampleRate;		// upsample target sample rate
#endif
} AudPostProc_t;

typedef struct AudDecCtrl_s
{
	// os
	OS_EVENT *AudioTaskQ;
	OS_EVENT *AudioTaskM;
	OS_EVENT *audio_fsq;
	void *fs_q_buf[AUDIO_FS_Q_SIZE];
	void *audio_q_buf[AUD_QUEUE_MAX];
	INT32U task_stack[AudTaskStackSize];
	
	// audio info
	INT8U	main_ch;		// audio decode main channel
	INT8U	state;			// audio decode state
	INT8U	parser_state;	// parser state
	INT8U   volume;			// volume
		
	// audio infomation
	ParserInfo_t info;
	
	// mp3 info
	INT16U  mp3_VBR_flag;	// 0: CBR, 1:VBR
	INT16U  mp3_ID3V2_len;	// id3 length
	INT32U  mp3_total_frame;// totol frame
	INT32U	dec_pcm_len;	// decode pcm length
	INT32U	seek_time;		// seek time in second
	
	// audio decode
	INT8U	*ring_buf;		// file ring buffer
	INT8S   *work_mem;		// audio decode libary workmem
	INT32U  ring_size;		// ring buffer size
	INT32U  workmem_size;	// workmem size
	INT32U  frame_size;		// pcm frame size
	INT32U  ri;				// ring buffer read index 
	INT32U  wi;				// ring buffer write index
	INT32U  header_len;		// parser header length
	INT32U  offset;			// file_offset
	INT32U  read_secs;		// sector
	INT32U  file_cnt;		// file decode count
	INT16U  retry_cnt;		// parser retry times
	INT8U   f_last;			// final frame decode
	INT8U	task_running;	// task running flag
	
	// audio decode api	
	INT32U	lock;
	INT32S	(*fp_deocde_init)(void *workmem);	// decode init api
	INT32S	(*fp_deocde)(void *workmem);		// decode process api
	INT32S	(*fp_seek_deocde)(void *workmem);	// decode process api
		
	// post process
	AudPostProc_t post_proc;

	// audio out 
	void *audout_wrokmem;		// audio out workmem
} AudDecCtrl_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
// post process setting
static INT32S audio_post_process_set(AudDecCtrl_t *pAudDec); 

#if APP_S880_DECODE_EN == 1
static INT32S audio_s880_play_init(void *ch_wm);
static INT32S audio_s880_process(void *ch_wm);
#endif
#if APP_A1600_DECODE_EN == 1
static INT32S audio_a16_play_init(void *ch_wm);
static INT32S audio_a16_process(void *ch_wm);
#endif
#if APP_A1800_DECODE_EN == 1
static INT32S audio_a1800_play_init(void *ch_wm);
static INT32S audio_a1800_process(void *ch_wm);
#endif
#if APP_A6400_DECODE_EN == 1
static INT32S audio_a64_play_init(void *ch_wm);
static INT32S audio_a64_process(void *ch_wm);
#endif
#if APP_WAV_CODEC_EN == 1
static INT32S audio_wave_play_init(void *ch_wm);
static INT32S audio_wave_process(void *ch_wm);
static INT32S audio_wave_seek_play(void *ch_wm);
#endif
#if APP_MP3_DECODE_EN == 1
static INT8U parser_id3_get_type(INT8U *data, INT32U length);
static void parser_id3_header(INT8U *header, INT32U *version, INT32S *flags, INT32U *size);
static INT32U parser_id3_get_size(INT8U *ptr);
static INT32S parser_id3_get_tag_len(INT8U *data, INT32U length);
static INT32S audio_mp3_play_init(void *ch_wm);
static INT32S audio_mp3_process(void *ch_wm);
static INT32S audio_mp3_seek_play(void *ch_wm);
#endif
#if APP_WMA_DECODE_EN == 1
static INT32S audio_wma_play_init(void *ch_wm);
static INT32S audio_wma_process(void *ch_wm);
static INT32S audio_wma_seek_play(void *ch_wm);
#endif
#if APP_AAC_DECODE_EN == 1
static INT32S audio_aac_play_init(void *ch_wm);
static INT32S audio_aac_process(void *ch_wm);
#endif
#if APP_OGG_DECODE_EN == 1
static INT32S audio_ogg_play_init(void *ch_wm);
static INT32S audio_ogg_process(void *ch_wm);
#endif

static void audio_task_entry(void *p_arg);
static void audio_lock(AudDecCtrl_t *pAudDec);
static void audio_unlock(AudDecCtrl_t *pAudDec);
static INT32S audio_decode_send_msg(AudDecCtrl_t *pAudDec, INT32U message);

static INT32S audio_parser_start(AudDecCtrl_t *pAudDec);
static void audio_parser_stop(AudDecCtrl_t *pAudDec);
static INT32S audio_start(AudDecCtrl_t *pAudDec);
static INT32S audio_pause(AudDecCtrl_t *pAudDec);
static INT32S audio_resume(AudDecCtrl_t *pAudDec);
static INT32S audio_stop(AudDecCtrl_t *pAudDec);
static INT32S audio_seek_play(AudDecCtrl_t *pAudDec);
static INT32S audio_volume_set(AudDecCtrl_t *pAudDec);
static void audio_decode_next_frame(AudDecCtrl_t *pAudDec);
static void audio_send_next_frame_q(OS_EVENT *queue);

static void audio_ramp_down(void *audout_wm, INT32U channel_no, INT32U sample_rate, INT32U frame_size);
static INT32S audio_mem_alloc(AudDecCtrl_t *pAudDec);

static INT8S parse_mp3_file_head(INT8S *p_audio_file_head);
static INT8S parse_wma_file_head(INT8S *p_audio_file_head);
static INT8S parse_wav_file_head(INT8S *p_audio_file_head);
static INT32S audio_real_type_get(INT16S fd, INT8S type_index, OS_EVENT *ack_fsq);
static INT32S audio_get_type(INT16S fd, INT8S* file_name, OS_EVENT *ack_fsq);
static INT32S audio_play_file_set(AudDecCtrl_t *pAudDec, OS_EVENT *ack_fsq);

static INT32S audio_decode_fs_read(INT32S fd, INT32U buf_addr, INT32U buf_size, INT32U data_offset);
static INT32S audio_decode_fill_ring_buf(AudDecCtrl_t *pAudDec);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static OS_EVENT *audio_sema;
static AudDecCtrl_t *g_audio_ctrl;

void (*auddec_end)(INT32U main_ch);
INT32S (*auddec_user_read)(INT32U main_ch, INT32U buf_addr, INT32U buf_size, INT8U *data_start_addr, INT32U data_offset);

/**
 * @brief   open audio decode device
 * @param   main_ch[in]: 0: SPU, 1:DAC 2:I2S
 * @return 	workmem
 */
void *AudDecOpen(INT32U main_ch)
{
	INT8U error;
	INT32U priority;
	AudDecCtrl_t *pAudDec;
#if ((APP_CONST_PITCH_EN == 1)||(APP_ECHO_EN == 1)||(APP_VOICE_CHANGER_EN == 1)||(APP_UP_SAMPLE_EN == 1))
	AudPostProc_t *ppp;
#endif
	if(audio_sema == 0) {
		audio_sema = OSSemCreate(1);
	}
	
	pAudDec = (AudDecCtrl_t *)gp_malloc_align(sizeof(AudDecCtrl_t), 4);
	if(pAudDec == 0) {
		return 0;
	}
	
	MSG("audio_dec_workmem[%d] = 0x%x\r\n", main_ch, pAudDec);
	gp_memset((INT8S *)pAudDec, 0x00, sizeof(AudDecCtrl_t));
	
	// audio decode initialize
	pAudDec->main_ch = main_ch; 
	pAudDec->state = AUDIO_PLAY_STOP;
	pAudDec->parser_state = AUDIO_PARSER_STOP;
#if ((APP_CONST_PITCH_EN == 1)||(APP_ECHO_EN == 1)||(APP_VOICE_CHANGER_EN == 1)||(APP_UP_SAMPLE_EN == 1))
	ppp = &pAudDec->post_proc;
#endif
	// post process initialize
#if APP_CONST_PITCH_EN == 1
	ppp->hConstPitch = ConstantPitch_Create(8192, 22050, 1, 0);
	if(ppp->hConstPitch == 0) {
		goto __fail;
	}
#endif
#if APP_ECHO_EN == 1
	ppp->hEcho = Echo_Create(8192, 48000, 1, 0);
	if(ppp->hEcho == 0) {
		goto __fail;
	}
#endif
#if APP_VOICE_CHANGER_EN == 1
	ppp->hVC = VoiceChanger_Create(8192, 48000, 2, 0);
	if(ppp->hVC == 0) {
		goto __fail;
	}
	
	ppp->Speed = 12;
	ppp->Pitch = 12;
#endif
#if APP_UP_SAMPLE_EN
	ppp->hUpSample = UpSample_Create(8192);
	if(ppp->hUpSample == 0) {
		goto __fail;
	}
	
	ppp->UpSampleEn = 1;
	ppp->UpSampleRate = 48000;
#endif
	
	// audio output initialize
	pAudDec->audout_wrokmem = audio_out_open(main_ch);
	if(pAudDec->audout_wrokmem == 0) {
		goto __fail;
	}
	
	// queue create
	pAudDec->AudioTaskQ = OSQCreate(pAudDec->audio_q_buf, AUD_QUEUE_MAX);
	if(pAudDec->AudioTaskQ == 0) {
		goto __fail;
	}
	
	pAudDec->AudioTaskM = OSMboxCreate(NULL);
	if(pAudDec->AudioTaskM == 0) {
		goto __fail;
	}
	
	pAudDec->audio_fsq = OSQCreate(pAudDec->fs_q_buf, AUDIO_FS_Q_SIZE);
	if(pAudDec->audio_fsq == 0) {
		goto __fail;
	}
	
	switch(main_ch)
	{
	case AUDIO_CHANNEL_SPU:
		priority = AUD_DEC_PRIORITY0;
		break;
	
	case AUDIO_CHANNEL_DAC:
		priority = AUD_DEC_PRIORITY1;
		break;
		
	case AUDIO_CHANNEL_I2S:	
		priority = AUD_DEC_PRIORITY2;
		break;	
		
	default:
		while(1);	
	}
	
	error = OSTaskCreate(audio_task_entry, (void *)pAudDec, &pAudDec->task_stack[AudTaskStackSize - 1], priority); 
	if(error != OS_NO_ERR) {
		goto __fail;
	}
	
	pAudDec->task_running = 1;
	return (void *)pAudDec;
	
__fail:
	AudDecClose((void *)pAudDec);
	return 0;	
}

/**
 * @brief   close audio decode device
 * @param   workmem[in]: work memory
 * @return 	none
 */
void AudDecClose(void *workmem)
{
	INT8U err;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	//task stop
	if(pAudDec->task_running) {
		audio_decode_send_msg(pAudDec, MSG_AUD_STOP);
		audio_decode_send_msg(pAudDec, MSG_AUD_PARSER_STOP);
		audio_decode_send_msg(pAudDec, MSG_AUD_EXIT);
	}
 	
 	if(pAudDec->audio_fsq) {
 		OSQDel(pAudDec->audio_fsq, OS_DEL_ALWAYS, &err);
 	}
 	
 	if(pAudDec->AudioTaskM) {
 		OSMboxDel(pAudDec->AudioTaskM, OS_DEL_ALWAYS, &err);
 	}
 	
 	if(pAudDec->AudioTaskQ) {
 		OSQDel(pAudDec->AudioTaskQ, OS_DEL_ALWAYS, &err);
 	}
 	
 	if(pAudDec->audout_wrokmem) {
 		audio_out_close(pAudDec->audout_wrokmem);
 	}
 	
 	
#if APP_CONST_PITCH_EN	
	if(pAudDec->post_proc.hConstPitch) {
		ConstantPitch_Del(pAudDec->post_proc.hConstPitch);
	}
#endif
#if APP_ECHO_EN == 1
	if(pAudDec->post_proc.hEcho) {
		Echo_Del(pAudDec->post_proc.hEcho);
	}
#endif
#if APP_VOICE_CHANGER_EN == 1
	if(pAudDec->post_proc.hVC) {
		VoiceChanger_Del(pAudDec->post_proc.hVC);
	}
#endif
#if APP_UP_SAMPLE_EN
	if(pAudDec->post_proc.hUpSample) {
		UpSample_Del(pAudDec->post_proc.hUpSample);
	}
#endif

	if(pAudDec) {
		gp_free((void *)pAudDec);
	}
}

INT32S AudDecParserStart(void *workmem, ParserInfo_t *pInfo)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	gp_memcpy((INT8S *)&pAudDec->info, (INT8S *)pInfo, sizeof(ParserInfo_t));
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_PARSER_START);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	gp_memcpy((INT8S *)pInfo, (INT8S *)&pAudDec->info, sizeof(ParserInfo_t));
	return STATUS_OK;
}

INT32S AudDecParserStop(void *workmem)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_PARSER_STOP);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecStart(void *workmem)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_PLAY);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecPause(void *workmem)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_PAUSE);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecResume(void *workmem)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_RESUME);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecStop(void *workmem)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_STOP);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecSeekPlay(void *workmem, INT32U sec)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	if(sec >= pAudDec->info.duration) {
		return STATUS_FAIL;
	}
	
	pAudDec->seek_time = sec;
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_SEEK_PLAY);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecSetVolume(void *workmem, INT32U Volume)
{
	INT32S ret;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	
	pAudDec->volume = Volume;
	ret = audio_decode_send_msg(pAudDec, MSG_AUD_SET_VOLUME);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S AudDecGetCurTime(void *workmem)
{
	INT32U time;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	ParserInfo_t *pInfo = &pAudDec->info;
	
	time = pAudDec->dec_pcm_len / pInfo->sample_rate;
	return time;
}

INT32S AudDecGetTotalTime(void *workmem)
{
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	return pAudDec->info.duration;
}

INT32S AudDecGetStatus(void *workmem)
{
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)workmem;
	return pAudDec->state;
}

static void audio_task_entry(void *p_arg)
{
	INT8U err;
	INT32S ret;
    INT32U msg_id;
	AudDecCtrl_t *pAudDec = (AudDecCtrl_t *)p_arg;
	OS_EVENT *AudioTaskQ = pAudDec->AudioTaskQ;
	OS_EVENT *AudioTaskM = pAudDec->AudioTaskM;

	while (1)
	{
		msg_id = (INT32U)OSQPend(AudioTaskQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}
		
		switch(msg_id) 
		{
		case MSG_AUD_PARSER_START:
			MSG("MSG_AUD_PARSER_START\r\n");
			ret = audio_parser_start(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			break;

		case MSG_AUD_PARSER_STOP:
			MSG("MSG_AUD_PARSER_STOP\r\n");
			audio_parser_stop(pAudDec);
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;

		case MSG_AUD_PLAY:
			MSG("MSG_AUD_PLAY\r\n");
			ret = audio_start(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				audio_send_next_frame_q(pAudDec->AudioTaskQ);
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			break;
			
		case MSG_AUD_PAUSE:
			MSG("MSG_AUD_PAUSE\r\n");
			ret = audio_pause(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			break;
			
		case MSG_AUD_RESUME:
			MSG("MSG_AUD_RESUME\r\n");
			ret = audio_resume(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			break;
		
		case MSG_AUD_STOP:
			MSG("MSG_AUD_STOP\r\n");
			ret = audio_stop(pAudDec);	
			if(ret == AUDIO_ERR_NONE) {
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}			
			break;
		
		case MSG_AUD_SEEK_PLAY:
			MSG("MSG_AUD_SEEK_PLAY\r\n");
			ret = audio_seek_play(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				audio_send_next_frame_q(pAudDec->AudioTaskQ);
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			
		case MSG_AUD_SET_VOLUME:
			MSG("MSG_AUD_SET_VOLUME\r\n");
			ret = audio_volume_set(pAudDec);
			if(ret == AUDIO_ERR_NONE) {
				OSMboxPost(AudioTaskM, (void *)ACK_OK);
			} else {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
			}
			break;
			
		case MSG_AUD_DECODE_NEXT_FRAME:
			audio_decode_next_frame(pAudDec);
			break;
		
		case MSG_AUD_EXIT:
			MSG("MSG_AUD_EXIT\r\n");
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
			
		default:
			break;
		}
	}
	while(1);
}

static void audio_lock(AudDecCtrl_t *pAudDec)
{
	INT8U err;
	
	OSSemPend(audio_sema, 0x00, &err);
	pAudDec->lock = 1;
	g_audio_ctrl = pAudDec;
	
}

static void audio_unlock(AudDecCtrl_t *pAudDec)
{	
	if(pAudDec->lock) {
		g_audio_ctrl = 0;
		pAudDec->lock = 0;
		OSSemPost(audio_sema);
	}
}

static INT32S audio_decode_send_msg(AudDecCtrl_t *pAudDec, INT32U message)
{
	INT8U err;
	INT32S ack_msg;
	
	// clear ack mbox
	OSMboxAccept(pAudDec->AudioTaskM);
	
	// post message
	err = OSQPost(pAudDec->AudioTaskQ, (void *)message);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	// wait ack mbox
	ack_msg = (INT32S)OSMboxPend(pAudDec->AudioTaskM, 5000, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return ack_msg;
}

static INT32S audio_parser_start(AudDecCtrl_t *pAudDec)
{
	INT32S ret;
	ParserInfo_t *pInfo = &pAudDec->info;
	
	if(pAudDec->parser_state == AUDIO_PARSER_START) {
		return AUDIO_ERR_NONE;
	}
	
	// clear queue
	OSQFlush(pAudDec->audio_fsq);
	
	if(pInfo->source_type == AUDIO_SRC_TYPE_FS) { 
		ret = audio_play_file_set(pAudDec, pAudDec->audio_fsq);
		if (ret != AUDIO_ERR_NONE) {
			return (0 - AUDIO_ERR_INVALID_FORMAT);
		}
	}
	
	ret = audio_mem_alloc(pAudDec);
	if (ret != AUDIO_ERR_NONE) {
		MSG("audio memory allocate fail\r\n");
		return ( 0 - AUDIO_ERR_MEM_ALLOC_FAIL);
	}
	
	ret = pAudDec->fp_deocde_init(pAudDec);
	if(ret != AUDIO_ERR_NONE) {
		audio_parser_stop(pAudDec);
		MSG("audio play init failed\r\n");
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
			
	pAudDec->parser_state = AUDIO_PARSER_START;
	return AUDIO_ERR_NONE;
}

static void audio_parser_stop(AudDecCtrl_t *pAudDec)
{
	if(pAudDec->parser_state == AUDIO_PARSER_STOP) {
		return;
	}
	
	// free memory
	if(pAudDec->ring_buf) {
		gp_free(pAudDec->ring_buf);
		pAudDec->ring_buf = 0;
	}
	
	if(pAudDec->work_mem) {
		gp_free(pAudDec->work_mem);
		pAudDec->work_mem = 0;
	}
	
	pAudDec->parser_state = AUDIO_PARSER_STOP;
}

static INT32S audio_start(AudDecCtrl_t *pAudDec)
{
	INT32S ret, i;
	ParserInfo_t *pInfo = &pAudDec->info;
	
	if ((pAudDec->parser_state == AUDIO_PARSER_STOP) || (pAudDec->state == AUDIO_PLAYING)) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	// audio decode start
	ret = audio_out_init(pAudDec->audout_wrokmem, pAudDec->frame_size);
	if(ret < 0) {
    	return (0 - AUDIO_ERR_FAILED);	
	} 
        
    // pre-decode two frame then start play 
    if(pAudDec->fp_deocde == 0) {
    	return (0 - AUDIO_ERR_FAILED);	
    }
    
    for(i=0; i<2; i++) {
    	ret = pAudDec->fp_deocde(pAudDec);
    	if(ret != AUDIO_ERR_NONE) {
    		return (0 - AUDIO_ERR_DEC_FAIL);
    	}
    }
        	
    ret = audio_out_start(pAudDec->audout_wrokmem, pInfo->channel_no, pInfo->sample_rate);
    if(ret < 0) {
    	return AUDIO_ERR_FAILED;
    } 
    
    pAudDec->state = AUDIO_PLAYING;
	return AUDIO_ERR_NONE;
}

static INT32S audio_pause(AudDecCtrl_t *pAudDec)
{
	if (pAudDec->state != AUDIO_PLAYING) {
		return AUDIO_ERR_NONE;
	}

	audio_out_pause(pAudDec->audout_wrokmem);
	pAudDec->state = AUDIO_PLAY_PAUSE;
	return AUDIO_ERR_NONE;
}

static INT32S audio_resume(AudDecCtrl_t *pAudDec)
{
	if (pAudDec->state != AUDIO_PLAY_PAUSE) {
		return AUDIO_ERR_NONE;
	}
	
	audio_out_resume(pAudDec->audout_wrokmem);
	pAudDec->state = AUDIO_PLAYING;
	audio_send_next_frame_q(pAudDec->AudioTaskQ);
	return AUDIO_ERR_NONE;
}

static INT32S audio_stop(AudDecCtrl_t *pAudDec)
{
	ParserInfo_t *pInfo = &pAudDec->info;
	
	if (pAudDec->state == AUDIO_PLAY_STOP) {
		return AUDIO_ERR_NONE;
	}
	
	if(pAudDec->state == AUDIO_PLAY_PAUSE) {
		audio_out_volume(pAudDec->audout_wrokmem, 0x00); 	// mute
		audio_out_resume(pAudDec->audout_wrokmem);			// resume
	}
	
	audio_ramp_down(pAudDec->audout_wrokmem, pInfo->channel_no, pInfo->sample_rate, pAudDec->frame_size);
	audio_out_stop(pAudDec->audout_wrokmem);
	pAudDec->state = AUDIO_PLAY_STOP;
	return AUDIO_ERR_NONE;
}

static INT32S audio_seek_play(AudDecCtrl_t *pAudDec)
{
	INT32S ret, i;
	ParserInfo_t *pInfo = &pAudDec->info;
	
	if (pAudDec->parser_state == AUDIO_PARSER_STOP) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	if(pAudDec->fp_seek_deocde == 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	if (pAudDec->state == AUDIO_PLAYING) {
		audio_stop(pAudDec);
	}

	// seek time
	ret = pAudDec->fp_seek_deocde(pAudDec);
	if(ret < 0) {
		return (0 - AUDIO_ERR_FAILED);	
	}

	// audio decode start
	ret = audio_out_init(pAudDec->audout_wrokmem, pAudDec->frame_size);
	if(ret < 0) {
		return (0 - AUDIO_ERR_FAILED);	
	} 
	
	// set current time
	pAudDec->dec_pcm_len = pInfo->sample_rate * pAudDec->seek_time;
	
	// pre-decode two frame then start play 
	for(i=0; i<2; i++) {
		ret = pAudDec->fp_deocde(pAudDec);   	
		if(ret != AUDIO_ERR_NONE) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
	}
	
	ret = audio_out_start(pAudDec->audout_wrokmem, pInfo->channel_no, pInfo->sample_rate);
	if(ret < 0) {
		return AUDIO_ERR_FAILED;
	}
	
	pAudDec->state = AUDIO_PLAYING;
	return AUDIO_ERR_NONE;
}


static INT32S audio_volume_set(AudDecCtrl_t *pAudDec)
{
	audio_out_volume(pAudDec->audout_wrokmem, pAudDec->volume);	
	return AUDIO_ERR_NONE;
}

static void audio_decode_next_frame(AudDecCtrl_t *pAudDec)
{
	INT32S ret;
	
	if (pAudDec->state != AUDIO_PLAYING) {
		return;
	}
	
	ret = pAudDec->fp_deocde(pAudDec);
	if (ret != AUDIO_ERR_NONE) {
		MSG("AudioDecodeDone.\r\n");
		audio_stop(pAudDec);
		audio_parser_stop(pAudDec);
		if (auddec_end) {
			auddec_end(pAudDec->main_ch);
		}
	} else {
		audio_send_next_frame_q(pAudDec->AudioTaskQ);
	}
}

static void audio_send_next_frame_q(OS_EVENT *queue)
{
	OSQPost(queue, (void *)MSG_AUD_DECODE_NEXT_FRAME);
}

static void audio_ramp_down(void *audout_wm, INT32U channel_no, INT32U sample_rate, INT32U frame_size)
{
	INT8U   ramp_down_step;
	INT16S  *ptr, *pcm_buf;
	INT16S  last_ldata, last_rdata;
	INT32U  pcm_cwlen;
	INT32U  i, j, buf_len = frame_size >> 1;
	
	if(sample_rate > 16000) {
		ramp_down_step = RAMP_DOWN_STEP;
	} else {
		ramp_down_step = RAMP_DOWN_STEP_LOW_SR;
	}
	
	// get the last r and l data
	pcm_buf = audio_out_prepare_ramp_down(audout_wm, (INT32U *)&pcm_cwlen);
	last_ldata = *(pcm_buf + pcm_cwlen - 2);
	last_rdata = *(pcm_buf + pcm_cwlen - 1);
	last_ldata = last_ldata & ~(ramp_down_step-1);
	if (channel_no == 2) {
		last_rdata = last_rdata & ~(ramp_down_step-1);
	} else {
		last_rdata = 0;
	}
	
	MSG("ldata = 0x%x\r\n",last_ldata);
	MSG("rdata = 0x%x\r\n",last_rdata);

	while(1) {
		pcm_buf = audio_out_get_empty_buf(audout_wm);
		ptr = pcm_buf;
		
		for (i=0; i<(buf_len/RAMP_DOWN_STEP_HOLD); i++) {
			for (j=0; j<RAMP_DOWN_STEP_HOLD; j++) {
				*ptr++ = last_ldata;
				if (channel_no == 2) {
					*ptr++ = last_rdata;
				}
			}

			if (last_ldata > 0x0) {
				last_ldata -= ramp_down_step;
			}
			else if (last_ldata < 0x0) {
				last_ldata += ramp_down_step;
			}

			if (channel_no == 2) {
				if (last_rdata > 0x0) {
					last_rdata -= ramp_down_step;
		        }
				else if (last_rdata < 0x0) {
					last_rdata += ramp_down_step;
		        }
		    }
	    }
	    
	    audio_out_send_ready_buf(audout_wm, pcm_buf, buf_len * channel_no);		
		if ((last_ldata == 0x0) && (last_rdata == 0x0)) {
			break;
		}
	}
}

static INT32S audio_mem_alloc(AudDecCtrl_t *pAudDec)
{
	ParserInfo_t *pInfo = &pAudDec->info;
	
	switch(pInfo->audio_format) 
	{
	#if APP_S880_DECODE_EN == 1
		case AUDIO_TYPE_S880:
			pAudDec->fp_deocde_init = audio_s880_play_init;
			pAudDec->fp_deocde = audio_s880_process;
			pAudDec->ring_size = S880_DEC_BITSTREAM_BUFFER_SIZE;				
			pAudDec->workmem_size = S880_DEC_MEMORY_SIZE;
			pAudDec->frame_size = S880_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_A1600_DECODE_EN == 1
		case AUDIO_TYPE_A1600:	
			pAudDec->fp_deocde_init = audio_a16_play_init;
			pAudDec->fp_deocde = audio_a16_process;
			pAudDec->ring_size = A16_DEC_BITSTREAM_BUFFER_SIZE;			
			pAudDec->workmem_size = A16_DEC_MEMORY_SIZE;
			pAudDec->frame_size = A16_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_A1800_DECODE_EN == 1
		case AUDIO_TYPE_A1800:
			pAudDec->fp_deocde_init = audio_a1800_play_init;
			pAudDec->fp_deocde = audio_a1800_process;
			pAudDec->ring_size = A18_DEC_BITSTREAM_BUFFER_SIZE;
			pAudDec->workmem_size = A1800DEC_MEMORY_BLOCK_SIZE;
			pAudDec->frame_size = A18_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_A6400_DECODE_EN == 1
		case AUDIO_TYPE_A6400:
			pAudDec->fp_deocde_init = audio_a64_play_init;
			pAudDec->fp_deocde = audio_a64_process;
		#if GPLIB_MP3_HW_EN == 1	
			pAudDec->ring_size = A6400_DEC_BITSTREAM_BUFFER_SIZE;
			pAudDec->workmem_size = A6400_DEC_MEMORY_SIZE;
		#else
			pAudDec->ring_size = RING_BUF_SIZE;
			pAudDec->workmem_size = A6400_DEC_MEMORY_SIZE + A6400_DECODE_RAM;
		#endif	
			pAudDec->frame_size = A6400_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_WAV_CODEC_EN == 1
		case AUDIO_TYPE_WAV:
			pAudDec->fp_deocde_init = audio_wave_play_init;
			pAudDec->fp_deocde = audio_wave_process;
			pAudDec->fp_seek_deocde = audio_wave_seek_play;
			pAudDec->ring_size = WAV_DEC_BITSTREAM_BUFFER_SIZE;
			pAudDec->workmem_size = WAV_DEC_MEMORY_SIZE;
			pAudDec->frame_size = WAV_DEC_FRAMESIZE*2;
			break;
	#endif		
	#if APP_MP3_DECODE_EN == 1
		case AUDIO_TYPE_MP3:
			pAudDec->fp_deocde_init = audio_mp3_play_init;
			pAudDec->fp_deocde = audio_mp3_process;
			pAudDec->fp_seek_deocde = audio_mp3_seek_play;
			pAudDec->ring_size = RING_BUF_SIZE;
		#if GPLIB_MP3_HW_EN == 1
			pAudDec->workmem_size = MP3_DEC_MEMORY_SIZE;
		#else
			pAudDec->workmem_size = MP3_DEC_MEMORY_SIZE + MP3_DECODE_RAM;
		#endif
			pAudDec->frame_size = MP3_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_WMA_DECODE_EN == 1
		case AUDIO_TYPE_WMA:
			pAudDec->fp_deocde_init = audio_wma_play_init;
			pAudDec->fp_deocde = audio_wma_process;
			pAudDec->fp_seek_deocde = audio_wma_seek_play;
			pAudDec->ring_size = RING_BUF_SIZE;
			pAudDec->workmem_size = WMA_DEC_MEMORY_SIZE;
			pAudDec->frame_size = WMA_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_AAC_DECODE_EN == 1
		case AUDIO_TYPE_AAC:
			pAudDec->fp_deocde_init = audio_aac_play_init;
			pAudDec->fp_deocde = audio_aac_process;
			pAudDec->ring_size = RING_BUF_SIZE;
			pAudDec->workmem_size = AAC_DEC_MEMORY_BLOCK_SIZE;
			pAudDec->frame_size = AAC_DEC_FRAMESIZE;
			break;
	#endif
	#if APP_OGG_DECODE_EN == 1
		case AUDIO_TYPE_OGG:
			pAudDec->fp_deocde_init = audio_ogg_play_init;
			pAudDec->fp_deocde = audio_ogg_process;
			pAudDec->ring_size = OGGVORBIS_DEC_BITSTREAM_BUFFER_SIZE;	
			pAudDec->workmem_size = OGGVORBIS_DEC_MEMORY_SIZE;
			pAudDec->frame_size = OGGVORBIS_DEC_FRAMESIZE;
			break;
	#endif
		default:
			return (0 - AUDIO_ERR_FAILED);
	}

	// memory allocate 
	pAudDec->ring_buf = (INT8U *)gp_malloc_align(pAudDec->ring_size, 4);
	if(pAudDec->ring_buf == NULL) {
		return (0 - AUDIO_ERR_FAILED);
	}

	pAudDec->work_mem = (INT8S*) gp_malloc_align(pAudDec->workmem_size, 4);
	if(pAudDec->work_mem == NULL) {
		gp_free((void *)pAudDec->ring_buf);
		pAudDec->ring_buf = 0;
		return (0 - AUDIO_ERR_FAILED);
	}
	
	gp_memset((INT8S *)pAudDec->ring_buf, 0x00, pAudDec->ring_size);
	gp_memset((INT8S *)pAudDec->work_mem, 0x00,pAudDec->workmem_size);
	MSG("ring_buf = 0x%x (%d)\r\n", pAudDec->ring_buf, pAudDec->ring_size);
	MSG("work_mem = 0x%x (%d)\r\n", pAudDec->work_mem, pAudDec->workmem_size);
	return AUDIO_ERR_NONE;
}

static INT8S parse_mp3_file_head(INT8S *p_audio_file_head)
{
	INT8S mpeg_version, layer,sample_rate;
	INT8S j =0; 
	//INT32U ID3V2_length = 0;
	INT32U cnt  = AUDIO_PASER_BUFFER_SIZE;
	MP3_FILE_HEAD  mp3_file_head[2];
	INT8U *pData;

	pData = (INT8U *)p_audio_file_head;
	
	if(*(pData) == (INT8S)'I' && *(pData + 1) == (INT8S)'D' && *(pData + 2) == (INT8S)'3' )
	{
		//ID3V2_length = 10 + ((*(pData + 9)& 0x7f)|((*(pData + 8) & 0x7f)<<7)|((*(pData + 7) & 0x7f)<<14)|((*(pData + 6) & 0x7f)<<21));
		return AUDIO_PARSE_SUCCS ;
	}

	while (cnt > 4)  
	{
		if((*(pData) == 0xFF ) && ((*(pData + 1)&0xE0) == 0xE0 )  && ((*(pData + 2)&0xF0 )!= 0xF0 ))  // first 11 bits should be 1
		{
			mpeg_version =( *(pData + 1)&0x18)>>3;
			layer = (*(pData + 1)&0x06)>>1;
			sample_rate = (*(pData + 2)&0x0c)>>2;

			if((mpeg_version != 0x01) && (layer != 0x00) && (layer != 0x03) && (sample_rate != 0x03))   // != RESERVERD 
			{
				if(j<2)
				{
					mp3_file_head[j].mpeg_version = mpeg_version;
					mp3_file_head[j].layer = layer;
					mp3_file_head[j].sample_rate = sample_rate;

					j++;
				}
				else if ((mp3_file_head[0].mpeg_version == mp3_file_head[1].mpeg_version) && (mp3_file_head[0].layer == mp3_file_head[1].layer) && (mp3_file_head[0].sample_rate == mp3_file_head[1].sample_rate))
				{		
					MSG("audio file parse succes \r\n");
					return  AUDIO_PARSE_SUCCS ;		
				}
			}
			else
			{
				pData += 4;
				cnt -= 4 ;
			}
		}
		else
		{
			pData += 4;
			cnt -= 4 ;
		}

	}

	return AUDIO_PARSE_FAIL;
}

static INT8S parse_wma_file_head(INT8S *p_audio_file_head)
{
	INT32U *pData = (INT32U *)p_audio_file_head;
	
	if (*pData++ != 0x75B22630) {
		return AUDIO_PARSE_FAIL;
	} else if (*pData++ != 0x11CF668E) {
		return AUDIO_PARSE_FAIL;
	} else if (*pData++ != 0xAA00D9A6) {
		return AUDIO_PARSE_FAIL;
	} else if(*pData++ != 0x6CCE6200) {
		return AUDIO_PARSE_FAIL;
	}
	
	return AUDIO_PARSE_SUCCS;
}

static INT8S parse_wav_file_head(INT8S *p_audio_file_head)
{
	INT16U wave_format;
	INT32U temp;
	
	// Chunk ID
	temp = READSTRING(p_audio_file_head);
	if (temp != MAKEFOURCC('R', 'I', 'F', 'F')) {
		return AUDIO_PARSE_FAIL;
	}
	
	// RIFF Type 
	p_audio_file_head += 8;
	temp = READSTRING(p_audio_file_head);
	if (temp != MAKEFOURCC('W', 'A', 'V', 'E')) {
		return AUDIO_PARSE_FAIL;
	}
	
	p_audio_file_head += 4;
	temp = READSTRING(p_audio_file_head);
	
	// Broadcast Audio Extension Chunk		
	if (temp == MAKEFOURCC('b', 'e', 'x', 't')) {
		INT32S size;
		 
		p_audio_file_head += 4;		//chunk ID
		size = *p_audio_file_head | (*(p_audio_file_head + 1) << 8) | 
				(*(p_audio_file_head + 2) << 16) | (*(p_audio_file_head + 3) << 24);
		
		p_audio_file_head += 4;		//size of extension chunk	
		p_audio_file_head += size;	
		temp = READSTRING(p_audio_file_head);	
	}
	
	// WAVE Format Chunk
	if (temp == MAKEFOURCC('f', 'm', 't', ' ')) {
		p_audio_file_head += 4;		//chunk ID
		p_audio_file_head += 4;		//chunk data size
		wave_format = *p_audio_file_head | (*(p_audio_file_head+1) << 8); //compression code
		switch(wave_format) {
			case WAVE_FORMAT_PCM:
			case WAVE_FORMAT_ADPCM:
			case WAVE_FORMAT_ALAW:
			case WAVE_FORMAT_MULAW:
			case WAVE_FORMAT_IMA_ADPCM:
				return AUDIO_PARSE_SUCCS;
				
			default:
				return AUDIO_PARSE_FAIL;	
		}
	}
	
	return AUDIO_PARSE_FAIL;
}

static INT32S audio_real_type_get(INT16S fd, INT8S type_index, OS_EVENT *ack_fsq)
{
	INT32U data_buf;
	INT32S i, ret;
	AUDIO_FILE_PARSE audio_file_parse_head[3];
	
	switch (type_index)
	{
		case AUDIO_TYPE_MP3:
			audio_file_parse_head[0].audio_file_real_type = AUDIO_TYPE_MP3;
			audio_file_parse_head[0].parse_audio_file_head = parse_mp3_file_head;
			
			audio_file_parse_head[1].audio_file_real_type = AUDIO_TYPE_WMA;
			audio_file_parse_head[1].parse_audio_file_head = parse_wma_file_head;
			
			audio_file_parse_head[2].audio_file_real_type = AUDIO_TYPE_WAV;
			audio_file_parse_head[2].parse_audio_file_head = parse_wav_file_head;
			break;
		
		case AUDIO_TYPE_WMA:
			audio_file_parse_head[0].audio_file_real_type = AUDIO_TYPE_WMA;
			audio_file_parse_head[0].parse_audio_file_head = parse_wma_file_head;
					
			audio_file_parse_head[1].audio_file_real_type = AUDIO_TYPE_WAV;
			audio_file_parse_head[1].parse_audio_file_head = parse_wav_file_head;
			
			audio_file_parse_head[2].audio_file_real_type = AUDIO_TYPE_MP3;
			audio_file_parse_head[2].parse_audio_file_head = parse_mp3_file_head;
			break;
	
		case AUDIO_TYPE_WAV:
			audio_file_parse_head[0].audio_file_real_type = AUDIO_TYPE_WAV;
			audio_file_parse_head[0].parse_audio_file_head = parse_wav_file_head;
			
			audio_file_parse_head[1].audio_file_real_type = AUDIO_TYPE_WMA;
			audio_file_parse_head[1].parse_audio_file_head = parse_wma_file_head;
			
			audio_file_parse_head[2].audio_file_real_type = AUDIO_TYPE_MP3;
			audio_file_parse_head[2].parse_audio_file_head = parse_mp3_file_head;	
			break;
			
		default:
			return type_index;
	}

	// check file only for source type is file system.
	data_buf = (INT32U) gp_malloc_align(AUDIO_PASER_BUFFER_SIZE, 4);
	if (data_buf == NULL) {
		MSG("audio file parse failed to allocate memory\r\n");
		return type_index;
	}
	
	audio_decode_fs_read(fd, data_buf, AUDIO_PASER_BUFFER_SIZE, 0x00);
	audio_decode_fs_read(fd, 0, 0, 0);

	// check it is real the type_index,according the file extension to judge the file real type.
	ret = audio_file_parse_head[0].parse_audio_file_head((INT8S *)data_buf);
	if (ret == AUDIO_PARSE_SUCCS) {	
		gp_free((void *) data_buf);
		return audio_file_parse_head[0].audio_file_real_type;
	}

	// check other support file type except current extension type
	for (i=1; i<sizeof(audio_file_parse_head)/sizeof(AUDIO_FILE_PARSE); i++) {
		ret = audio_file_parse_head[i].parse_audio_file_head((INT8S *)data_buf); 
		if (ret == AUDIO_PARSE_SUCCS) {
			gp_free((void *) data_buf);
			return audio_file_parse_head[i].audio_file_real_type;
		}
	}

	gp_free((void *) data_buf);
	return AUDIO_TYPE_NONE;
}

static INT32S audio_get_type(INT16S fd, INT8S* file_name, OS_EVENT *ack_fsq)
{
   	INT8S temp[5] = {0};
	INT16U i;
	
	gp_strcpy(temp,file_name);
	for(i=0; i<gp_strlen(temp); i++) {
		temp[i] = gp_toupper(temp[i]);
	}

#if APP_S880_DECODE_EN == 1
    if(gp_strcmp(temp, (INT8S *)"S88")==0) {
    	return AUDIO_TYPE_S880;
    }
#endif

#if APP_A1600_DECODE_EN == 1
    if(gp_strcmp(temp, (INT8S *)"A16")==0) {
    	return AUDIO_TYPE_A1600;
    }
#endif

#if APP_A1800_DECODE_EN == 1
    if(gp_strcmp(temp, (INT8S *)"A18")==0) {
    	return AUDIO_TYPE_A1800;
    }
#endif

#if APP_A6400_DECODE_EN == 1
    if(gp_strcmp(temp, (INT8S *)"A64")==0) {
    	return AUDIO_TYPE_A6400;
    }
#endif

#if APP_WAV_CODEC_EN == 1 
   	if(gp_strcmp(temp, (INT8S *)"WAV")==0) {
   	#if 0
		return AUDIO_TYPE_WAV;
	#else
		return audio_real_type_get(fd, AUDIO_TYPE_WAV, ack_fsq);
   	#endif
   	}
#endif

#if APP_MP3_DECODE_EN == 1
   	if(gp_strcmp(temp, (INT8S *)"MP3")==0) {
   	#if 0
		return AUDIO_TYPE_MP3;
	#else	
		return audio_real_type_get(fd, AUDIO_TYPE_MP3, ack_fsq);
	#endif
	}
#endif

#if APP_WMA_DECODE_EN == 1
	if(gp_strcmp(temp, (INT8S *)"WMA")==0) {
	#if 0
		return AUDIO_TYPE_WMA;
	#else
		return audio_real_type_get(fd,AUDIO_TYPE_WMA, ack_fsq);
	#endif
	}
#endif

#if APP_AAC_DECODE_EN == 1
	if(gp_strcmp(temp, (INT8S *)"AAC")==0) {
		return AUDIO_TYPE_AAC;
	}
#endif

#if APP_OGG_DECODE_EN == 1
   	if(gp_strcmp(temp, (INT8S *)"OGG")==0) {
		return AUDIO_TYPE_OGG;
	}
#endif

    if(gp_strcmp(temp, (INT8S *)"IDI")==0) {
    	return AUDIO_TYPE_MIDI;
    }
    	
    if(gp_strcmp(temp, (INT8S *)"GMD")==0) {
    	return AUDIO_TYPE_MIDI;
    }

   	return AUDIO_TYPE_NONE;
}

static INT32S audio_play_file_set(AudDecCtrl_t *pAudDec, OS_EVENT *ack_fsq)
{
	ParserInfo_t *pInfo = &pAudDec->info;	
	struct sfn_info aud_sfn_file;
	
	sfn_stat(pInfo->fd, &aud_sfn_file);
	pInfo->audio_format = audio_get_type(pAudDec->info.fd, aud_sfn_file.f_extname, ack_fsq);
   	if (pInfo->audio_format == AUDIO_TYPE_NONE) {
	   MSG("find not support audio.\r\n");
   	   return (0 - AUDIO_ERR_INVALID_FORMAT);
   	}
	
	pInfo->file_len = aud_sfn_file.f_size;
	return AUDIO_ERR_NONE;
}

static INT32S audio_decode_fs_read(INT32S fd, INT32U buf_addr, INT32U buf_size, INT32U data_offset)
{
	if(fd < 0) {
		return AUDIO_READ_FAIL;
	}

	if(lseek(fd, data_offset, SEEK_SET) < 0) {
		return AUDIO_READ_FAIL;
	}
	
	if(buf_addr == 0 || buf_size == 0) {
		return 0;
	}
	
	return read(fd, buf_addr, buf_size);
}

static INT32S audio_decode_fill_ring_buf(AudDecCtrl_t *pAudDec)
{
	INT32S ret, size;
	INT32U buf_addr, buf_size;
	ParserInfo_t *pInfo = &pAudDec->info;
	
	if((pAudDec->wi == 0) && (pAudDec->ri == 0)) {
		buf_addr = (INT32U)pAudDec->ring_buf;
		buf_size = pAudDec->ring_size/2;
	} else {
		// get remain data
		size = pAudDec->wi - pAudDec->ri;
		if(size < 0) {
			size += pAudDec->ring_size;
		}
		
		if(size < pAudDec->ring_size/2) {
			buf_addr = (INT32U)pAudDec->ring_buf + pAudDec->wi;
			buf_size = pAudDec->ring_size/2;
		} else {
			return 0;
		}
	}

	// read method
	if(pInfo->source_type == AUDIO_SRC_TYPE_FS || pInfo->source_type == AUDIO_SRC_TYPE_FS_RESOURCE_IN_FILE) {
		ret = audio_decode_fs_read(pInfo->fd, buf_addr, buf_size, pInfo->data_offset + pAudDec->offset);
	} else if(pInfo->source_type == AUDIO_SRC_TYPE_USER_DEFINE) {
		if(auddec_user_read) {
			ret = auddec_user_read(pAudDec->main_ch, buf_addr, buf_size, pInfo->data_start_addr, pInfo->data_offset + pAudDec->offset);
		} else {
			ret = 0;
		}
	} else {
		INT8U err;
		INT32U msg_id;
		TK_FILE_SERVICE_STRUCT audio_fs_para;
		
		switch(pInfo->source_type) 
		{				
			case AUDIO_SRC_TYPE_GPRS:
				msg_id = MSG_FILESRV_NVRAM_AUDIO_GPRS_READ;
				break;
				
			case AUDIO_SRC_TYPE_APP_RS:
				msg_id = MSG_FILESRV_NVRAM_AUDIO_APP_READ;
				break;
				
			case AUDIO_SRC_TYPE_APP_PACKED_RS:
				msg_id = MSG_FILESRV_NVRAM_AUDIO_APP_PACKED_READ;
				break;

			default:
				return AUDIO_READ_FAIL;
		}
		
		audio_fs_para.fd = pInfo->fd;
		audio_fs_para.buf_addr = buf_addr;
		audio_fs_para.buf_size = buf_size;
		audio_fs_para.spi_para.sec_offset = pAudDec->ring_size/1024;
		audio_fs_para.result_queue = pAudDec->audio_fsq;
		audio_fs_para.data_offset = pInfo->data_offset + pAudDec->offset;
		audio_fs_para.main_channel = pAudDec->main_ch;
		audio_fs_para.data_start_addr = pInfo->data_start_addr;
        msgQSend(fs_msg_q_id, msg_id, (void *)&audio_fs_para, sizeof(TK_FILE_SERVICE_STRUCT), MSG_PRI_URGENT);
        
        // ack
        ret = (INT32S) OSQPend(pAudDec->audio_fsq, 10000, &err);
        if (err != OS_NO_ERR) {
    	    return AUDIO_READ_FAIL;
        }
	}
	
	if(ret < 0) {
		return AUDIO_READ_FAIL;
	}

	pAudDec->wi += ret;
	if(pAudDec->wi >= pAudDec->ring_size) {
		pAudDec->wi -= pAudDec->ring_size;
	}
	
	pAudDec->offset += ret;
	pAudDec->read_secs += (ret/512);
	return ret;	
}

static INT32S audio_post_process_set(AudDecCtrl_t *pAudDec)
{
	ParserInfo_t *pInfo = &pAudDec->info;
	AudPostProc_t *ppp = &pAudDec->post_proc;
	
	ppp->channel_no = pInfo->channel_no;
	ppp->sample_rate = pInfo->sample_rate;
	
#if APP_CONST_PITCH_EN == 1
	if(ppp->hConstPitch && ppp->ConstPitchEn && (ppp->channel_no == 1))
	{
		switch(ppp->sample_rate)
		{
		case 8000:
		case 16000:
		case 11025:
		case 22050:
			ConstantPitch_Link(ppp->hConstPitch, ppp->hSrc, ppp->pfnGetOutput, ppp->sample_rate, ppp->channel_no, 0);
			ConstantPitch_SetParam(ppp->hConstPitch,ppp->Pitch_idx, 0);
			ppp->hSrc = ppp->hConstPitch;
			ppp->sample_rate = ConstantPitch_GetSampleRate(ppp->hConstPitch);
			ppp->channel_no = ConstantPitch_GetChannel(ppp->hConstPitch);
			ppp->pfnGetOutput = &ConstantPitch_GetOutput;
			break;
			
		default:
			return (0 - AUDIO_ERR_FAILED);
		}
	}
#endif

#if APP_ECHO_EN == 1
	if(ppp->hEcho && ppp->EchoEn && (ppp->channel_no == 1))
	{
		Echo_Link(ppp->hEcho, ppp->hSrc, ppp->pfnGetOutput, ppp->sample_rate, ppp->channel_no, 0);
		Echo_SetParam(ppp->hEcho, ppp->delay_len, ppp->weight_idx);
		ppp->hSrc = ppp->hEcho;
		ppp->sample_rate = Echo_GetSampleRate(ppp->hEcho);
		ppp->channel_no = Echo_GetChannel(ppp->hEcho);
		ppp->pfnGetOutput = &Echo_GetOutput;
	}
#endif	

#if APP_VOICE_CHANGER_EN == 1
	if(ppp->hVC && ppp->VoiceChangerEn)
	{
		VoiceChanger_Link(ppp->hVC, ppp->hSrc, ppp->pfnGetOutput, ppp->sample_rate, ppp->channel_no, 0);
		VoiceChanger_SetParam(ppp->hVC, ppp->Speed, ppp->Pitch);
		ppp->hSrc = ppp->hVC;
		ppp->sample_rate = VoiceChanger_GetSampleRate(ppp->hVC);
		ppp->channel_no = VoiceChanger_GetChannel(ppp->hVC);
		ppp->pfnGetOutput = &VoiceChanger_GetOutput;
	}
#endif

#if APP_UP_SAMPLE_EN
	if(ppp->hUpSample &&ppp->UpSampleEn) {
		INT32U N = ppp->UpSampleRate / ppp->sample_rate;
	
		if(N > 4) {
			N = 4;
		}
	
		if(N >= 2) {
			UpSample_Link(ppp->hUpSample, ppp->hSrc, ppp->pfnGetOutput, ppp->sample_rate, ppp->channel_no, N, 0);
			ppp->hSrc = ppp->hUpSample;
			ppp->sample_rate = UpSample_GetSampleRate(ppp->hUpSample);
			ppp->channel_no = UpSample_GetChannel(ppp->hUpSample);
			ppp->pfnGetOutput = &UpSample_GetOutput;
		}	
	}
#endif
	MSG("pp Channel: %d\r\n", ppp->channel_no);
	MSG("pp Sample_rate: %d\r\n", ppp->sample_rate);
	return AUDIO_ERR_NONE;
}

// S880 decode api
#if APP_S880_DECODE_EN == 1
static INT32S audio_s880_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = S880_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_DEC_FINISH);
		}
		
		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = S880_dec_run(work_mem, Buffer, audio_ctrl->wi);
		
		audio_ctrl->ri = S880_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;	
	return pcm_point;
}

static INT32S audio_s880_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;
	
	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// s880 init
	if(S880_dec_get_mem_block_size() != S880_DEC_MEMORY_SIZE) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	ret = S880_dec_init(work_mem, (unsigned char *)audio_ctrl->ring_buf);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = S880_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	count = 500;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = S880_dec_parsing(work_mem , audio_ctrl->wi);
		
		audio_ctrl->ri = S880_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;
		
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case S880_OK:
				break;
			
			case S880_E_NO_MORE_SRCDATA:		//not found sync word
			case S880_E_READ_IN_BUFFER:			//reserved sample frequency value
			case S880_CODE_FILE_FORMAT_ERR:		//forbidden bitrate value
			case S880_E_FILE_END:
				audio_ctrl->ri = S880_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				
				if (--count == 0) {
					return (0 - AUDIO_ERR_FAILED);
				}
				continue;
			
			default:
				return (0 - AUDIO_ERR_FAILED);
		}
		
		if(ret == S880_OK) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = S880_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = S880_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = S880_dec_get_channel(work_mem);
	size = audio_ctrl->info.file_len - audio_ctrl->header_len;
	audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_s880_get_output;
	
	MSG("s880 duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("s880 channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("s880 sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_s880_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;

	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, S880_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}
#endif 

// A1600 decode api
#if APP_A1600_DECODE_EN == 1
static INT32S audio_a16_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = A16_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		// check decode end		
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_DEC_FINISH);
		}

		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = A16_dec_run(work_mem, Buffer, audio_ctrl->wi);
		
		audio_ctrl->ri = A16_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;	
	return pcm_point;	
}

static INT32S audio_a16_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// a1600 init
	if(A16_dec_get_mem_block_size() != A16_DEC_MEMORY_SIZE) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	ret = A16_dec_init(work_mem, (unsigned char *)audio_ctrl->ring_buf);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = A16_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	count = 500;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = A16_dec_parsing(work_mem , audio_ctrl->wi);

		audio_ctrl->ri = A16_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;
		
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case A16_OK:
				break;
			
			case A16_E_NO_MORE_SRCDATA:		//not found sync word
			case A16_E_READ_IN_BUFFER:		//reserved sample frequency value
			case A16_CODE_FILE_FORMAT_ERR:	//forbidden bitrate value
			case A16_E_FILE_END:
			case A16_E_MODE_ERR:
				audio_ctrl->ri = A16_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}

				if (--count == 0) {
					return (0 - AUDIO_ERR_FAILED);
				}
				continue;
				
			default:
				return (0 - AUDIO_ERR_FAILED);
		}
		
		if(ret == A16_OK) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = A16_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = A16_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = A16_dec_get_channel(work_mem);
	size = audio_ctrl->info.file_len - audio_ctrl->header_len;
	audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_a16_get_output;

	MSG("a1600 duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("a1600 channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("a1800 sample rate: %d\r\n", audio_ctrl->info.sample_rate);

	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_a16_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, A16_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}
#endif

// A1800 decode api
#if APP_A1800_DECODE_EN == 1
static INT32S audio_a1800_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = a1800dec_read_index(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
	
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len){
			return (0 - AUDIO_ERR_DEC_FINISH);
		}
		
		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = a1800dec_run(work_mem, audio_ctrl->wi, Buffer);
		
		audio_ctrl->ri = a1800dec_read_index(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}

	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;	
	return pcm_point;
}

static INT32S audio_a1800_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	INT8U *work_mem = (INT8U *)audio_ctrl->work_mem;
	INT32U *pData;
	INT32U in_length;
	INT32S ret, size;

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// a1800 init
	if(a1800dec_GetMemoryBlockSize() != A1800DEC_MEMORY_BLOCK_SIZE) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	ret = a1800dec_init(work_mem, (const unsigned char *)audio_ctrl->ring_buf);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	A18_dec_SetRingBufferSize(work_mem, audio_ctrl->ring_size);
	
	audio_ctrl->wi = 0;
	audio_ctrl->ri = a1800dec_read_index(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// check for skip gp header
	pData = (INT32U *)audio_ctrl->ring_buf;
	if(*pData == 0xFF00FF00) {
		audio_ctrl->wi = audio_ctrl->ri = 0;
		audio_ctrl->offset = 48;
		audio_ctrl->file_cnt = 48;
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
	}
	
	in_length = audio_ctrl->ri;
	ret = a1800dec_parsing(work_mem, audio_ctrl->wi);
	if(ret != 1) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	audio_ctrl->ri = a1800dec_read_index(work_mem);
	size = audio_ctrl->ri - in_length;
	if(size < 0) {
		size += audio_ctrl->ring_size;
	}
	
	audio_ctrl->header_len += size;	
	audio_ctrl->file_cnt += size;
		
	audio_ctrl->info.bit_rate = A18_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = A18_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = A18_dec_get_channel(work_mem);
	size = audio_ctrl->info.file_len - audio_ctrl->header_len;
	audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_a1800_get_output;
	
	MSG("a1800 duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("a1800 channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("a1800 sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_a1800_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, A18_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}
#endif 

// A6400 decode api
#if APP_A6400_DECODE_EN == 1
static INT32S audio_a64_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = a6400_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		// check decode end
	#if GPLIB_MP3_HW_EN == 1 
		in_length = audio_ctrl->info.file_len;
	#else
		in_length = audio_ctrl->info.file_len - 1;
	#endif
		if(audio_ctrl->file_cnt >= in_length) {	
			if(audio_ctrl->f_last) {
				return (0 - AUDIO_ERR_DEC_FINISH);
			} else {
				audio_ctrl->f_last = 1;
			}
		}

		// decode and compute length
		in_length = audio_ctrl->ri;
	#if GPLIB_MP3_HW_EN == 1 
		pcm_point = a6400_dec_run(work_mem, Buffer, audio_ctrl->wi, audio_ctrl->f_last);
	#else
		pcm_point = a6400_dec_run(work_mem, Buffer, audio_ctrl->wi);
	#endif	
	
		audio_ctrl->ri = a6400_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FINISH);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;
#if GPLIB_MP3_HW_EN == 1	
	return (pcm_point * 2); /* 2 channel */
#else
	return (pcm_point * a6400_dec_get_channel(work_mem));
#endif
}

static INT32S audio_a64_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// a6400 init
	if(a6400_dec_get_mem_block_size() != A6400_DEC_MEMORY_SIZE) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

#if GPLIB_MP3_HW_EN == 1
	ret = a6400_dec_init(work_mem, (unsigned char *)audio_ctrl->ring_buf);
#else
	ret = a6400_dec_init(work_mem, (unsigned char *)audio_ctrl->ring_buf, (char *)(work_mem + A6400_DEC_MEMORY_SIZE));
	a6400_dec_set_bs_buf_size(work_mem, audio_ctrl->ring_size);
#endif	
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = a6400_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	count = 500;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = a6400_dec_parsing(work_mem , audio_ctrl->wi);

		audio_ctrl->ri = a6400_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		audio_ctrl->header_len += size;

		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case A6400_DEC_ERR_NONE:
				break;
				
			case A6400_DEC_ERR_LOSTSYNC:		//not found sync word
			case A6400_DEC_ERR_BADSAMPLERATE:	//reserved sample frequency value
			case A6400_DEC_ERR_BADBITRATE:		//forbidden bitrate value
			case A6400_DEC_ERR_BADLAYER:
				//feed in DecodeInBuffer;
				audio_ctrl->ri = a6400_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}

				if (--count == 0) {
					return (0 - AUDIO_ERR_FAILED);
				}
				continue;
			
			default:
				return (0 - AUDIO_ERR_FAILED);
		}
		
		if(ret == A6400_DEC_ERR_NONE) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = a6400_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = a6400_dec_get_samplerate(work_mem);
#if GPLIB_MP3_HW_EN == 1	
	audio_ctrl->info.channel_no = 2;
#else
	audio_ctrl->info.channel_no = a6400_dec_get_channel(work_mem);
#endif
	size = audio_ctrl->info.file_len - audio_ctrl->header_len;
	audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_a64_get_output;
	
	MSG("a6400 duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("a6400 channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("a6400 sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_a64_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, A6400_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;	
}
#endif

// WAVE decode api 
#if APP_WAV_CODEC_EN == 1
static INT32S audio_wave_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = wav_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_READ_FAIL);
		}
		
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_DEC_FINISH);
		} 
		
		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = wav_dec_run(work_mem, Buffer, audio_ctrl->wi);
		if (pcm_point == -1) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		audio_ctrl->ri = wav_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (--audio_ctrl->retry_cnt == 0) {
				return (0 - AUDIO_ERR_DEC_FAIL);
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;
	return (pcm_point * wav_dec_get_nChannels(work_mem));
}

static INT32S audio_wave_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	INT8U *work_mem = (INT8U *)audio_ctrl->work_mem;
	INT32U in_length;
	INT32S ret, size;
	
	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->retry_cnt = 10;
	audio_ctrl->dec_pcm_len = 0;
	
	// wave init
	ret = wav_dec_init(work_mem, audio_ctrl->ring_buf);
	wav_dec_set_ring_buf_size(work_mem, audio_ctrl->ring_size);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = wav_dec_get_ri(work_mem);
	audio_ctrl->ri = wav_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_READ_FAIL);
	}

	in_length = audio_ctrl->ri;
	ret = wav_dec_parsing(work_mem, audio_ctrl->wi);
	if(ret <= 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	audio_ctrl->info.file_len = ret;
	audio_ctrl->ri = wav_dec_get_ri(work_mem);
	size = audio_ctrl->ri - in_length;
	if(size < 0) {
		size += audio_ctrl->ring_size;
	}
	
	audio_ctrl->header_len += size;	
	audio_ctrl->file_cnt += size;
	
	audio_ctrl->info.bit_rate = wav_dec_get_nAvgBytesPerSec(work_mem);
	ret = wav_dec_get_wFormatTag(work_mem);
	switch(ret)
	{
	case WAVE_FORMAT_PCM:
	case WAVE_FORMAT_ADPCM:
	case WAVE_FORMAT_IMA_ADPCM:
		audio_ctrl->info.bit_rate <<= 3;
		break;
	}
	
	audio_ctrl->info.sample_rate = wav_dec_get_SampleRate(work_mem);
	audio_ctrl->info.channel_no = wav_dec_get_nChannels(work_mem);
	size = audio_ctrl->info.file_len - audio_ctrl->header_len;
	audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_wave_get_output;
	
	MSG("wave duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("wave channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("wave sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_wave_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, WAV_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}

static INT32S audio_wave_seek_play(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32S offset, N;

	offset = (audio_ctrl->info.bit_rate >> 3) * audio_ctrl->seek_time;
	N = offset / wav_dec_get_nBlockAlign(work_mem);
	offset = wav_dec_get_nBlockAlign(work_mem) * N;
	offset += audio_ctrl->header_len;
	audio_ctrl->offset = offset;
	audio_ctrl->file_cnt = offset;
	
	wav_dec_set_ri(work_mem, 0);
	audio_ctrl->wi = audio_ctrl->ri = 0;
	return AUDIO_ERR_NONE;
}
#endif

// MP3 decode api
#if APP_MP3_DECODE_EN == 1
static INT8U parser_id3_get_type(INT8U *data, INT32U length)
{
	if (length >= 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G') {
    	return ID3_TAG_V1;
	}
	
  	if (length >= 10 && ((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
      	(data[0] == '3' && data[1] == 'D' && data[2] == 'I')) && data[3] < 0xff && data[4] < 0xff)
   	{ 
   		if (data[0] == 'I')
   			return ID3_TAG_V2;
   		else
   			return ID3_TAG_V2_FOOTER;
	}
  	return ID3_TAG_NONE;
}

static INT32U parser_id3_get_size(INT8U *ptr)
{
	INT32U value = 0;
	
	value = (value << 7) | (*ptr++ & 0x7f);
    value = (value << 7) | (*ptr++ & 0x7f);
	value = (value << 7) | (*ptr++ & 0x7f);
	value = (value << 7) | (*ptr++ & 0x7f);
 
  	return value;
}

static void parser_id3_header(INT8U *header, INT32U *version, INT32S *flags, INT32U *size)
{
	INT8U *ptr;
	INT32U ver = 0;
	
	ptr = header;
  	ptr += 3;
  	ver = *ptr++ << 8; 
  	*version = ver | *ptr++;
  	*flags = *ptr++;
  	*size = parser_id3_get_size(ptr);
}

static INT32S parser_id3_get_tag_len(INT8U *data, INT32U length)
{
  INT32U version;
  INT32S flags;
  INT32U len;

  switch (parser_id3_get_type(data, length)) {
 	case ID3_TAG_V1:
    	return 128;
  	case ID3_TAG_V2:
    	parser_id3_header(data, &version, &flags, &len);
    	if (flags & ID3_TAG_FLAG_FOOTER) {
      		len += 10;
		}
    	return 10 + len;
  	case ID3_TAG_NONE:
    	break;
  }

  return 0;
}

static INT32S audio_mp3_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = mp3_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_READ_FAIL);
		}
		
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			if(audio_ctrl->f_last) {
				return (0 - AUDIO_ERR_DEC_FINISH);
			} else {
				audio_ctrl->f_last = 1;
			}
		}

	#if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedLock();
	#endif
	
		// decode and compute length
		in_length = audio_ctrl->ri;
	#if GPLIB_MP3_HW_EN == 1		
		pcm_point = mp3_dec_run(work_mem, Buffer, audio_ctrl->wi, audio_ctrl->f_last);
	#else
		pcm_point = mp3_dec_run(work_mem, Buffer, audio_ctrl->wi);
	#endif
	
	#if _OPERATING_SYSTEM == _OS_UCOS2
		OSSchedUnlock();
	#endif
		
		audio_ctrl->ri = mp3_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;
#if GPLIB_MP3_HW_EN == 1	
	return (pcm_point * 2); /* 2 channel */
#else
	return (pcm_point * mp3_dec_get_channel((CHAR*)work_mem));
#endif
}

static INT32S audio_mp3_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT8U *pData;
	INT32S ret, size;
	INT32U in_length, count;
	
	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;

	// mp3 init
#if GPLIB_MP3_HW_EN == 1
	ret = mp3_dec_init(work_mem, (void*)audio_ctrl->ring_buf);
	mp3_dec_set_ring_size(work_mem, audio_ctrl->ring_size);
#else
	ret = mp3_dec_init(work_mem, (unsigned char*)audio_ctrl->ring_buf, (char*)(work_mem + MP3_DEC_MEMORY_SIZE));
	mp3_dec_set_bs_buf_size(work_mem, audio_ctrl->ring_size);
#endif
	if(ret < 0) {
		return (0 - AUDIO_ERR_READ_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = mp3_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_READ_FAIL);
	}
	
	// get id3 type and skip
   	audio_ctrl->mp3_ID3V2_len = parser_id3_get_tag_len(audio_ctrl->ring_buf, audio_ctrl->ring_size/2);
   	if(audio_ctrl->mp3_ID3V2_len) {
   		MSG("ID3Len = %d\r\n", audio_ctrl->mp3_ID3V2_len);
   		audio_ctrl->offset = audio_ctrl->mp3_ID3V2_len;
   		audio_ctrl->file_cnt = audio_ctrl->mp3_ID3V2_len; 
   		audio_ctrl->read_secs = 0;
   		
   		// refill ring buffer
   		audio_ctrl->wi = 0;
		audio_ctrl->ri = mp3_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_READ_FAIL);
		}
	}
	
	// check vbr flag
	pData = (INT8U*)audio_ctrl->ring_buf;
	if((*(pData+36)=='X') && (*(pData+37)=='i') && (*(pData+38)=='n') && (*(pData+39)=='g')) {
		MSG("MP3VBR = %d\r\n", audio_ctrl->mp3_VBR_flag);
		audio_ctrl->mp3_VBR_flag = 1;// VBR
	} else {
		audio_ctrl->mp3_VBR_flag = 0;// CBR
	}
	
	// VBR total frame	
	if(audio_ctrl->mp3_VBR_flag) { 
		if((*(pData + 43)) | 0x01) { // total frame number exist
			audio_ctrl->mp3_total_frame = (INT32U)(*(pData+44)<<24) | (*(pData+45)<<16) | (*(pData+46)<<8) | (*(pData+47));
			if(audio_ctrl->mp3_total_frame == 0) {
				audio_ctrl->mp3_VBR_flag = 0;
			}
		} else {
			audio_ctrl->mp3_VBR_flag = 0;
		}
	}

	count = 400;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = mp3_dec_parsing(work_mem , audio_ctrl->wi);

		audio_ctrl->ri = mp3_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;
		
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case MP3_DEC_ERR_NONE:
				break;
				
			case MP3_DEC_ERR_LOSTSYNC:		//not found sync word
			case MP3_DEC_ERR_BADSAMPLERATE:	//reserved sample frequency value
			case MP3_DEC_ERR_BADBITRATE:		//forbidden bitrate value
			case MP3_DEC_ERR_BADLAYER:
				//feed in DecodeInBuffer;
				audio_ctrl->ri = mp3_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_READ_FAIL);
				}
				
				if (--count == 0) {
					return AUDIO_ERR_DEC_FAIL;
				}
				continue;
				
			default:
				return AUDIO_ERR_DEC_FAIL;
		}
		
		if(ret == MP3_DEC_ERR_NONE) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = mp3_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = mp3_dec_get_samplerate(work_mem);
#if GPLIB_MP3_HW_EN == 1	
	audio_ctrl->info.channel_no = 2;
#else
	audio_ctrl->info.channel_no = mp3_dec_get_channel(work_mem);
#endif
	if(audio_ctrl->mp3_VBR_flag) {
		INT32U layer = mp3_dec_get_layer(work_mem);
		INT32U frame;
		
		if(gp_strcmp((INT8S *)mp3_dec_get_mpegid(work_mem), (INT8S *)"mpeg1") == 0) {
			if(layer == 2 || layer == 3) {
				frame = 1152;
			} else {
				frame = 384;
			}
		} else {
			// mpeg2 and mpeg2.5
			if(layer == 3) {
				frame = 576;
			} else if(layer == 2) {
				frame = 1152;
			} else {
				frame = 384;
			}
		}
		
		audio_ctrl->info.duration = (audio_ctrl->mp3_total_frame * frame) / audio_ctrl->info.sample_rate;
	} else {
		size = audio_ctrl->info.file_len - audio_ctrl->mp3_ID3V2_len - audio_ctrl->header_len;
		audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	}
	
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_mp3_get_output;
	
	MSG("mp3 duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("mp3 channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("mp3 sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_mp3_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, MP3_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;		
}

static INT32S audio_mp3_seek_play(void *ch_wm)
{
	extern void mp3_dec_set_ri(char *work_mem, INT32S ri);
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32S offset;
	
	if(audio_ctrl->mp3_VBR_flag) {
		MSG("not support vbr mp3 seek now\r\n");
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
		
	offset = audio_ctrl->mp3_ID3V2_len + audio_ctrl->header_len + 
			((audio_ctrl->info.bit_rate >> 3) * audio_ctrl->seek_time);
	
	audio_ctrl->wi = audio_ctrl->ri = 0;
	mp3_dec_set_ri(work_mem, audio_ctrl->ri);
	audio_ctrl->offset = offset;
	audio_ctrl->file_cnt = offset;
	return AUDIO_ERR_NONE;
}
#endif

// WMA decode api
#if APP_WMA_DECODE_EN == 1
static INT32S audio_wma_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = wma_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			if(audio_ctrl->f_last){
				return (0 - AUDIO_ERR_DEC_FINISH);
			} else {
				audio_ctrl->f_last = 1;
			}
		}
		
		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = wma_dec_run(work_mem, Buffer, audio_ctrl->wi);
		
		audio_ctrl->ri = wma_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else { 
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;
	return (pcm_point * wma_dec_get_channel(work_mem));
}

static INT32S audio_wma_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;

	// wma init
	ret = wma_dec_init(work_mem, audio_ctrl->ring_buf,(char *)work_mem + 8192, audio_ctrl->ring_size);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}

	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = wma_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	count = 50;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = wma_dec_parsing(work_mem , audio_ctrl->wi);
		
		audio_ctrl->ri = wma_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;
		
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case WMA_OK:
				break;
			
			case WMA_E_BAD_PACKET_HEADER:
			case WMA_E_INVALIDHEADER:
			case WMA_E_NOTSUPPORTED:
			case WMA_E_NOTSUPPORTED_CODEC:
				return AUDIO_ERR_INVALID_FORMAT;
			
			case WMA_E_ONHOLD:
			case WMA_E_DATANOTENOUGH:
				//feed in DecodeInBuffer;
				audio_ctrl->ri = wma_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				
				if (--count == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				continue;
				
			default:
				if (--count == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				break;
		}

		if(ret == WMA_OK) {
			break;
		}
	}
	
	audio_ctrl->info.bit_rate = wma_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = wma_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = wma_dec_get_channel(work_mem);
	audio_ctrl->info.duration = wma_dec_get_playtime(work_mem) / 1000;
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_wma_get_output;
	
	MSG("wma duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("wma channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("wma sample rate: %d\r\n", audio_ctrl->info.sample_rate);
	
	return AUDIO_ERR_NONE;
}

static INT32S audio_wma_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, WMA_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;	
}

static INT32S audio_wma_seek_play(void *ch_wm)
{
	extern int wma_dec_seek(void *work_mem, INT32U sec);
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32S ret, offset;
	
	wma_dec_reset_offset(work_mem);
	if(audio_ctrl->seek_time >= wma_dec_get_playtime(work_mem)) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	ret = audio_wma_play_init(ch_wm);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	offset = wma_dec_seek(work_mem, audio_ctrl->seek_time);
	MSG("seek=%d, offset=0x%x\r\n", audio_ctrl->seek_time, offset);
	audio_ctrl->wi = 0;
	audio_ctrl->ri = 0;
	audio_ctrl->offset = offset;
	audio_ctrl->file_cnt = offset;
	wma_dec_set_ri(work_mem, 0);
	wma_dec_reset_offset(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	return AUDIO_ERR_NONE;
}
#endif

// AAC decode api
#if APP_AAC_DECODE_EN == 1
static INT32S audio_aac_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = aac_dec_get_read_index(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}

		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_DEC_FINISH);
		}

		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = aac_dec_run(work_mem, audio_ctrl->wi, Buffer);

		audio_ctrl->ri = aac_dec_get_read_index(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;	
	return (pcm_point * aac_dec_get_channel(work_mem));
}

static INT32S audio_aac_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;
	INT32S downMatrix = 0;	// 1: input 5.1 channels and output 2 channels

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// aac init
	ret = aac_dec_init(work_mem, downMatrix, (INT8U *)audio_ctrl->ring_buf);
	aac_dec_SetRingBufferSize(work_mem, audio_ctrl->ring_size);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = aac_dec_get_read_index(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	count = 100;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = aac_dec_parsing(work_mem, audio_ctrl->wi);

		audio_ctrl->ri = aac_dec_get_read_index(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;

		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}

		switch(ret)
		{
			case AAC_OK:
				break;

			case  UNABLE_TO_FIND_ADTS_SYNCWORD:
				//feed in DecodeInBuffer;
				audio_ctrl->ri = aac_dec_get_read_index(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				
				if (--count == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				continue;
				
			case UNSUPPORTED_FILE_FORMAT_MP4:		
			case NOT_MONO_OR_STEREO:
			case NOT_LC_OBJECT_TYPE:
			default:
				return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		if(ret == AAC_OK) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = aac_dec_get_bitspersample(work_mem);
	audio_ctrl->info.sample_rate = aac_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = aac_dec_get_channel(work_mem);
	if(audio_ctrl->info.bit_rate) {
		size = audio_ctrl->info.file_len - audio_ctrl->header_len;
		audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	} else {
		audio_ctrl->info.duration = 0;
	}
	
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_aac_get_output;

	MSG("aac duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("aac channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("aac sample rate: %d\r\n", audio_ctrl->info.sample_rate);

	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_aac_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;

	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, AAC_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}
#endif 

// OGG decode api
#if APP_OGG_DECODE_EN == 1
static INT32S audio_ogg_get_output(void *work_mem, short *Buffer, INT32S MaxLen)
{
	INT32S size, pcm_point;
	INT32U in_length;
	AudDecCtrl_t *audio_ctrl = g_audio_ctrl;
	
	audio_unlock(audio_ctrl);
	while(1)
	{
		// fill ring buffer
		audio_ctrl->ri = oggvorbis_dec_get_ri(work_mem);
		if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
			return (0 - AUDIO_ERR_DEC_FAIL);
		}
	
		// check decode end
		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_DEC_FINISH);
		}
		
		// decode and compute length
		in_length = audio_ctrl->ri;
		pcm_point = oggvorbis_dec_run(work_mem, Buffer, audio_ctrl->wi);

		audio_ctrl->ri = oggvorbis_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->file_cnt += size;
		if (pcm_point <= 0) {
			if (pcm_point < 0) {
				if (--audio_ctrl->retry_cnt == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
			}
		} else {
			break;
		}
	}
	
	// add total output pcm length 
	audio_ctrl->dec_pcm_len += pcm_point;	
	return (pcm_point * oggvorbis_dec_get_channel(work_mem));
}

static INT32S audio_ogg_play_init(void *ch_wm)
{
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	CHAR *work_mem = (CHAR *)audio_ctrl->work_mem;
	INT32U in_length, count;
	INT32S ret, size;

	audio_ctrl->header_len = 0;
	audio_ctrl->offset = 0;
	audio_ctrl->read_secs = 0;
	audio_ctrl->file_cnt = 0;
	audio_ctrl->f_last = 0;
	audio_ctrl->retry_cnt = 20;
	audio_ctrl->dec_pcm_len = 0;
	
	// aac init
	ret = oggvorbis_dec_init(work_mem, (const char*)audio_ctrl->ring_buf, audio_ctrl->ring_size, 0);
	oggvorbis_dec_set_ring_buffer(work_mem, (const char*)audio_ctrl->ring_buf, audio_ctrl->ring_size, 0);
	if(ret < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	// fill ring buffer
	audio_ctrl->wi = 0;
	audio_ctrl->ri = oggvorbis_dec_get_ri(work_mem);
	if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
		return (0 - AUDIO_ERR_DEC_FAIL);
	}
	
	count = 500;
	while(1)
	{
		in_length = audio_ctrl->ri;
		ret = oggvorbis_dec_parsing(work_mem, audio_ctrl->wi);

		audio_ctrl->ri = oggvorbis_dec_get_ri(work_mem);
		size = audio_ctrl->ri - in_length;
		if(size < 0) {
			size += audio_ctrl->ring_size;
		}
		
		audio_ctrl->header_len += size;
		audio_ctrl->file_cnt += size;

		if(audio_ctrl->file_cnt >= audio_ctrl->info.file_len) {
			return (0 - AUDIO_ERR_FAILED);
		}
		
		switch(ret)
		{
			case OGGVORBIS_PARSING_OK:
				break;

			case OGGVORBIS_MORE_DATA:
				//feed in DecodeInBuffer;
				audio_ctrl->ri = oggvorbis_dec_get_ri(work_mem);
				if(audio_decode_fill_ring_buf(audio_ctrl) < 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				
				if (--count == 0) {
					return (0 - AUDIO_ERR_DEC_FAIL);
				}
				continue;
				
			default:
				return (0 - AUDIO_ERR_DEC_FAIL);
		}
		
		if(ret == OGGVORBIS_PARSING_OK) {
			break;
		}
	}

	audio_ctrl->info.bit_rate = oggvorbis_dec_get_bitrate(work_mem);
	audio_ctrl->info.sample_rate = oggvorbis_dec_get_samplerate(work_mem);
	audio_ctrl->info.channel_no = oggvorbis_dec_get_channel(work_mem);
	if(audio_ctrl->info.bit_rate) {
		size = audio_ctrl->info.file_len - audio_ctrl->header_len;
		audio_ctrl->info.duration = size / (audio_ctrl->info.bit_rate >> 3);
	} else {
		audio_ctrl->info.duration = 0;
	}
	
	audio_ctrl->post_proc.hSrc = audio_ctrl->work_mem;
	audio_ctrl->post_proc.pfnGetOutput = audio_ogg_get_output;

	MSG("ogg duration: %d sec\r\n", audio_ctrl->info.duration);
	MSG("ogg channel: %d\r\n", audio_ctrl->info.channel_no);
	MSG("ogg sample rate: %d\r\n", audio_ctrl->info.sample_rate);

	// post process
	return audio_post_process_set(audio_ctrl);
}

static INT32S audio_ogg_process(void *ch_wm)
{
	INT16S *pcm_buf;
	INT32S pcm_point;
	AudDecCtrl_t *audio_ctrl = (AudDecCtrl_t *)ch_wm;
	AudPostProc_t *ppp = &audio_ctrl->post_proc;
	
	pcm_buf = audio_out_get_empty_buf(audio_ctrl->audout_wrokmem);
	if(pcm_buf == 0) {
		return (0 - AUDIO_ERR_FAILED);
	}
	
	audio_lock(audio_ctrl);
	pcm_point = ppp->pfnGetOutput(ppp->hSrc, pcm_buf, AAC_DEC_FRAMESIZE);
	audio_unlock(audio_ctrl);
	
	if (pcm_point <= 0) {
		return AUDIO_ERR_DEC_FINISH;
	}
	
	audio_out_send_ready_buf(audio_ctrl->audout_wrokmem, pcm_buf, pcm_point);	
	return AUDIO_ERR_NONE;
}
#endif 
