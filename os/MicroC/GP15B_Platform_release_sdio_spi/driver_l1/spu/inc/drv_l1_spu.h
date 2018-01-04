#ifndef _DRV_L1_SPU_H_
#define _DRV_L1_SPU_H_

#include "driver_l1.h"

#if 0
	#define DBG_LEVEL_1		DBG_PRINT
#else
	#define DBG_LEVEL_1(...)
#endif

#if 0
	#define DBG_LEVEL_2		DBG_PRINT
#else
	#define DBG_LEVEL_2(...)
#endif

#define SPU_MIDI_SPEECH_TOGETHER		1

#if SPU_MIDI_SPEECH_TOGETHER
	#define MIDI_CH_START	0
	#define MIDI_CH_TOTAL	6
	#define SPEECH_CH_START	6
	#define SPEECH_CH_TOTAL	2
#else
	//Midi only, speech off
	#define MIDI_CH_START	0
	#define MIDI_CH_TOTAL	8
	#define SPEECH_CH_START	0
	#define SPEECH_CH_TOTAL	0
#endif

#define SPU_PCM_FROM_DECODER	1	//PCM from other decoder, and play on spu pcm channel, this PCM CANNOT play with midi at the same time

#define GP15B_MINI_SPU_EN		1
#endif

