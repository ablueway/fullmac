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
#include "drv_l2_system_timer.h"
#include "drv_l1_timer.h"

#if (defined _DRV_L2_SYSTEM_TIMER) && (_DRV_L2_SYSTEM_TIMER == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MAX_SYSTEM_TIMER_ISR	4
#define TIMER_BASE_USE			TIME_BASE_C
#define TIMER_BASE_HZ			TMBCS_128HZ

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void TimeBase_ISR(void);
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U gstSysTimerEnable;
static void (*gTimeBaseC_ISR[MAX_SYSTEM_TIMER_ISR])();

/**
 * @brief   system timer initialize  
 * @param   none
 * @return 	none
 */
void drv_l2_system_timer_init(void)
{
	INT32U i;

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++) {
		gTimeBaseC_ISR[i] = NULL;
	}
	
	gstSysTimerEnable = 0;
}

/**
 * @brief   system timer un-initialize  
 * @param   none
 * @return 	none
 */
void drv_l2_system_timer_uninit(void)
{
	INT32U i;

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++) {
		gTimeBaseC_ISR[i] = NULL;
	}
	
	gstSysTimerEnable = 0;
	time_base_stop(TIMER_BASE_USE);
}

/**
 * @brief   system timer register isr function  
 * @param   TimerIsrFunction[in]: system timer isr function
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_system_timer_registe_isr(void (*TimerIsrFunction)(void))
{
	INT32S	i;

	if(gstSysTimerEnable == 0) {
		time_base_setup(TIMER_BASE_USE, TIMER_BASE_HZ, 0, TimeBase_ISR);
	}

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++) {
		if(gTimeBaseC_ISR[i] == TimerIsrFunction) {
			return STATUS_OK;
		}
		
		if(gTimeBaseC_ISR[i] == NULL) {
			gTimeBaseC_ISR[i] = TimerIsrFunction;
			gstSysTimerEnable |= (1 << i);
			return STATUS_OK;
		}		
	}
	
	return STATUS_FAIL;
}

/**
 * @brief   system timer unregister isr function  
 * @param   TimerIsrFunction[in]: system timer isr function
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l2_system_timer_release_isr(void (*TimerIsrFunction)(void))
{
	INT32S	i;

	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++)
	{
		if(gTimeBaseC_ISR[i] == TimerIsrFunction) {
			gTimeBaseC_ISR[i] = NULL;
			gstSysTimerEnable &= ~(1 << i);
			
			if(gstSysTimerEnable == 0) {
				time_base_stop(TIMER_BASE_USE);
			}
			
			return STATUS_OK;
		}
	}
	
	return STATUS_FAIL;
}

static void TimeBase_ISR(void)
{
	INT32S i;
	
	for(i = 0; i < MAX_SYSTEM_TIMER_ISR; i++) {
		if((gstSysTimerEnable & (1 << i)) == 0) {
			continue;
		}
		
		if(gTimeBaseC_ISR[i] != NULL) {
			gTimeBaseC_ISR[i]();
		}
	}

	if(gstSysTimerEnable == 0) {
		time_base_stop(TIMER_BASE_USE);
		return;
	}
}
#endif