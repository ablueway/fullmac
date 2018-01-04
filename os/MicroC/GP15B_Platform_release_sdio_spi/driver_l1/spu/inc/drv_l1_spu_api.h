#ifndef _DRV_L1_SPU_API_H_
#define _DRV_L1_SPU_API_H_

#include "driver_l1.h"

extern void SPU_SetVol(INT8U volume);
extern void SPU_SetMute(void);
extern void SPU_SetChMixer(INT8U spu_ch, INT8U stereo_ch, INT8U enable);
extern void SPU_SetOutputToI2S(INT32U enable);
#endif

