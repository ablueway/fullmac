#ifndef __AUDIO_DECODER_TASK_H__
#define __AUDIO_DECODER_TASK_H__
#include "application.h"

// audio decode state
#define AUDIO_PLAY_STOP			0
#define AUDIO_PLAYING			1
#define AUDIO_PLAY_PAUSE		2

typedef struct ParserInfo_s
{
	INT32S fd;					// file handle
	INT32U source_type;			// audio format
	INT32U audio_format;		// audio format	
	INT8U  *data_start_addr;	// data source address 
	INT32U data_offset;			// data offset
	INT32U file_len;			// total file size 
	
	// get back information
	INT32U channel_no;			// channel number
	INT32U sample_rate;			// samle rate
	INT32U bit_rate;			// bit rate
	INT32U duration;			// total time in second
} ParserInfo_t;

// callback function
extern void (*auddec_end)(INT32U main_ch);
extern INT32S (*auddec_user_read)(INT32U main_ch, INT32U buf_addr, INT32U buf_size, INT8U *data_start_addr, INT32U data_offset);

// api function
extern void *AudDecOpen(INT32U main_ch);
extern void AudDecClose(void *workmem);
extern INT32S AudDecParserStart(void *workmem, ParserInfo_t *pInfo);
extern INT32S AudDecParserStop(void *workmem);
extern INT32S AudDecStart(void *workmem);
extern INT32S AudDecPause(void *workmem);
extern INT32S AudDecResume(void *workmem);
extern INT32S AudDecStop(void *workmem);
extern INT32S AudDecSeekPlay(void *workmem, INT32U msec);
extern INT32S AudDecSetVolume(void *workmem, INT32U Volume);
extern INT32S AudDecGetCurTime(void *workmem);
extern INT32S AudDecGetTotalTime(void *workmem);
extern INT32S AudDecGetStatus(void *workmem);

//mipi api
extern void *MidiDecOpen(void);
extern void MidiDecClose(void *workmem);
extern INT32U MidiDecParserStart(void *workmem, INT32U source_type, INT32S fd, INT32U midi_res_addr);
extern INT32S MidiDecParserStop(void *workmem);
extern INT32S MidiDecStart(void *workmem, INT32U midi_index);
extern INT32S MidiDecPause(void *workmem);
extern INT32S MidiDecResume(void *workmem);
extern INT32S MidiDecStop(void *workmem);
extern INT32S MidiDecSetVolume(void *workmem, INT32U Volume);
extern INT32S MidiDecGetStatus(void *workmem);
#endif 		// __AUDIO_DECODER_TASK_H__

