#ifndef __GP_TIMER_H__
#define __GP_TIMER_H__

#include "driver_l1.h"

extern INT32U sw_timer_init(INT32U TimerId, INT32U freq_hz);
extern INT32U sw_timer_get_counter_L(void);
extern INT32U sw_timer_get_counter_H(void);
extern INT64U sw_timer_get_counter(void);
#endif
