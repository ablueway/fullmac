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
//Include files
#include "driver_l1.h"
#include "drv_l1_adc.h"
#include "drv_l1_dac.h"
#include "drv_l1_uart.h"
#include "drv_l1_cache.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_dma.h"
#include "drv_l1_gpio.h"
#include "drv_l1_rtc.h"

#if _DRV_L1_TV == 1
extern void system_clk_ext_XLAT_12M(void);
#endif

void drv_l1_init(void)
{
#if _DRV_L1_CACHE == 1
	cache_init();						// Initiate CPU Cache
#endif

	exception_table_init();				// Initiate CPU exception handler

#if _DRV_L1_TV == 1
	system_clk_ext_XLAT_12M();
#endif

#if _DRV_L1_INTERRUPT == 1
	vic_init();							// Initiate Interrupt controller
#endif

#if _DRV_L1_UART0 == 1
	drv_l1_uart0_init();
#endif

#if _DRV_L1_UART1 == 1
	drv_l1_uart1_init();
#endif

#if _DRV_L1_TIMER==1
	timer_init();						// Initiate Timer
	timerD_counter_init();              // Tiny Counter Initial (1 tiny count == 2.67 us)
#endif

#if _DRV_L1_DMA == 1
	drv_l1_dma_init();					// Initiate DMA controller
#endif

#if _DRV_L1_DAC == 1
	drv_l1_dac_init();
#endif

#if _DRV_L1_ADC == 1
	drv_l1_adc_init();
#endif

#if _DRV_L1_RTC == 1
	drv_l1_rtc_init();
#endif
}
