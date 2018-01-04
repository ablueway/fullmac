#include "task_video_decoder.h"
#include "drv_l1_scaler.h"
#include "drv_l1_jpeg.h"
#include "drv_l2_scaler.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define TSK_PRI_VID_PARSER				VIDEO_PASER_PRIORITY
#define TSK_PRI_VID_VID_STATE			VID_DEC_STATE_PRIORITY
#define TSK_PRI_VID_VID_DEC				VIDEO_DECODE_PRIORITY
#define	TSK_PRI_VID_AUD_DEC				AUDIO_DECODE_PRIORITY

#define C_VID_DEC_STATE_STACK_SIZE		512
#define C_VID_DEC_QUEUE_MAX_LEN    		5

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/ 
typedef enum
{
	MSG_VIDEO_DECODE_PARSER_HEADER_START = 0x1000,
	MSG_VIDEO_DECODE_PARSER_HEADER_STOP,
	MSG_VIDEO_DECODE_START,
	MSG_VIDEO_DECODE_STOP,	
	MSG_VIDEO_DECODE_PAUSE,
	MSG_VIDEO_DECODE_RESUME,
	MSG_VIDEO_DECODE_GET_CUR_TIME,
	MSG_VIDEO_DECODE_SET_SEEK,
	MSG_VIDEO_DECODE_SET_SEEK_PLAY,
	MSG_VIDEO_DECODE_SET_PLAY_SPEED,
	MSG_VIDEO_DECODE_SET_REVERSE,
	MSG_VIDEO_DECODE_NTH_FRAME,
	MSG_VIDEO_DECODE_VID_END,
	MSG_VIDEO_DECODE_AUD_END,
	MSG_VIDEO_DECODE_ERROR,
	MSG_VIDEO_DECODE_EXIT 
} MSG_VID_DEC_STATE_ENUM;

typedef union
{
	long v;
	char s[4];
} FOURCC_t;

typedef struct
{
	long len;
	FOURCC_t type;
} ATOM_HDR_t;

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define MAKEFOURCC(ch0, ch1, ch2, ch3)				    \
		((unsigned long)(unsigned char)(ch0)		 |	\
		((unsigned long)(unsigned char)(ch1) << 8)   |	\
		((unsigned long)(unsigned char)(ch2) << 16)  |	\
		((unsigned long)(unsigned char)(ch3) << 24))
	
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/ 
INT32U	vid_dec_state_stack[C_VID_DEC_STATE_STACK_SIZE];
OS_EVENT *vid_dec_state_q;
OS_EVENT *vid_dec_state_ack_m;
void *vid_dec_state_q_buffer[C_VID_DEC_QUEUE_MAX_LEN];

video_decode_para_struct vid_dec_para;
video_decode_para_struct *p_vid_dec_para;
const GP_BITMAPINFOHEADER *p_bitmap_info;
const GP_WAVEFORMATEX *p_wave_info;
long  g_bitmap_info_len, g_wave_info_len;
void (*viddec_end)(void);


INT32S vid_dec_entry(void)
{
	INT32S nRet;

	nRet = video_decode_state_task_create(TSK_PRI_VID_VID_STATE);
	if(nRet < 0) {
		DEBUG_MSG("video_decode_state_task_create Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = video_decode_task_create(TSK_PRI_VID_VID_DEC);
	if(nRet < 0) {
		DEBUG_MSG("video_decode_task_create Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = audio_decode_task_create(TSK_PRI_VID_AUD_DEC);
	if(nRet < 0) {
		DEBUG_MSG("audio_decode_task_create Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_dec_exit(void)
{
	INT32S nRet;

	nRet = video_decode_state_task_del(TSK_PRI_VID_VID_STATE);
	if(nRet < 0) {
		DEBUG_MSG("video_decode_state_task_del Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = video_decode_task_del(TSK_PRI_VID_VID_DEC);
	if(nRet < 0) {
		DEBUG_MSG("video_decode_task_del Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = audio_decode_task_del(TSK_PRI_VID_AUD_DEC);
	if(nRet < 0) {
		DEBUG_MSG("audio_decode_task_del Fail!!!\r\n");
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
Return:	
	return nRet;
}

INT32S vid_dec_parser_start(INT16S fd, INT32S FileTpye, INT64U FileSize)
{
	INT8U  err;
	INT32S nRet, msg;

	if(fd < 0) {
		RETURN(STATUS_FAIL);
	}
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) { 
		RETURN(STATUS_FAIL);
	}	
		
	nRet = STATUS_OK;	
	p_vid_dec_para->file_handle = fd;
	p_vid_dec_para->FileType = FileTpye;
	p_vid_dec_para->FileSize = FileSize;
	if((vid_dec_get_status() & C_VIDEO_DECODE_PARSER) == 0) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_START, vid_dec_state_ack_m, 10000, msg, err); //wait 10 sec
	}	
Return:	
	return nRet;
} 

INT32S vid_dec_parser_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);	
	}
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 10000, msg, err);
	}
Return:	
	return nRet;
}

void vid_dec_enable_resume_play(void)
{
	p_vid_dec_para->bResumePlayFlag |= C_RESUME_PLAY;
}

void vid_dec_store_play_state(void)
{
	p_vid_dec_para->bResumePlayFlag |= C_STORE_STATE;
}

INT32S vid_dec_start(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER_NTH) {
		RETURN(STATUS_FAIL);
	}
		
	nRet = STATUS_OK;
	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0) {	
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_START, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:	
	return nRet;
}

INT32S vid_dec_stop(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) { 
		RETURN(STATUS_FAIL);
	}
		
#if AUD_DEC_RESTART_EN == 0	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE) {
    	if(vid_dec_get_audio_flag()) {
			audio_buffer_set_mute();
		}
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_RESUME, vid_dec_state_ack_m, 5000, msg, err);
	}
#endif
		
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:
	return nRet;
}

INT32S vid_dec_pause(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) { 
		RETURN(STATUS_FAIL);
	}
	
	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0) {
		RETURN(STATUS_FAIL);
	}
			
	nRet = STATUS_OK;
	if(!(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE)) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PAUSE, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:	
	return nRet;
}

INT32S vid_dec_resume(void)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PAUSE) {
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_RESUME, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:	
	return nRet;
}

INT32S vid_dec_get_total_samples(void)
{
	return p_vid_dec_para->video_total_sample;
}

INT32S vid_dec_get_total_time(void)
{
	return p_vid_dec_para->video_total_time;
}

INT32S vid_dec_get_current_time(void)
{
	INT32S nRet;
	
	if((vid_dec_get_status() & C_VIDEO_DECODE_PARSER) == 0) {
		RETURN(0);
	}

#if 1
	OSSchedLock();	//software protect
	if(p_vid_dec_para->media_handle == 0) {
		OSSchedUnlock();
		RETURN(-1);
	}
	
	p_vid_dec_para->video_cur_time = MultiMediaParser_GetVidCurTime(p_vid_dec_para->media_handle) >> 8;
	OSSchedUnlock();
#else
	{
		INT8U  err;
		INT32S msg;

		nRet = STATUS_OK;
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_GET_CUR_TIME, vid_dec_state_ack_m, 5000, msg, err);
	}
#endif
	nRet = p_vid_dec_para->video_cur_time;
	DEBUG_MSG("Time%dSec\r\n", nRet);
Return:
	return nRet;
}

INT32S vid_dec_set_play_time(INT32U second)
{
	INT8U  err;
	INT32S nRet, msg;	
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) { 
		RETURN(STATUS_FAIL);
	}
		
	if((vid_dec_get_status() & C_VIDEO_DECODE_PARSER) == 0) { 
		RETURN(STATUS_FAIL);	
	}
	
	if(second > p_vid_dec_para->video_total_time) {
		RETURN(STATUS_FAIL);
	}
		
	nRet = STATUS_OK;
	p_vid_dec_para->video_seek_time = second;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) {
		// stop play
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
		
		// set seek
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK, vid_dec_state_ack_m, 0, msg, err);	
		
		// start play
		if(vid_dec_get_audio_flag() || vid_dec_get_video_flag()) {	
			SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK_PLAY, vid_dec_state_ack_m, 5000, msg, err);
		} else if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER) {
			SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, msg, err);		
		}
	} else if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_SEEK, vid_dec_state_ack_m, 0, msg, err);
	} else {
		RETURN(STATUS_FAIL)
	}
	nRet = p_vid_dec_para->video_seek_time;	
Return:		
	return nRet;
}

// play speed = speed / 0x10000, normal speed = 0x10000 
INT32S vid_dec_set_play_speed(INT32U speed)
{	
	INT8U  err;
	INT32S nRet, msg;	
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);
	}
		
	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0) {
		RETURN(STATUS_FAIL);	
	}
	
	if(speed == 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;	
	p_vid_dec_para->play_speed = speed;	
	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_PLAY_SPEED, vid_dec_state_ack_m, 5000, msg, err);	
Return:	
	return nRet;
}

INT32S vid_dec_set_reverse_play(INT32U reverse_play_flag)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	if((vid_dec_get_status() & C_VIDEO_DECODE_PLAY) == 0) { 
		RETURN(STATUS_FAIL);	
	}
		
	if(vid_dec_get_video_flag() == 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;	
	p_vid_dec_para->reverse_play_set = reverse_play_flag;
	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_SET_REVERSE, vid_dec_state_ack_m, 5000, msg, err);
Return:
	return nRet;
}

INT32S vid_dec_nth_frame(INT32U nth)
{
	INT8U  err;
	INT32S nRet, msg;
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) {
		RETURN(STATUS_FAIL);
	}
	
	if(vid_dec_get_video_flag() == 0) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
	if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER) {
		SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_NTH_FRAME, vid_dec_state_ack_m, 5000, msg, err);
	}
Return:
	return nRet;	
}

INT32S video_decode_state_task_create(INT8U pori)
{
	INT8U  err;
	INT32S nRet;
	
	p_vid_dec_para = &vid_dec_para;
	gp_memset((INT8S*)p_vid_dec_para, 0, sizeof(video_decode_para_struct));
    vid_dec_state_q = OSQCreate(vid_dec_state_q_buffer, C_VID_DEC_QUEUE_MAX_LEN);
    if(!vid_dec_state_q) {
    	RETURN(STATUS_FAIL);
    }
    	
    vid_dec_state_ack_m = OSMboxCreate(NULL);
	if(!vid_dec_state_ack_m) {
		RETURN(STATUS_FAIL);
	}
	
	err = OSTaskCreate(video_decode_state_task_entry, (void*) NULL, &vid_dec_state_stack[C_VID_DEC_STATE_STACK_SIZE - 1], pori);
	if(err != OS_NO_ERR) {
		RETURN(STATUS_FAIL);
	}
	
	nRet = STATUS_OK;
Return:		
	return nRet;	
}

INT32S video_decode_state_task_del(INT8U pori)
{ 
    INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK; 
    if(vid_dec_get_status() & C_VIDEO_DECODE_PLAY) {	
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_STOP, vid_dec_state_ack_m, 5000, msg, err);
    }
    
    if(vid_dec_get_status() & C_VIDEO_DECODE_PARSER) {
    	SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_PARSER_HEADER_STOP, vid_dec_state_ack_m, 5000, msg, err);
    }
    
    SEND_MESSAGE(vid_dec_state_q, MSG_VIDEO_DECODE_EXIT, vid_dec_state_ack_m, 5000, msg, err);
Return:    		
    OSQFlush(vid_dec_state_q);
   	OSQDel(vid_dec_state_q, OS_DEL_ALWAYS, &err);
	OSMboxDel(vid_dec_state_ack_m, 0, &err);
	return nRet;
}

//////////////////////////////////////////////////////////////////////////////////////
static void vid_dec_parser_end(void)
{
	INT16S fd;
	void *hMedia;
	
	if(p_vid_dec_para->media_handle) {
		hMedia = p_vid_dec_para->media_handle;
		p_vid_dec_para->media_handle = NULL;
		p_bitmap_info = NULL;
		p_wave_info = NULL;
		MultiMediaParser_Delete(hMedia);
	}
		
    if(p_vid_dec_para->file_handle >= 0) {
    	fd = p_vid_dec_para->file_handle;
    	p_vid_dec_para->file_handle = -1;
    	close(fd);
    }	
} 

static INT32S set_play_speed(void)
{
	INT32S nRet;
	OS_CPU_SR cpu_sr;
	
	DEBUG_MSG("PlaySpeed = 0x%x\r\n", p_vid_dec_para->play_speed);
	nRet = STATUS_OK;
	vid_dec_stop_timer();
	OS_ENTER_CRITICAL();
	MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
	p_vid_dec_para->tick_time2 = (p_vid_dec_para->tick_time * p_vid_dec_para->play_speed) >> 16;
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->ta = 0;
	p_vid_dec_para->tv = 0;
	p_vid_dec_para->Tv = 0;
	OS_EXIT_CRITICAL();
	
	if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0) {
		// enable audio streaming
		if(p_vid_dec_para->ff_audio_flag) {
			p_vid_dec_para->ff_audio_flag = FALSE;
			vid_dec_set_audio_flag(TRUE);
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);			
		}

		// start audio play
		if(vid_dec_get_audio_flag()) {
			if((vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) == 0) {
				if(audio_decode_task_start() < 0) {
					RETURN(STATUS_FAIL);
				}
				vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
			}
		}
	} else {
		// no video, only audio, skip set 
		if(vid_dec_get_video_flag() == 0) {
			DEBUG_MSG("NoVidData.\r\n");
			RETURN(STATUS_FAIL);
		}

		// stop audio play
		if(vid_dec_get_audio_flag()) {
			vid_dec_set_audio_flag(FALSE);
   			if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
   				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
   				if(audio_decode_task_stop() < 0) {
   					RETURN(STATUS_FAIL);	
   				}
   			}

			// disable audio streaming
   			p_vid_dec_para->ff_audio_flag = TRUE;
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);   				
		}
	}

	// start video play timer
	vid_dec_start_timer();
Return:
	return nRet;
}

static INT32S set_play_reverse(void)
{
	INT32S nRet;
	OS_CPU_SR cpu_sr;
	
	nRet = STATUS_OK;
	if(p_vid_dec_para->reverse_play_flag == p_vid_dec_para->reverse_play_set) {
		RETURN(STATUS_OK);
	}
	
	DEBUG_MSG("PlaySpeed = 0x%x\r\n", p_vid_dec_para->play_speed);
	DEBUG_MSG("PlayRev = %d\r\n", p_vid_dec_para->reverse_play_set);
	vid_dec_stop_timer();
	OS_ENTER_CRITICAL();
	MultiMediaParser_SetFrameDropLevel(p_vid_dec_para->media_handle, 0);
	p_vid_dec_para->fail_cnt = 0;
	p_vid_dec_para->ta = 0;
	p_vid_dec_para->tv = 0;
	p_vid_dec_para->Tv = 0;
	OS_EXIT_CRITICAL(); 	
			
	p_vid_dec_para->reverse_play_flag = p_vid_dec_para->reverse_play_set;
	if(p_vid_dec_para->play_speed == 0x10000 && p_vid_dec_para->reverse_play_flag == 0) {
		// enable audio streaming
		if(p_vid_dec_para->ff_audio_flag) {
			p_vid_dec_para->ff_audio_flag = FALSE;
			vid_dec_set_audio_flag(TRUE);
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 1);
		}

		// get data by normal direction  
		if(MultiMediaParser_SetReversePlay(p_vid_dec_para->media_handle, 0) < 0) {
			RETURN(STATUS_FAIL);
		}

		// restart video play 
		if(vid_dec_get_video_flag()) {
			if((vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) == 0) {
				nRet = video_decode_task_start();
			} else {
				nRet = video_decode_task_restart();
			}
			
			if(nRet < 0) {
				RETURN(STATUS_FAIL);
			}
			
			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
		}

		// start audio play
		if(vid_dec_get_audio_flag()) {
			if((vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) == 0) {
				if(audio_decode_task_start() < 0) {
					RETURN(STATUS_FAIL);
				}
				vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
			}
		}
	} else {
		/* no video, only audio, so skip set play speed */
		if(vid_dec_get_video_flag() == 0) {
			DEBUG_MSG("NoVidData.\r\n");
			RETURN(STATUS_FAIL);
		}

		// stop audio play	
		if(vid_dec_get_audio_flag()) {
			vid_dec_set_audio_flag(FALSE);
   			if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
				if(audio_decode_task_stop() < 0) {
					RETURN(STATUS_FAIL);	
				}
			}

			// disable audio streaming
   			p_vid_dec_para->ff_audio_flag = TRUE;	
			MultiMediaParser_EnableAudStreaming(p_vid_dec_para->media_handle, 0);      				
		}

		// get data by normal or reverse direction 
		if(MultiMediaParser_SetReversePlay(p_vid_dec_para->media_handle, p_vid_dec_para->reverse_play_flag) < 0) {
			RETURN(STATUS_FAIL);
		}

		// restart video play
		if(vid_dec_get_video_flag()) {
			if((vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) == 0) {
				nRet = video_decode_task_start();
			} else {
				nRet = video_decode_task_restart();
			}
			
			if(nRet < 0) {
				RETURN(STATUS_FAIL);
			}
			
			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
		}
	}
		
	// start video play timer
	vid_dec_start_timer();
Return:
	return nRet;
}

//////////////////////////////////////////////////////////////////////////////////////
INT32S vid_dec_end_callback(INT32U bflag)
{
	INT8U err=0;
	
	if(bflag == C_VIDEO_DECODE_PLAYING) {
		DEBUG_MSG("%s>>VIDEO.\r\n", __func__);
		vid_dec_stop_timer();
		err = OSQPost(vid_dec_state_q, (void*)MSG_VIDEO_DECODE_VID_END);
	}
	
	if(bflag == C_AUDIO_DECODE_PLAYING) {
		DEBUG_MSG("%s>>AUDIO.\r\n", __func__);
		err = OSQPost(vid_dec_state_q, (void*)MSG_VIDEO_DECODE_AUD_END);
	}
	
	if(err != OS_NO_ERR) {
		DEBUG_MSG("PostFail\r\n");
		return -1;
	}
	
	return 0;
}

void video_decode_error_callback(void)
{
	DEBUG_MSG("%s.\r\n", __func__);
}

int video_parser_error_handle(INT32S error_id)
{
	INT8U err;
	
	DEBUG_MSG("%s>>0x%x.\r\n", __func__, error_id);
	vid_dec_stop_timer();
	vid_dec_set_status(C_VIDEO_DECODE_ERR);
#if 1
	/* error happen close video parser automatic */
	err = OSQPost(vid_dec_state_q, (void*)MSG_VIDEO_DECODE_ERROR);
	if(err != OS_NO_ERR) {
		DEBUG_MSG("PostFail\r\n");
		return 0;
	}
	
	return 1;
#else
	/* error happen close video parser by user, call video decode stop */
	return 0;
#endif
}

static INT32S viddec_read(INT16S fd, INT32U fs_buf, INT32U cblen)
{
#if 0 //for seek and read file always is 4-align
	INT8U copyflag;
	INT32U seek_cur, seek_new, seek_end;
	INT32U temp_buf, temp_len;
	INT32S ret, n;
	
	copyflag = 0;
	temp_buf = 0;
	seek_cur = seek_new = lseek(fd, 0, SEEK_CUR);
	seek_end = seek_cur + cblen;
	DEBUG_MSG(DBG_PRINT("read[0x%x,0x%x]\r\n", seek_cur, cblen));
	
	if((seek_cur & 0x03) || (seek_end & 0x03)) {
		copyflag = 1;
	}
	
	if(copyflag) {
		if(seek_cur & 0x03) {
			seek_new = seek_cur >> 2 << 2;
		}
		
		if(seek_end & 0x03) {
			seek_end = seek_end >> 2 << 2;
			seek_end += 0x04;
		}
		
		temp_len = seek_end - seek_new;
		temp_buf = (INT32U) gp_malloc_align(temp_len, 4);
		if(temp_buf == 0) {
			goto __exit;
		}
		
		ret = lseek(fd, seek_new, SEEK_SET);
		if(ret < 0) {
			goto __exit;
		}
		
		ret = read(fd, temp_buf, temp_len);
		if(ret < 0) {
			goto __exit;
		}
		
		n = seek_cur - seek_new;
		gp_memcpy((INT8S *)fs_buf, (INT8S *)(temp_buf + n), cblen);
		
		seek_cur += cblen;
		n = lseek(fd, seek_cur, SEEK_SET);
		ret = cblen;
	} else {
		ret = read(fd, fs_buf, cblen);
	}

__exit:	
	if(temp_buf) {
		gp_free((void *)temp_buf);
	}
	
	return ret; 
#else
	INT32S ret = read(fd, fs_buf, cblen);

	if(ret < 0) {
		DEBUG_MSG("%s fail\r\n", __func__);
	}
	
	return ret;
#endif
}

static INT32S viddec_seek(INT16S handle, INT32S offset, INT16S fromwhere)
{
	INT32S ret = lseek(handle, offset, fromwhere);
	
	if(ret < 0) {
		DEBUG_MSG("%s fail\r\n", __func__);
	}
	
	return ret;
}

static int viddec_store(INT32U work_buffer, INT32U len)
{
	INT16S fid;
	INT32S ret;
	
	fid = open("C:\\VidPlayingState.bin", O_CREAT|O_TRUNC|O_WRONLY);
	if(fid < 0) {
		while(1);
	}
	
	ret = write(fid, work_buffer, len);
	close(fid);
	
	if(ret != len) {
		DEBUG_MSG("%s fail\r\n", __func__);
		ret = -1;
	}
	return ret;
}

static int viddec_load(INT32U work_buffer, INT32U len)
{
	INT16S fid;
	INT32S ret;
	
	fid = open("C:\\VidPlayingState.bin", O_RDONLY);
	if(fid < 0) {
		while(1);
	}
	
	ret = read(fid, work_buffer, len);
	close(fid);
	
	if(ret != len) {
		DEBUG_MSG("%s fail\r\n", __func__);
		ret = -1;
	}
	return ret;
}

void video_decode_state_task_entry(void *para)
{
	void    *hMedia=NULL;
	INT8U   err;
    INT32S  i, error_id, nRet;
    INT32U  msg_id;
#if AUD_DEC_RESTART_EN == 1
	INT8U   aud_pause;
#endif
    while(1)
    {      
        msg_id = (INT32U) OSQPend(vid_dec_state_q, 0, &err);
        if((!msg_id) || (err != OS_NO_ERR)) {
        	continue;
        }
        
        switch(msg_id)
        {
        case MSG_VIDEO_DECODE_PARSER_HEADER_START:
        	//clear count
        	p_vid_dec_para->n = 0;
			p_vid_dec_para->ta = 0;
			p_vid_dec_para->tv = 0;
			p_vid_dec_para->Tv = 0;
			p_vid_dec_para->fail_cnt = 0;
        	p_vid_dec_para->pend_cnt = 0;
        	p_vid_dec_para->post_cnt = 0;
        	p_vid_dec_para->video_flag = 0;
        	p_vid_dec_para->user_define_flag = 0;
        	p_vid_dec_para->video_format = 0;
        	p_vid_dec_para->deblock_flag = 0;
        	p_vid_dec_para->play_speed = 0x10000;
        	p_vid_dec_para->reverse_play_set = 0;
        	p_vid_dec_para->reverse_play_flag = 0;
        	p_vid_dec_para->ff_audio_flag = 0;	
        	p_vid_dec_para->audio_flag = 0;
        	p_vid_dec_para->scaler_flag = 0;
        	p_vid_dec_para->image_output_width = 0;
        	p_vid_dec_para->image_output_height = 0;
        	p_vid_dec_para->buffer_output_width = 0;
        	p_vid_dec_para->buffer_output_height = 0;
        	p_vid_dec_para->mpeg4_decode_out_format = C_MP4_OUTPUT_Y1UY0V;
			p_vid_dec_para->video_decode_in_format =  C_SCALER_CTRL_IN_YUYV;
			p_vid_dec_para->video_decode_out_format = C_SCALER_CTRL_OUT_YUYV;
           	for(i=0; i<AUDIO_FRAME_NO; i++) {
				p_vid_dec_para->audio_decode_addr[i] = 0;
			}
			
           	for(i=0; i<VIDEO_FRAME_NO; i++) {
				p_vid_dec_para->video_decode_addr[i] = 0;
			}
			
			for(i=0; i<SCALER_FRAME_NO; i++) {
				p_vid_dec_para->scaler_output_addr[i] = 0;
			}
			
			for(i=0; i<DEBLOCK_FRAME_NO; i++) {
				p_vid_dec_para->deblock_addr[i] = 0;
			}
			
			DEBUG_MSG("MSG_VIDEO_DECODE_PARSER_HEADER_START\r\n");
			MultiMediaParser_RegisterReadFunc(viddec_read);
			MultiMediaParser_RegisterSeekFunc(viddec_seek);
			MultiMediaParser_RegisterStoreFunc(viddec_store);
			MultiMediaParser_RegisterLoadFunc(viddec_load);	

			if(p_vid_dec_para->bResumePlayFlag & C_RESUME_PLAY) {
				p_vid_dec_para->bResumePlayFlag &= ~C_RESUME_PLAY;
				hMedia = MultiMediaParser_ReCreate(
	           		p_vid_dec_para->file_handle,
	           		p_vid_dec_para->FileSize,
	           		PARSER_AUD_BITSTREAM_SIZE,
	           		PARSER_VID_BITSTREAM_SIZE,
	           		TSK_PRI_VID_PARSER,
	           		p_vid_dec_para->FileType==FILE_TYPE_AVI ? AviParser_GetFcnTab() : QtffParser_GetFcnTab(),
	           		video_parser_error_handle,
	           		&error_id);
			} else {
           		hMedia = MultiMediaParser_Create(
           			p_vid_dec_para->file_handle,
           			p_vid_dec_para->FileSize,
           			PARSER_AUD_BITSTREAM_SIZE,
           			PARSER_VID_BITSTREAM_SIZE,
           			TSK_PRI_VID_PARSER,
           			p_vid_dec_para->FileType==FILE_TYPE_AVI ? AviParser_GetFcnTab() : QtffParser_GetFcnTab(),
           			video_parser_error_handle,
           			&error_id);
           	}
           	
           	if((error_id & 0x80000000) || !hMedia) {
           		DEBUG_MSG("ParserErrID = 0x%x\r\n", error_id);
           		vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
           		close(p_vid_dec_para->file_handle);
	           	p_vid_dec_para->file_handle = -1;
	           	p_wave_info = NULL;
           		p_bitmap_info = NULL;
           		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL);
           	} else {	
           		vid_dec_set_status(C_VIDEO_DECODE_PARSER);
           		p_vid_dec_para->media_handle = hMedia;
           		p_vid_dec_para->VidTickRate = MultiMediaParser_GetVidTickRate(p_vid_dec_para->media_handle);
           		p_vid_dec_para->video_total_sample = MultiMediaParser_GetVidTotalSamples(p_vid_dec_para->media_handle);
			 	p_vid_dec_para->video_total_time = MultiMediaParser_GetVidTrackDur(p_vid_dec_para->media_handle) >> 8;
				p_vid_dec_para->video_cur_time = 0;
           		p_bitmap_info = MultiMediaParser_GetVidInfo(hMedia, &g_bitmap_info_len);
           		p_wave_info = MultiMediaParser_GetAudInfo(hMedia, &g_wave_info_len);
				
           		switch(p_wave_info->wFormatTag)
           		{
           		case WAVE_FORMAT_PCM:
           		case WAVE_FORMAT_MULAW:
           		case WAVE_FORMAT_ALAW:
           		case WAVE_FORMAT_ADPCM:
           		case WAVE_FORMAT_DVI_ADPCM:
           		#if MP3_DECODE_ENABLE == 1
           		case WAVE_FORMAT_MPEGLAYER3:
           		#endif
           		#if AAC_DECODE_ENABLE == 1
           		case WAVE_FORMAT_RAW_AAC1:
           		case WAVE_FORMAT_MPEG_ADTS_AAC:
           		#endif
           			if(p_wave_info->wFormatTag == WAVE_FORMAT_MPEGLAYER3) {
           				DEBUG_MSG("AudFormat = MP3\r\n");
	   				} else if(p_wave_info->wFormatTag == WAVE_FORMAT_RAW_AAC1 || 
	   					p_wave_info->wFormatTag == WAVE_FORMAT_MPEG_ADTS_AAC) {	
           				DEBUG_MSG("AudFormat = AAC\r\n");
           			} else if(p_wave_info->wFormatTag == WAVE_FORMAT_PCM) {	
           				DEBUG_MSG("AudFormat = WAVE\r\n");
           			} else if(p_wave_info->wFormatTag == WAVE_FORMAT_ADPCM || 
	   					p_wave_info->wFormatTag == WAVE_FORMAT_DVI_ADPCM) {	
           				DEBUG_MSG("AudFormat = ADPCM\r\n");	
           			} else {
           				DEBUG_MSG("AudFormat = 0x%x\r\n", p_wave_info->wFormatTag);
           			}
           			
	   				vid_dec_set_audio_flag(TRUE);
	   				p_vid_dec_para->aud_frame_size = AUDIO_PCM_BUFFER_SIZE;
	   			#if UP_SAMPLE_EN == 1
	   				p_vid_dec_para->aud_frame_size <<= 1;
	   			#endif	
	   				break;
           			
           		default:
           			DEBUG_MSG("NotSupportAudFormat!!!\r\n");
           			vid_dec_set_audio_flag(FALSE);
           			p_wave_info = NULL;
           			break;
           		}
           		
           		p_vid_dec_para->video_format = vid_dec_get_video_format(p_bitmap_info->biCompression);
       			switch(p_vid_dec_para->video_format)
       			{
       			case C_MJPG_FORMAT:
       				vid_dec_set_video_flag(TRUE);
       				break;
       				
       			#if MPEG4_DECODE_ENABLE == 1
       			case C_XVID_FORMAT:
       			case C_M4S2_FORMAT:
       			case C_H263_FORMAT:
       				if(p_bitmap_info->biWidth%16 != 0 || p_bitmap_info->biWidth > 768) {
       					DEBUG_MSG("Width is not 16-Align or > 768\r\n");
       					vid_dec_set_video_flag(FALSE);
       				} else {
       					vid_dec_set_video_flag(TRUE);
       				}
       				break;
				#endif
				
       			default:
       				vid_dec_set_video_flag(FALSE);
       				p_bitmap_info = NULL;
       				break;
       			}
				
				// no audio and no video
           		if((vid_dec_get_audio_flag() == 0) && (vid_dec_get_video_flag() == 0)) {	
           			vid_dec_parser_end();
           			vid_dec_clear_status(C_VIDEO_DECODE_ALL);	
	       			OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL);
	       			continue;
           		}
				
           		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK);
           	}
           	break; 	
           	
        case MSG_VIDEO_DECODE_PARSER_HEADER_STOP:
			if(p_vid_dec_para->media_handle == 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
				continue;
			}
			
			DEBUG_MSG("MSG_VIDEO_DECODE_PARSER_HEADER_STOP\r\n");
			if(p_vid_dec_para->bResumePlayFlag & C_STORE_STATE) {
        		p_vid_dec_para->bResumePlayFlag &= ~C_STORE_STATE;
        		MultiMediaParser_StoreState(p_vid_dec_para->media_handle);
        	}

           	vid_dec_parser_end();
           	vid_dec_memory_free();
           	vid_dec_clear_status(C_VIDEO_DECODE_PARSER);
           	vid_dec_clear_status(C_VIDEO_DECODE_ALL);
           	OSQFlush(vid_dec_state_q);
           	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 
           	break;
           
        case MSG_VIDEO_DECODE_START:
			if(p_vid_dec_para->media_handle == 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
				continue;
			}
			
			DEBUG_MSG("MSG_VIDEO_DECODE_START\r\n");
			if(p_wave_info) {
       			p_vid_dec_para->n = p_wave_info->nSamplesPerSec*2/TIME_BASE_TICK_RATE; 
       			p_vid_dec_para->tick_time = (INT64S)p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
       			p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2; // about 256ms
       		} else {
       			p_vid_dec_para->n = 1;
       			p_vid_dec_para->tick_time = (INT64S)p_vid_dec_para->VidTickRate * p_vid_dec_para->n;
       			p_vid_dec_para->time_range = p_vid_dec_para->tick_time * TIME_BASE_TICK_RATE >> 2; // about 256ms
       		}
       		p_vid_dec_para->tick_time2 = p_vid_dec_para->tick_time;
           	
        	if(vid_dec_memory_alloc() < 0) {
        		vid_dec_memory_free();
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL);
        		continue; 
        	}	
        	
        	if(vid_dec_get_video_flag()) {
    			if(video_decode_task_start() < 0) {
    				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
    			}		
    			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
    		}
        		
    		if(vid_dec_get_audio_flag()) {
    			if(audio_decode_task_start() < 0) {
    				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
    			}	
    			vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
    		}
    			
        	if(vid_dec_get_video_flag()) {
        		vid_dec_start_timer();
        	}

			#if AUD_DEC_RESTART_EN == 1
			aud_pause = 0;
			#endif
			vid_dec_set_status(C_VIDEO_DECODE_PLAY);
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 
        	break;
        	
        case MSG_VIDEO_DECODE_STOP:
			if(p_vid_dec_para->media_handle == 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
				continue;
			}
			
        	DEBUG_MSG("MSG_VIDEO_DECODE_STOP\r\n");
        	vid_dec_stop_timer();
        	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
        		vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
        		if(video_decode_task_stop() < 0) {
        			OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
        		}
        	}
        		
        	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
        		vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
        		if(audio_decode_task_stop() < 0) {
        			OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
        		}
			}
			#if AUD_DEC_RESTART_EN == 1
            aud_pause = 0;
			#endif
            vid_dec_clear_status(C_VIDEO_DECODE_PLAY);
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;
        
        case MSG_VIDEO_DECODE_PAUSE:
			if(p_vid_dec_para->media_handle == 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
				continue;
			}
			
        	DEBUG_MSG("MSG_VIDEO_DECODE_PAUSE\r\n");        
        	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
        		vid_dec_stop_timer();
        	}
        
        #if AUD_DEC_RESTART_EN == 0
        	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
        		audio_decode_task_pause();	
        	}
        #else
        	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {	
        		aud_pause = 1;
        		vid_dec_set_audio_flag(FALSE);
            	vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
        		audio_decode_task_stop();
        	}
        #endif				
        	
        	vid_dec_set_status(C_VIDEO_DECODE_PAUSE);		
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;
        
        case MSG_VIDEO_DECODE_RESUME:
			if(p_vid_dec_para->media_handle == 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
				continue;
			}
			
        	DEBUG_MSG("MSG_VIDEO_DECODE_RESUME\r\n");
			if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
				vid_dec_start_timer();
			}
			
		#if AUD_DEC_RESTART_EN == 0	
			if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
				audio_decode_task_resume();
			}
		#else
			if(aud_pause) {
				aud_pause = 0;
				vid_dec_set_audio_flag(TRUE);
            	vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
				audio_decode_task_start();
			}
		#endif
			
			vid_dec_clear_status(C_VIDEO_DECODE_PAUSE);	
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;
        	
		case MSG_VIDEO_DECODE_GET_CUR_TIME:
        	if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	
        	DEBUG_MSG("MSG_VIDEO_DECODE_GET_CUR_TIME\r\n");
			p_vid_dec_para->video_cur_time = MultiMediaParser_GetVidCurTime(p_vid_dec_para->media_handle) >> 8;
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;
        	
        case MSG_VIDEO_DECODE_SET_SEEK:
        	if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
			
			DEBUG_MSG("MSG_VIDEO_DECODE_SET_SEEK\r\n");
        	nRet = p_vid_dec_para->video_seek_time << 8;
			nRet = MultiMediaParser_SeekTo(p_vid_dec_para->media_handle, nRet);
			if(nRet < 0) {
				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL);
				continue; 
			}
			
			p_vid_dec_para->video_seek_time = nRet >> 8;	
			DEBUG_MSG("SeekTime = %d sec\r\n", p_vid_dec_para->video_seek_time);	
	   		if(MultiMediaParser_IsEOA(hMedia) == 1) {
				vid_dec_set_audio_flag(FALSE);	
				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
			}

			if(MultiMediaParser_IsEOV(hMedia) == 1) {
				vid_dec_set_video_flag(FALSE);
				vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
			}

			OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK);
			break;
			
		case MSG_VIDEO_DECODE_SET_SEEK_PLAY:
			if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	
        	DEBUG_MSG("MSG_VIDEO_DECODE_SET_SEEK_PLAY\r\n");
        	if(vid_dec_get_video_flag()) {
    			if(video_decode_task_restart() < 0) {
    				OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
    			}		
    			vid_dec_set_status(C_VIDEO_DECODE_PLAYING);
    		}
        		
        	if(vid_dec_get_audio_flag()) {
        		if(audio_decode_task_start() < 0) {
        			OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    				continue;
        		}	
        		vid_dec_set_status(C_AUDIO_DECODE_PLAYING);
        	}	
        	
        	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {		
        		vid_dec_start_timer();
        	}
        	
        	vid_dec_set_status(C_VIDEO_DECODE_PLAY);
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 		
        	break;
        
        case MSG_VIDEO_DECODE_SET_PLAY_SPEED:
        	if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	
        	DEBUG_MSG("MSG_VIDEO_DECODE_SET_PLAY_SPEED\r\n");
        	if(set_play_speed() < 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;
        	
         case MSG_VIDEO_DECODE_SET_REVERSE:
        	if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	
        	DEBUG_MSG("MSG_VIDEO_DECODE_SET_REVERSE\r\n");
        	if(set_play_reverse() < 0)	{
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;	
        	
        case MSG_VIDEO_DECODE_NTH_FRAME:
        	if(p_vid_dec_para->media_handle == 0) {
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
    			continue;
        	}
        	
        	DEBUG_MSG("MSG_VIDEO_DECODE_NTH_FRAME\r\n");
        	if(vid_dec_memory_alloc() < 0) {
        		vid_dec_memory_free();
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
        		continue;
        	}	
        	
        	vid_dec_set_status(C_VIDEO_DECODE_PARSER_NTH);
        	if(video_decode_task_nth_frame() < 0) {
        		vid_dec_clear_status(C_VIDEO_DECODE_PARSER_NTH);
        		OSMboxPost(vid_dec_state_ack_m, (void*)ACK_FAIL); 
        		continue;
        	}
        	
        	vid_dec_clear_status(C_VIDEO_DECODE_PARSER_NTH);
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 	
        	break;	
        
        case MSG_VIDEO_DECODE_VID_END:
        	DEBUG_MSG("MSG_VIDEO_DECODE_VID_END\r\n");
        	if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
       			vid_dec_set_video_flag(FALSE);
		        vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
				video_decode_task_stop();
		    }
		    
		    if(((vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) == 0) && 
		      (vid_dec_get_audio_flag() == 0)) {
            	goto __VidDecFinish;
            }
        	break;
        
        case MSG_VIDEO_DECODE_AUD_END:
        	DEBUG_MSG("MSG_VIDEO_DECODE_AUD_END\r\n");
        	if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
            	vid_dec_set_audio_flag(FALSE);
            	vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
            	audio_decode_task_stop();
            }
            
            if(((vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) == 0) && 
              (vid_dec_get_video_flag() == 0)) {
            	goto __VidDecFinish;
            }   
        	break;
		
		__VidDecFinish:
			vid_dec_parser_end();
           	vid_dec_memory_free();	
            vid_dec_clear_status(C_VIDEO_DECODE_ALL);
       		if(viddec_end) {
       			viddec_end();
       		}
          	break;
			
        case MSG_VIDEO_DECODE_ERROR:
        	DEBUG_MSG("MSG_VIDEO_DECODE_ERROR\r\n");
        	// stop video
        	vid_dec_stop_timer();
			if(vid_dec_get_status() & C_VIDEO_DECODE_PLAYING) {
				vid_dec_set_video_flag(FALSE);
		        vid_dec_clear_status(C_VIDEO_DECODE_PLAYING);
				video_decode_task_stop();
		    }
		    
		    // stop audio
		    if(vid_dec_get_status() & C_AUDIO_DECODE_PLAYING) {
				vid_dec_set_audio_flag(FALSE);
				vid_dec_clear_status(C_AUDIO_DECODE_PLAYING);
            	audio_decode_task_stop();
		  	}
		  	
        	vid_dec_parser_end();
           	vid_dec_memory_free();	
           	video_decode_error_callback();
           	vid_dec_clear_status(C_VIDEO_DECODE_ALL);
           	OSQFlush(vid_dec_state_q);
        	break;
        			 	
        case MSG_VIDEO_DECODE_EXIT:
        	DEBUG_MSG("MSG_VIDEO_DECODE_EXIT\r\n");
        	OSMboxPost(vid_dec_state_ack_m, (void*)ACK_OK); 
   			OSTaskDel(OS_PRIO_SELF);
        	break;
        }  
    }
}

INT32S qtff_decode_thumbnail(CHAR *filename, INT32U Fmt, INT16U width, INT16U height, INT32U decode_addr)
{
	CHAR atom[6] = "ftyp";
	INT16S fd = -1;
	INT32S dec_flag = 0;
	INT32S ret, length, sub_len;
	struct sfn_info fd_info;
	ATOM_HDR_t header;
	mjpeg_info jpeg;
	
	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	// get file format
	sfn_stat(fd, &fd_info);
	ret = vid_dec_get_file_format((INT8S*)fd_info.f_extname);
	if(ret != FILE_TYPE_MOV) {
		ret = STATUS_FAIL;
		goto __exit;
	}
	
	// get file size
	length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	// get thumb atom
	while(length > 0) {
		ret = read(fd, (INT32U)&header, sizeof(ATOM_HDR_t));
		if(ret <= 0) {
			ret = STATUS_FAIL;
			goto __exit;
		}
		
		gp_memcpy((INT8S *)atom, (INT8S *)header.type.s, 4);
		ret = (header.len << 24) & 0xFF000000;
		ret |= (header.len << 8) & 0x00FF0000;
		ret |= (header.len >> 8) & 0x0000FF00;
		ret |= (header.len >> 24) & 0x000000FF;		
		header.len = ret;
		DBG_PRINT("atom=%s, len=0x%x\r\n", atom, header.len);
		
		sub_len = header.len;
		sub_len -= 8;
		if(sub_len < 0) {
			ret = STATUS_FAIL;
			goto __exit;
		}
		
		switch(header.type.v)
		{
		case MAKEFOURCC('f','t','y','p'):
			ret = lseek(fd, sub_len, SEEK_CUR);
			DBG_PRINT("FileOffset = 0x%x\r\n", ret);
			sub_len = 0;	
			break;
		
		case MAKEFOURCC('t','h','u','m'):
			sub_len = (sub_len + 3) >> 2 << 2;
			dec_flag = 1;
			goto __exit;		
			
		case MAKEFOURCC('m','d','a','t'):
			ret = lseek(fd, sub_len, SEEK_CUR);
			DBG_PRINT("FileOffset = 0x%x\r\n", ret);			
			sub_len = 0;
			break;
			
		case MAKEFOURCC('m','o','o','v'):
			ret = lseek(fd, sub_len, SEEK_CUR);
			DBG_PRINT("FileOffset = 0x%x\r\n", ret);
			sub_len = 0;
			break;
			
		default:
			ret = lseek(fd, sub_len, SEEK_CUR);
			DBG_PRINT("FileOffset = 0x%x\r\n", ret);
			sub_len = 0;
			break;	
		}
		
		length -= (INT32S)header.len;
	}
	
__exit:
	if(dec_flag) {
		INT32U raw_data_addr;
		
		raw_data_addr = (INT32U)gp_malloc_align(sub_len, 32);
		if(raw_data_addr == 0) {
			ret = STATUS_FAIL;
			goto __exit;
		}

		ret = read(fd, (INT32U)raw_data_addr, sub_len);
		if(ret <= 0) {
			ret = STATUS_FAIL;
			goto __exit;
		}

		jpeg.scaler_mode = C_SCALER_FULL_SCREEN;
		jpeg.raw_data_addr = raw_data_addr;
		jpeg.raw_data_size = sub_len;
		jpeg.jpeg_yuv_mode = C_JPG_CTRL_YUV422;
		jpeg.jpeg_valid_w = width;
		jpeg.jpeg_valid_h = height;
		jpeg.output_addr = decode_addr;
		jpeg.output_format = Fmt; //C_SCALER_CTRL_OUT_YUYV;
		jpeg.jpeg_output_w = width;
		jpeg.jpeg_output_h = height;
		jpeg.jpeg_output_buffer_w = width;
		jpeg.jpeg_output_buffer_h = height; 
		jpeg.boundary_color = 0x008080;	
		ret = mjpeg_decode_and_scaler(&jpeg);
		gp_free((void *)raw_data_addr);		
	}

	if(fd >= 0) {
		close(fd);
	}
	return ret;
}
