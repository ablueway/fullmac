#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__

#include "driver_l2.h"

extern void drv_l2_system_timer_init(void);
extern void drv_l2_system_timer_uninit(void);
extern INT32S drv_l2_system_timer_registe_isr(void (*TimerIsrFunction)(void));
extern INT32S drv_l2_system_timer_release_isr(void (*TimerIsrFunction)(void));

#endif		// __SYSTEM_TIMER_H__
