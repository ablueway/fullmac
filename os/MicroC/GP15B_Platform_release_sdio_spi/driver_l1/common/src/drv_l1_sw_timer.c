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
#include "drv_l1_sw_timer.h"
#include "drv_l1_sfr.h"
#include "drv_l1_rtc.h"
#include "drv_l1_timer.h"

#if (defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT64U sw_timer_count = 0;


static void sw_timer_isr(void)
{
	sw_timer_count++;
}

INT32U sw_timer_init(INT32U TimerId, INT32U freq_hz)
{
    sw_timer_count = 0;
    if (TimerId < 4) {
        timer_freq_setup(TimerId, freq_hz, 0, sw_timer_isr);
    } else {
    	drv_l1_rtc_callback_clear(RTC_SCH_INT_INDEX);
        drv_l1_rtc_callback_set(RTC_SCH_INT_INDEX, sw_timer_isr);
       	drv_l1_rtc_schedule_enable(ENABLE, RTC_SCH_1024HZ);
    }  
       
    return STATUS_OK;
}

INT32U sw_timer_get_counter_L(void)
{
	return (sw_timer_count & 0xFFFFFFFF);
}

INT32U sw_timer_get_counter_H(void)
{
	return (sw_timer_count >> 32);
}

INT64U sw_timer_get_counter(void)
{
	return sw_timer_count;
}
#endif //(defined _DRV_L1_SW_TIMER) && (_DRV_L1_SW_TIMER == 1)    
