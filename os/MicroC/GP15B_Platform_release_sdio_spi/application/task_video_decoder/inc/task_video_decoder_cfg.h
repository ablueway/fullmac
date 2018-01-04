#ifndef __TASK_VIDEO_DECODER_CFG_H__
#define __TASK_VIDEO_DECODER_CFG_H__

#define VIDEO_DECODE_TIMER			TIMER_C
#define VIDEO_FRAME_NO				3	//video frame number
#define DEBLOCK_FRAME_NO			3	//deblock frame number for mpeg4 use
#define SCALER_FRAME_NO				3 	//scaler frame number for mpeg4 use
#define AUDIO_FRAME_NO				3	//audio frame number

#define TIME_BASE_TICK_RATE			100	//video timer frequency
#define DAC_RAMP_DOWN_STEP			128
#define DAC_RAMP_DOWN_STEP_HOLD 	4

// mpeg4 configure
#define MPEG4_DECODE_ENABLE			0	//fixed 0
#define S263_DECODE_ENABLE			0	//fix 0: disable
#define ASP_ENABLE					0	//advance simple profile

// audio decode configure
#if APP_AAC_DECODE_EN == 1
	#define AAC_DECODE_ENABLE		1	//0: disable, 1:enable	
#else
	#define AAC_DECODE_ENABLE		0	//fix 0: disable
#endif
#if APP_MP3_DECODE_EN == 1
	#define MP3_DECODE_ENABLE		1	//0: disable, 1:enable
#else
	#define MP3_DECODE_ENABLE		0	//fix 0: disable
#endif

#define AUD_DEC_RESTART_EN			0	//pause/resume audio method. 0: disable, 1:enable
#define VOICE_CHANGER_EN			0	//fix 0: disable
#if APP_UP_SAMPLE_EN == 1
	#define UP_SAMPLE_EN			0	//0: disable, 1:enable 
#else
	#define UP_SAMPLE_EN			0	//fix 0: disable
#endif

// parser configure
#define PARSER_AUD_BITSTREAM_SIZE	8192  	//unit: byte
#define PARSER_VID_BITSTREAM_SIZE	16384 	//unit: byte
#define AUDIO_PCM_BUFFER_SIZE		4096<<1 //unit: byte

#define PARSER_DROP					0x00
#define DECODE_DROP					0x01
#define DROP_MENTHOD				PARSER_DROP
#endif
