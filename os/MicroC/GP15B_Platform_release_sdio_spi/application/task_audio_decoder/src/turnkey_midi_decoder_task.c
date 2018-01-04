#include <string.h>
#include "drv_l1_spu.h"
#include "drv_l1_spu_api.h"
#include "drv_l1_spu_midi.h"
#include "drv_l1_spu_speech.h"
#include "turnkey_audio_decoder_task.h"
#include "turnkey_audio_dac_task.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MIDI_QUEUE_MAX			5
#define MidiTaskStackSize		512

#define PARSER_STOP				0
#define PARSER_START			1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
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
	MSG_MIDI_PLAY,
	MSG_MIDI_PAUSE,
	MSG_MIDI_RESUME,
	MSG_MIDI_STOP,
	MSG_MIDI_SET_VOLUME,
	MSG_MIDI_END,
	MSG_MIDI_EXIT,
	MSG_MIDI_MAX
} MSG_MIDI_ENUM;

typedef struct MidiDecCtrl_s
{
	OS_EVENT *MidiTaskQ;
	OS_EVENT *MidiTaskM;
	void *midi_q_buf[MIDI_QUEUE_MAX];
	INT32U task_stack[MidiTaskStackSize];
	
	INT32S fd;
	INT32U source_type;
	INT32U audio_format;		// audio format	
	
	INT8U  index;
	INT8U  total_index;
	INT8U  task_running;
	INT8U  volume;

	INT32U midi_res_addr;
	INT32U parser_state;
	INT32U state;
} MidiDecCtrl_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void midi_task_entry(void *p_arg);
static INT32S midi_decode_send_msg(MidiDecCtrl_t *pAudDec, INT32U message);
static void midi_end(void);
extern void SPU_Init(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static MidiDecCtrl_t *g_midi_ctrl;


void *MidiDecOpen(void)
{
	INT8U error;
	MidiDecCtrl_t *pMidiDec;

	pMidiDec = (MidiDecCtrl_t *)gp_malloc_align(sizeof(MidiDecCtrl_t), 4);
	if(pMidiDec == 0) {
		return 0;
	}

	MSG("midi_dec_workmem = 0x%x\r\n", pMidiDec);
	gp_memset((INT8S *)pMidiDec, 0x00, sizeof(MidiDecCtrl_t));

	pMidiDec->volume = 63;
	pMidiDec->parser_state = PARSER_STOP;
	pMidiDec->state = AUDIO_PLAY_STOP;

	// queue create
	pMidiDec->MidiTaskQ = OSQCreate(pMidiDec->midi_q_buf, MIDI_QUEUE_MAX);
	if(pMidiDec->MidiTaskQ == 0) {
		goto __fail;
	}
	
	pMidiDec->MidiTaskM = OSMboxCreate(NULL);
	if(pMidiDec->MidiTaskM == 0) {
		goto __fail;
	}
	
	g_midi_ctrl = pMidiDec;
	error = OSTaskCreate(midi_task_entry, (void *)pMidiDec, &pMidiDec->task_stack[MidiTaskStackSize - 1], MIDI_PRIORITY); 
	if(error != OS_NO_ERR) {
		goto __fail;
	}
	
	pMidiDec->task_running = 1;
	return (void *)pMidiDec;
	
__fail:
	MidiDecClose((void *)pMidiDec);
	return 0;	
}

void MidiDecClose(void* workmem)
{
	INT8U err;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;

	if(pMidiDec->task_running) {
		midi_decode_send_msg(pMidiDec, MSG_MIDI_STOP);
		midi_decode_send_msg(pMidiDec, MSG_MIDI_EXIT);
	}
	
	if(pMidiDec->MidiTaskM) {
 		OSMboxDel(pMidiDec->MidiTaskM, OS_DEL_ALWAYS, &err);
 	}
 	
 	if(pMidiDec->MidiTaskQ) {
 		OSQDel(pMidiDec->MidiTaskQ, OS_DEL_ALWAYS, &err);
 	}
	
	if(workmem) {
		gp_free((void *)workmem);
	}
}

INT32U MidiDecParserStart(void *workmem, INT32U source_type, INT32S fd, INT32U midi_res_addr)
{
	INT32S ret, size;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	if(pMidiDec->parser_state == PARSER_START) {
		return STATUS_FAIL;
	}
	
	if(source_type == AUDIO_SRC_TYPE_FS) {
		pMidiDec->source_type = AUDIO_SRC_TYPE_FS;
		pMidiDec->fd = fd;
		pMidiDec->midi_res_addr = 0;
		
		ret = lseek(fd, 0, SEEK_CUR);
		size = lseek(fd, 0, SEEK_END);
		
		lseek(fd, ret, SEEK_SET);
		size -= ret;
			
		pMidiDec->midi_res_addr = (INT32U)gp_malloc_align(size, 4);
		if(pMidiDec->midi_res_addr == 0) {
			return STATUS_FAIL;
		}
			
		ret = read(fd, pMidiDec->midi_res_addr, size);
		if(ret != size) {
			gp_free((void *)pMidiDec->midi_res_addr);
			pMidiDec->midi_res_addr = 0;
			return STATUS_FAIL;
		}
	} else {
		pMidiDec->source_type = AUDIO_SRC_TYPE_USER_DEFINE;	
		pMidiDec->fd = 0;
		pMidiDec->midi_res_addr = midi_res_addr;
	}
	
	pMidiDec->total_index = SPU_MIDI_GetGMDMidiNums((INT32U *)pMidiDec->midi_res_addr);
	MSG("TotalMidi = %d\r\n", pMidiDec->total_index);
	pMidiDec->parser_state = PARSER_START;
	return STATUS_OK;
}

INT32S MidiDecParserStop(void *workmem)
{
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	if((pMidiDec->source_type == AUDIO_SRC_TYPE_FS) && pMidiDec->midi_res_addr) {
		gp_free((void *)pMidiDec->midi_res_addr);
	}
	
	pMidiDec->midi_res_addr = 0;
	pMidiDec->parser_state = PARSER_STOP;
	return STATUS_OK;
}

INT32S MidiDecStart(void *workmem, INT32U midi_index)
{
	INT32S ret;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;

	if(pMidiDec->parser_state == PARSER_STOP) {
		return STATUS_FAIL;
	} 
	
	if(pMidiDec->state != AUDIO_PLAY_STOP) {
		return STATUS_FAIL;
	} 
	
	pMidiDec->index = midi_index;
	ret = midi_decode_send_msg(pMidiDec, MSG_MIDI_PLAY);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S MidiDecPause(void *workmem)
{
	INT32S ret;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	if(pMidiDec->parser_state == PARSER_STOP) {
		return STATUS_FAIL;
	} 
	
	if(pMidiDec->state != AUDIO_PLAYING) {
		return STATUS_FAIL;
	} 
	
	ret = midi_decode_send_msg(pMidiDec, MSG_MIDI_PAUSE);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S MidiDecResume(void *workmem)
{
	INT32S ret;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	if(pMidiDec->parser_state == PARSER_STOP) {
		return STATUS_FAIL;
	} 
	
	if(pMidiDec->state != AUDIO_PLAY_PAUSE) {
		return STATUS_FAIL;
	} 
	
	ret = midi_decode_send_msg(pMidiDec, MSG_MIDI_RESUME);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S MidiDecStop(void *workmem)
{
	INT32S ret;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	if(pMidiDec->parser_state == PARSER_STOP) {
		return STATUS_FAIL;
	} 
	
	if(pMidiDec->state != AUDIO_PLAYING) {
		return STATUS_FAIL;
	} 
	
	ret = midi_decode_send_msg(pMidiDec, MSG_MIDI_STOP);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S MidiDecSetVolume(void *workmem, INT32U Volume)
{
	INT32S ret;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	
	pMidiDec->volume = Volume;
	ret = midi_decode_send_msg(pMidiDec, MSG_MIDI_SET_VOLUME);
	if(ret < 0) {
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

INT32S MidiDecGetStatus(void *workmem)
{
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)workmem;
	return pMidiDec->state;
}

static void midi_task_entry(void *p_arg)
{
	INT8U err;
    INT32U msg_id;
	MidiDecCtrl_t *pMidiDec = (MidiDecCtrl_t *)p_arg;
	OS_EVENT *AudioTaskQ = pMidiDec->MidiTaskQ;
	OS_EVENT *AudioTaskM = pMidiDec->MidiTaskM;

	SPU_Init();
	SPU_MIDI_Register_Callback(midi_end);
	SPU_SetVol(64);
	
	while (1)
	{
		msg_id = (INT32U)OSQPend(AudioTaskQ, 0, &err);
		if(err != OS_NO_ERR) {
			continue;
		}
		
		switch(msg_id) 
		{
		case MSG_MIDI_PLAY:
			MSG("MSG_MIDI_PLAY\r\n");
			if(pMidiDec->index >= pMidiDec->total_index) {
				OSMboxPost(AudioTaskM, (void *)ACK_FAIL);
				continue;
			}
	
			SPU_MIDI_Play((INT32U *)pMidiDec->midi_res_addr, pMidiDec->index);
			pMidiDec->state = AUDIO_PLAYING;
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;
			
		case MSG_MIDI_PAUSE:
			MSG("MSG_MIDI_PAUSE\r\n");
			SPU_MIDI_Pause();
			pMidiDec->state = AUDIO_PLAY_PAUSE;
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;	
			
		case MSG_MIDI_RESUME:
			MSG("MSG_MIDI_RESUME\r\n");
			SPU_MIDI_Resume();
			pMidiDec->state = AUDIO_PLAYING;
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;
				
		case MSG_MIDI_STOP:
			MSG("MSG_MIDI_STOP\r\n");
			SPU_MIDI_Stop();
			if((pMidiDec->source_type == AUDIO_SRC_TYPE_FS) && pMidiDec->midi_res_addr) {
				gp_free((void *)pMidiDec->midi_res_addr);
			}
			
			pMidiDec->midi_res_addr = 0;
			pMidiDec->state = AUDIO_PLAY_STOP;
			pMidiDec->parser_state = PARSER_STOP;
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;	
		
		case MSG_MIDI_SET_VOLUME:
			MSG("MSG_MIDI_SET_VOLUME\r\n");
			if(pMidiDec->volume > 63) {
				pMidiDec->volume = 63;
			}
			
			SPU_SetVol(pMidiDec->volume << 1);
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			break;
		
		case MSG_MIDI_END:
			MSG("MSG_MIDI_END\r\n");
			SPU_MIDI_Stop();
			if((pMidiDec->source_type == AUDIO_SRC_TYPE_FS) && pMidiDec->midi_res_addr) {
				gp_free((void *)pMidiDec->midi_res_addr);
			}
			
			pMidiDec->midi_res_addr = 0;
			pMidiDec->state = AUDIO_PLAY_STOP;
			pMidiDec->parser_state = PARSER_STOP;
			break;
			
		case MSG_MIDI_EXIT:
			MSG("MSG_MIDI_EXIT\r\n");
			OSMboxPost(AudioTaskM, (void *)ACK_OK);
			OSTaskDel(OS_PRIO_SELF);
			break;
			
		default:
			break;		
		}
	}
	while(1);
}

static INT32S midi_decode_send_msg(MidiDecCtrl_t *pMidiDec, INT32U message)
{
	INT8U err;
	INT32S ack_msg;
	
	// clear ack mbox
	OSMboxAccept(pMidiDec->MidiTaskM);
	
	// post message
	err = OSQPost(pMidiDec->MidiTaskQ, (void *)message);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	// wait ack mbox
	ack_msg = (INT32S)OSMboxPend(pMidiDec->MidiTaskM, 5000, &err);
	if(err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
	
	return ack_msg;
}

static void midi_end(void)
{
	OSQPost(g_midi_ctrl->MidiTaskQ, (void *)MSG_MIDI_END);
	
	if(auddec_end) {
		auddec_end(AUDIO_CHANNEL_MIDI);
	}
}
