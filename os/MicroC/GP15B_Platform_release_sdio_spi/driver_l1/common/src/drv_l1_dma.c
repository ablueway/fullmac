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
#include "drv_l1_dma.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_cache.h"
#if _OPERATING_SYSTEM != _OS_NONE
#include "os.h"
#endif

#if (defined _DRV_L1_DMA) && (_DRV_L1_DMA == 1)                   
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32U dma_get_channel(INT8U usage);
///static INT32S dma_set_channel(INT8U usage, INT8U channel);
static void dma_free_channel(INT32U channel);

#if _OPERATING_SYSTEM != _OS_NONE
static INT32U dma_get_queue(void);
static void dma_free_queue(INT32U q_index);
#endif

void dma_set_notify(INT32U channel, INT32U notify, INT32U os_q);
static INT32S dma_device_protect(void);
static void dma_device_unprotect(INT32S mask);
static void dma_isr(void);

static void dma_set_control(INT32U channel, INT32U ctrl);
static void dma_set_source(INT32U channel, INT32U addr);
static void dma_set_target(INT32U channel, INT32U addr);
static INT32S dma_set_tx_count(INT32U channel, INT32U count);
static INT32S dma_set_device(INT32U channel, INT16U device);
static INT32S dma_set_timeout(INT32U channel, INT32U timeout);
static INT32S dma_set_line_length(INT32U length);
static INT32S dma_set_sprite_size(INT32U channel, INT32U size);

#if 0
static INT32S dma_set_transparent_enable(INT32U channel);
static INT32S dma_set_transparent_disable(INT32U channel);
static INT32S dma_set_transparent_pattern(INT32U channel, INT16U pattern);
#endif

static INT32S dma_transfer_extend(DMA_STRUCT *dma_struct, INT8U usage, INT32U os_q);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT8U dma_init_done = FALSE;
static INT8U dma_usage[C_DMA_CHANNEL_NUM];
static INT32U dma_notify_variable[C_DMA_CHANNEL_NUM];

#if _OPERATING_SYSTEM != _OS_NONE
static INT32U dma_notify_queue[C_DMA_CHANNEL_NUM];
static void *q_buffer[C_DMA_Q_NUM][C_DMA_Q_BUF_SIZE];
static OS_EVENT *dma_driver_queue[C_DMA_Q_NUM] = {NULL};
static INT8U dma_q_usage[C_DMA_Q_NUM];
#endif

/**
 * @brief   get dma idle channel
 * @param   usage: use method
 * @return 	channel
 */
static INT32U dma_get_channel(INT8U usage)
{
	INT32U i;
	INT32S mask;

#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	OSSchedLock();
#endif
	mask = dma_device_protect();			// Device Protect for critical section

	for (i=0; i<C_DMA_CHANNEL_NUM; i++) {
		if (dma_usage[i] == C_DMA_NOT_UESED) {
			dma_usage[i] = usage;

			dma_device_unprotect(mask);
		#if _OPERATING_SYSTEM != _OS_NONE
			OSSchedUnlock();
		#endif

			// Reset DMA controller
			dma_set_control(i, C_DMA_CTRL_RESET);
			return i;
		}
	}

	dma_device_unprotect(mask);
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif

	return i;
}

/**
 * @brief   get dma assign channel
 * @param   usage: use method
 * @param   channel: assign channel
 * @return 	result: >=0 is success, <0 is fail.
 */
/*static INT32S dma_set_channel(INT8U usage, INT8U channel)
{
	INT32S ret, mask;

	if(channel >= C_DMA_CHANNEL_NUM) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	OSSchedLock();
#endif
	mask = dma_device_protect();			// Device Protect for critical section

	if (dma_usage[channel] == C_DMA_NOT_UESED) {
		dma_usage[channel] = usage;
		dma_set_control(channel, C_DMA_CTRL_RESET); 	// Reset DMA controller
		ret = STATUS_OK;
	} else {
		ret = STATUS_FAIL;
	}

	dma_device_unprotect(mask);
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif
	return ret;
}*/

/**
 * @brief   free dma unsued channel
 * @param   channel: channel
 * @return 	none
 */
static void dma_free_channel(INT32U channel)
{
	if (channel >= C_DMA_CHANNEL_NUM) {
		return;
	}

	// Reset DMA controller
	dma_set_control(channel, C_DMA_CTRL_RESET);

	// Clear pending bit
	if (channel == C_DMA_CH0) {
		R_DMA_INT = C_DMA0_INT_PEND;
	} else if (channel == C_DMA_CH1) {
		R_DMA_INT = C_DMA1_INT_PEND;
	} else if (channel == C_DMA_CH2) {
		R_DMA_INT = C_DMA2_INT_PEND;
	} else if (channel == C_DMA_CH3) {
		R_DMA_INT = C_DMA3_INT_PEND;
	} else if (channel == C_DMA_CH4) {
		R_DMA_INT = C_DMA4_INT_PEND;
	} else if (channel == C_DMA_CH5) {
		R_DMA_INT = C_DMA5_INT_PEND;
	} else if (channel == C_DMA_CH6) {
		R_DMA_INT = C_DMA6_INT_PEND;
	} else if (channel == C_DMA_CH7) {
		R_DMA_INT = C_DMA7_INT_PEND;
	}

	dma_notify_variable[channel] = (INT32U) NULL;
#if _OPERATING_SYSTEM != _OS_NONE
  	dma_notify_queue[channel] = (INT32U) NULL;
#endif

	dma_usage[channel] = (INT8U) C_DMA_NOT_UESED;
}

#if _OPERATING_SYSTEM != _OS_NONE
/**
 * @brief   get and set dma channel queue
 * @param   none
 * @return 	queue number
 */
static INT32U dma_get_queue(void)
{
	INT32U i;

	OSSchedLock();
	for (i=0; i<C_DMA_Q_NUM; i++) {
		if (!dma_q_usage[i]) {
			dma_q_usage[i] = (INT8U) TRUE;

			OSSchedUnlock();

			if (dma_driver_queue[i]) {
				OSQFlush(dma_driver_queue[i]);
			} else {
				dma_driver_queue[i] = OSQCreate(&q_buffer[i][0], C_DMA_Q_BUF_SIZE);
			}
			return i;
		}
	}
	OSSchedUnlock();
	return i;
}

/**
 * @brief   free dma channel queue
 * @param   q_index: queue number
 * @return 	none
 */
static void dma_free_queue(INT32U q_index)
{
	if (q_index >= C_DMA_Q_NUM) {
		return;
	}

	dma_q_usage[q_index] = (INT8U) FALSE;
}
#endif

/**
 * @brief   set dma done notify 
 * @param   channel: channel number
 * @param   notify: notify point
 * @param   os_q: notify queue
 * @return 	none
 */
void dma_set_notify(INT32U channel, INT32U notify, INT32U os_q)
{
	dma_notify_variable[channel] = notify;
#if _OPERATING_SYSTEM != _OS_NONE
  	dma_notify_queue[channel] = os_q;
#else
  	os_q = os_q;							// Prevent compiler warning
#endif
}

/**
 * @brief   disable dma irq
 * @param   none
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_device_protect(void)
{
	return vic_irq_disable(VIC_DMA);
}

/**
 * @brief   enable/disable dma irq
 * @param   mask: mask value
 * @return 	none
 */
static void dma_device_unprotect(INT32S mask)
{
	if (mask == 0) {						// Clear device interrupt mask
		vic_irq_enable(VIC_DMA);
	} else if (mask == 1) {
		vic_irq_disable(VIC_DMA);
	} else {								// Something is wrong, do nothing
		return;
	}
}

static void handle_dma_timout(INT32S channel) 
{
	if (dma_notify_variable[channel]) {
		*((INT8S *) dma_notify_variable[channel]) = (INT8S) C_DMA_STATUS_TIMEOUT;
	}

#if _OPERATING_SYSTEM != _OS_NONE
	if (dma_notify_queue[channel]) {
		OSQPost((OS_EVENT *) dma_notify_queue[channel], (void *) C_DMA_STATUS_TIMEOUT);
	}
#endif
}

static void handle_dma_done(INT32S channel) 
{
	if (dma_notify_variable[channel]) {
		*((INT8S *) dma_notify_variable[channel]) = (INT8S) C_DMA_STATUS_DONE;
	}
	
#if _OPERATING_SYSTEM != _OS_NONE
	if (dma_notify_queue[channel]) {
		OSQPost((OS_EVENT *) dma_notify_queue[channel], (void *) C_DMA_STATUS_DONE);
	}
#endif
}

static void dma_isr(void)							// Device ISR
{
	INT32U pending;

	pending = R_DMA_INT;
	// DMA0
	if (pending & C_DMA0_INT_PEND) {		// DMA0 interrupt is pending
		if (dma_usage[C_DMA_CH0] != C_DMA_NOT_UESED) {
			if (pending & C_DMA0_TIMEOUT) {		
				handle_dma_timout(C_DMA_CH0);
			} else {
				handle_dma_done(C_DMA_CH0);
			}
			
			if (dma_usage[C_DMA_CH0] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH0);
			}
		}
		R_DMA_INT = C_DMA0_INT_PEND;		// Clear pending bit
	}

	// DMA1
	if (pending & C_DMA1_INT_PEND) {		// DMA1 interrupt is pending
		if (dma_usage[C_DMA_CH1] != C_DMA_NOT_UESED) {
			if (pending & C_DMA1_TIMEOUT) {		
				handle_dma_timout(C_DMA_CH1);
			} else {
				handle_dma_done(C_DMA_CH1);
			}
			
			if (dma_usage[C_DMA_CH1] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH1);
			}
		}
		R_DMA_INT = C_DMA1_INT_PEND;		// Clear pending bit
	}

	// DMA2
	if (pending & C_DMA2_INT_PEND) {		// DMA2 interrupt is pending
		if (dma_usage[C_DMA_CH2] != C_DMA_NOT_UESED) {
			if (pending & C_DMA2_TIMEOUT) {		
				handle_dma_timout(C_DMA_CH2);
			} else {
				handle_dma_done(C_DMA_CH2);
			}
			
			if (dma_usage[C_DMA_CH2] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH2);
			}
		}
		R_DMA_INT = C_DMA2_INT_PEND;		// Clear pending bit
	}

	// DMA3
	if (pending & C_DMA3_INT_PEND) {		// DMA3 interrupt is pending
		if (dma_usage[C_DMA_CH3] != C_DMA_NOT_UESED) {
			if (pending & C_DMA3_TIMEOUT) {		
				handle_dma_timout(C_DMA_CH3);
			} else {
				handle_dma_done(C_DMA_CH3);
			}
			
			if (dma_usage[C_DMA_CH3] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH3);
			}
		}
		R_DMA_INT = C_DMA3_INT_PEND;		// Clear pending bit
	}

	// DMA4
	if (pending & C_DMA4_INT_PEND) {		// DMA4 interrupt is pending
		if (dma_usage[C_DMA_CH4] != C_DMA_NOT_UESED) {
			if (pending & C_DMA4_TIMEOUT) { 	
				handle_dma_timout(C_DMA_CH4);
			} else {
				handle_dma_done(C_DMA_CH4);
			}
			
			if (dma_usage[C_DMA_CH4] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH4);
			}
		}
		R_DMA_INT = C_DMA4_INT_PEND;		// Clear pending bit
	}

	// DMA5
	if (pending & C_DMA5_INT_PEND) {		// DMA5 interrupt is pending
		if (dma_usage[C_DMA_CH5] != C_DMA_NOT_UESED) {
			if (pending & C_DMA5_TIMEOUT) { 	
				handle_dma_timout(C_DMA_CH5);
			} else {
				handle_dma_done(C_DMA_CH5);
			}
			
			if (dma_usage[C_DMA_CH5] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH5);
			}
		}
		R_DMA_INT = C_DMA5_INT_PEND;		// Clear pending bit
	}

	// DMA6
	if (pending & C_DMA6_INT_PEND) {		// DMA6 interrupt is pending
		if (dma_usage[C_DMA_CH6] != C_DMA_NOT_UESED) {
			if (pending & C_DMA6_TIMEOUT) { 	
				handle_dma_timout(C_DMA_CH6);
			} else {
				handle_dma_done(C_DMA_CH6);
			}
			
			if (dma_usage[C_DMA_CH6] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH6);
			}
		}
		R_DMA_INT = C_DMA6_INT_PEND;		// Clear pending bit
	}

	// DMA7
	if (pending & C_DMA7_INT_PEND) {		// DMA7 interrupt is pending
		if (dma_usage[C_DMA_CH7] != C_DMA_NOT_UESED) {
			if (pending & C_DMA7_TIMEOUT) { 	
				handle_dma_timout(C_DMA_CH7);
			} else {
				handle_dma_done(C_DMA_CH7);
			}
			
			if (dma_usage[C_DMA_CH7] != C_DMA_OCCUPIED) {
				dma_free_channel(C_DMA_CH7);
			}
		}
		R_DMA_INT = C_DMA7_INT_PEND;		// Clear pending bit
	}
}

/**
 * @brief   dma initialize
 * @param   none
 * @return 	none
 */
void drv_l1_dma_init(void)
{
	INT32U i;

	R_DMA0_CTRL	= C_DMA_CTRL_RESET;			// Software reset, this bit will auto clear after reset complete
	R_DMA0_TX_COUNT	= 0x0;
	R_DMA0_SPRITE_SIZE = 0x0;
	R_DMA0_TRANSPARENT = 0x0;
	R_DMA0_MISC = 0x0;

	R_DMA1_CTRL	= C_DMA_CTRL_RESET;
	R_DMA1_TX_COUNT	= 0x0;
	R_DMA1_SPRITE_SIZE = 0x0;
	R_DMA1_TRANSPARENT = 0x0;
	R_DMA1_MISC = 0x0;

	R_DMA2_CTRL	= C_DMA_CTRL_RESET;
	R_DMA2_TX_COUNT	= 0x0;
	R_DMA2_SPRITE_SIZE = 0x0;
	R_DMA2_TRANSPARENT = 0x0;
	R_DMA2_MISC = 0x0;

	R_DMA3_CTRL	= C_DMA_CTRL_RESET;
	R_DMA3_TX_COUNT	= 0x0;
	R_DMA3_SPRITE_SIZE = 0x0;
	R_DMA3_TRANSPARENT = 0x0;
	R_DMA3_MISC = 0x0;

	R_DMA4_CTRL	= C_DMA_CTRL_RESET;
	R_DMA4_TX_COUNT	= 0x0;
	R_DMA4_SPRITE_SIZE = 0x0;
	R_DMA4_TRANSPARENT = 0x0;
	R_DMA4_MISC = 0x0;

	R_DMA5_CTRL	= C_DMA_CTRL_RESET;
	R_DMA5_TX_COUNT	= 0x0;
	R_DMA5_SPRITE_SIZE = 0x0;
	R_DMA5_TRANSPARENT = 0x0;
	R_DMA5_MISC = 0x0;

	R_DMA6_CTRL	= C_DMA_CTRL_RESET;
	R_DMA6_TX_COUNT	= 0x0;
	R_DMA6_SPRITE_SIZE = 0x0;
	R_DMA6_TRANSPARENT = 0x0;
	R_DMA6_MISC = 0x0;

	R_DMA7_CTRL	= C_DMA_CTRL_RESET;
	R_DMA7_TX_COUNT	= 0x0;
	R_DMA7_SPRITE_SIZE = 0x0;
	R_DMA7_TRANSPARENT = 0x0;
	R_DMA7_MISC = 0x0;

	R_DMA_LINE_LEN = 0x0;
	R_DMA_DEVICE = 0x76543210;
	R_DMA_CEMODE = C_DMA_CE_DONT_RESET;
	R_DMA_INT = 0xFFFFFFFF;						// Clear all pending bits

	for (i=0; i<C_DMA_CHANNEL_NUM; i++) {
		dma_usage[i] = (INT8U) C_DMA_NOT_UESED;
		dma_notify_variable[i] = (INT32U) NULL;
	#if _OPERATING_SYSTEM != _OS_NONE
		dma_notify_queue[i] = (INT32U) NULL;
	#endif
	}
	
#if _OPERATING_SYSTEM != _OS_NONE
	for (i=0; i<C_DMA_Q_NUM; i++) {
		dma_q_usage[i] = (INT8U) FALSE;
		if (dma_driver_queue[i]) {
			OSQFlush(dma_driver_queue[i]);
		} else {
			dma_driver_queue[i] = OSQCreate(&q_buffer[i][0], C_DMA_Q_BUF_SIZE);
		}
	}
#endif   

    vic_irq_register(VIC_DMA, dma_isr);
    vic_irq_enable(VIC_DMA);

    dma_init_done = TRUE;
}

/**
 * @brief   set dma direction 
 * @param   channel: dma channel number
 * @param   dir: direction, frame buffer to sprite or sprite to frame buffer
 * @return 	none
 */
static void dma_set_direction(INT32U channel, INT32U dir)
{
	switch (channel) {
	case C_DMA_CH0:
		if(dir)
			R_DMA0_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA0_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH1:
		if(dir)
			R_DMA1_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA1_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH2:
		if(dir)
			R_DMA2_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA2_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH3:
		if(dir)
			R_DMA3_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA3_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH4:
		if(dir)
			R_DMA4_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA4_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH5:
		if(dir)
			R_DMA5_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA5_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH6:
		if(dir)
			R_DMA6_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA6_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;
	case C_DMA_CH7:
		if(dir)
			R_DMA7_MISC |= C_DMA_MISC_FB_TO_SPRITE;
		else
			R_DMA7_MISC &= ~C_DMA_MISC_FB_TO_SPRITE;
		break;				
	default:
		break;
	}
}

/**
 * @brief   set dma control register 
 * @param   channel: dma channel number
 * @param   ctrl: dma control register value
 * @return 	none
 */
static void dma_set_control(INT32U channel, INT32U ctrl)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_CTRL = ctrl;
		break;
	case C_DMA_CH1:
		R_DMA1_CTRL = ctrl;
		break;
	case C_DMA_CH2:
		R_DMA2_CTRL = ctrl;
		break;
	case C_DMA_CH3:
		R_DMA3_CTRL = ctrl;
		break;
	case C_DMA_CH4:
		R_DMA4_CTRL = ctrl;
		break;
	case C_DMA_CH5:
		R_DMA5_CTRL = ctrl;
		break;
	case C_DMA_CH6:
		R_DMA6_CTRL = ctrl;
		break;
	case C_DMA_CH7:
		R_DMA7_CTRL = ctrl;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma source address 
 * @param   channel: dma channel number
 * @param   addr: dma source address
 * @return 	none
 */
static void dma_set_source(INT32U channel, INT32U addr)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_SRC_ADDR = addr;
		break;
	case C_DMA_CH1:
		R_DMA1_SRC_ADDR = addr;
		break;
	case C_DMA_CH2:
		R_DMA2_SRC_ADDR = addr;
		break;
	case C_DMA_CH3:
		R_DMA3_SRC_ADDR = addr;
		break;
	case C_DMA_CH4:
		R_DMA4_SRC_ADDR = addr;
		break;
	case C_DMA_CH5:
		R_DMA5_SRC_ADDR = addr;
		break;
	case C_DMA_CH6:
		R_DMA6_SRC_ADDR = addr;
		break;
	case C_DMA_CH7:
		R_DMA7_SRC_ADDR = addr;
		break;
	default:
		break;
	}
}

/**
 * @brief   set dma destination address 
 * @param   channel: dma channel number
 * @param   addr: dma destination address
 * @return 	none
 */
static void dma_set_target(INT32U channel, INT32U addr)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_TAR_ADDR = addr;
		break;
	case C_DMA_CH1:
		R_DMA1_TAR_ADDR = addr;
		break;
	case C_DMA_CH2:
		R_DMA2_TAR_ADDR = addr;
		break;
	case C_DMA_CH3:
		R_DMA3_TAR_ADDR = addr;
		break;
	case C_DMA_CH4:
		R_DMA4_TAR_ADDR = addr;
		break;
	case C_DMA_CH5:
		R_DMA5_TAR_ADDR = addr;
		break;
	case C_DMA_CH6:
		R_DMA6_TAR_ADDR = addr;
		break;
	case C_DMA_CH7:
		R_DMA7_TAR_ADDR = addr;
		break;
	default:
		break;
	}
}

static INT32S dma_setting(INT32U channel, INT32U saddr, INT32U taddr, INT32U xfer_cnt, INT32U timeout, INT32U sprite_sz)
{
	if((xfer_cnt > C_DMA_COUNT_MAX) || (timeout>C_DMA_MISC_TIMEOUT_MAX) || (sprite_sz > C_DMA_SPRITE_MAX)) {
		return -1;
	}
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_SRC_ADDR = saddr;
		R_DMA0_TAR_ADDR = taddr;
		R_DMA0_TX_COUNT = xfer_cnt;
        R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA0_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA0_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH1:
		R_DMA1_SRC_ADDR = saddr;
		R_DMA1_TAR_ADDR = taddr;
		R_DMA1_TX_COUNT = xfer_cnt;
        R_DMA1_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA1_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA1_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH2:
		R_DMA2_SRC_ADDR = saddr;
		R_DMA2_TAR_ADDR = taddr;
		R_DMA2_TX_COUNT = xfer_cnt;
		R_DMA2_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA2_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA2_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH3:
		R_DMA3_SRC_ADDR = saddr;
		R_DMA3_TAR_ADDR = taddr;
		R_DMA3_TX_COUNT = xfer_cnt;
		R_DMA3_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA3_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA3_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH4:
		R_DMA4_SRC_ADDR = saddr;
		R_DMA4_TAR_ADDR = taddr;
		R_DMA4_TX_COUNT = xfer_cnt;
		R_DMA4_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA4_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA4_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH5:
		R_DMA5_SRC_ADDR = saddr;
		R_DMA5_TAR_ADDR = taddr;
		R_DMA5_TX_COUNT = xfer_cnt;
		R_DMA5_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA5_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA5_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH6:
		R_DMA6_SRC_ADDR = saddr;
		R_DMA6_TAR_ADDR = taddr;
		R_DMA6_TX_COUNT = xfer_cnt;
		R_DMA6_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA6_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA6_SPRITE_SIZE = sprite_sz;
		break;
	case C_DMA_CH7:
		R_DMA7_SRC_ADDR = saddr;
		R_DMA7_TAR_ADDR = taddr;
		R_DMA7_TX_COUNT = xfer_cnt;
		R_DMA7_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA7_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		R_DMA7_SPRITE_SIZE = sprite_sz;
		break;
	default:
		return -1;
	}
	return 0;
}

/**
 * @brief   set dma transfer length 
 * @param   channel: dma channel number
 * @param   count: transfer length 
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_tx_count(INT32U channel, INT32U count)
{
	if (count > C_DMA_COUNT_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_TX_COUNT = count;
		break;
	case C_DMA_CH1:
		R_DMA1_TX_COUNT = count;
		break;
	case C_DMA_CH2:
		R_DMA2_TX_COUNT = count;
		break;
	case C_DMA_CH3:
		R_DMA3_TX_COUNT = count;
		break;
	case C_DMA_CH4:
		R_DMA4_TX_COUNT = count;
		break;
	case C_DMA_CH5:
		R_DMA5_TX_COUNT = count;
		break;
	case C_DMA_CH6:
		R_DMA6_TX_COUNT = count;
		break;
	case C_DMA_CH7:
		R_DMA7_TX_COUNT = count;
		break;
	default:
		return -1;
	}
	return 0;
}

/**
 * @brief   set dma transfer device number 
 * @param   channel: dma channel number
 * @param   count: device number 
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_device(INT32U channel, INT16U device)
{
	INT32U shift = 0;

	if ((channel>=C_DMA_CHANNEL_NUM) || (device>C_DMA_IO_MAX)) {
		return -1;
	}

	if (channel == C_DMA_CH0) {
		shift = C_DMA0_IO_SHIFT;
	} else if (channel == C_DMA_CH1) {
		shift = C_DMA1_IO_SHIFT;
	} else if (channel == C_DMA_CH2) {
		shift = C_DMA2_IO_SHIFT;
	} else if (channel == C_DMA_CH3) {
		shift = C_DMA3_IO_SHIFT;
	} else if (channel == C_DMA_CH4) {
		shift = C_DMA4_IO_SHIFT;
	} else if (channel == C_DMA_CH5) {
		shift = C_DMA5_IO_SHIFT;
	} else if (channel == C_DMA_CH6) {
		shift = C_DMA6_IO_SHIFT;
	} else if (channel == C_DMA_CH7) {
		shift = C_DMA7_IO_SHIFT;
	}

	R_DMA_DEVICE &= ~((C_DMA_IO_MASK) << shift);
	R_DMA_DEVICE |= device << shift;
	return 0;
}

/**
 * @brief   set dma timeout value
 * @param   channel: dma channel number
 * @param   timeout: timeout value 
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_timeout(INT32U channel, INT32U timeout)
{
	if((channel>=C_DMA_CHANNEL_NUM) || (timeout>C_DMA_MISC_TIMEOUT_MAX)) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA0_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH1:
		R_DMA1_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA1_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH2:
		R_DMA2_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA2_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH3:
		R_DMA3_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA3_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH4:
		R_DMA4_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA4_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH5:
		R_DMA5_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA5_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH6:
		R_DMA6_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA6_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	case C_DMA_CH7:
		R_DMA7_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK);
		R_DMA7_MISC |= timeout << C_DMA_MISC_TIMEOUT_SHIFT;
		break;
	default:
		break;
	}

	return 0;
}

/**
 * @brief   set dma line length
 * @param   length: line length for LCD frame buffer
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_line_length(INT32U length)
{
	if  (length > C_DMA_LINE_MAX) {
		return -1;
	}

	R_DMA_LINE_LEN = length;
	return 0;
}

/**
 * @brief   set dma sprite size
 * @param   channel: dma channel
 * @param   size: sprite size
 * @return 	result: >=0 is success, <0 is fail.
 */
static INT32S dma_set_sprite_size(INT32U channel, INT32U size)
{
	if  (size > C_DMA_SPRITE_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_SPRITE_SIZE = size;
		break;
	case C_DMA_CH1:
		R_DMA1_SPRITE_SIZE = size;
		break;
	case C_DMA_CH2:
		R_DMA2_SPRITE_SIZE = size;
		break;
	case C_DMA_CH3:
		R_DMA3_SPRITE_SIZE = size;
		break;
	case C_DMA_CH4:
		R_DMA4_SPRITE_SIZE = size;
		break;
	case C_DMA_CH5:
		R_DMA5_SPRITE_SIZE = size;
		break;
	case C_DMA_CH6:
		R_DMA6_SPRITE_SIZE = size;
		break;
	case C_DMA_CH7:
		R_DMA7_SPRITE_SIZE = size;
		break;
	default:
		return -1;
	}

	return 0;
}

#if 0
static INT32S dma_set_transparent_enable(INT32U channel)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH1:
		R_DMA1_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH2:
		R_DMA2_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH3:
		R_DMA3_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH4:
		R_DMA4_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH5:
		R_DMA5_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH6:
		R_DMA6_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;
	case C_DMA_CH7:
		R_DMA7_MISC |= C_DMA_MISC_TRANSPARENT_EN;
		break;				
	default:
		return -1;
	}

	return 0;
}

static INT32S dma_set_transparent_disable(INT32U channel)
{
	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH1:
		R_DMA1_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH2:
		R_DMA2_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH3:
		R_DMA3_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH4:
		R_DMA4_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH5:
		R_DMA5_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH6:
		R_DMA6_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;
	case C_DMA_CH7:
		R_DMA7_MISC &= ~(C_DMA_MISC_TRANSPARENT_EN);
		break;			
	default:
		return -1;
	}

	return 0;
}

static INT32S dma_set_transparent_pattern(INT32U channel, INT16U pattern)
{
	if (pattern > C_DMA_TRANSPARENT_MAX) {
		return -1;
	}

	switch (channel) {
	case C_DMA_CH0:
		R_DMA0_TRANSPARENT = pattern;
		break;
	case C_DMA_CH1:
		R_DMA1_TRANSPARENT = pattern;
		break;
	case C_DMA_CH2:
		R_DMA2_TRANSPARENT = pattern;
		break;
	case C_DMA_CH3:
		R_DMA3_TRANSPARENT = pattern;
		break;
	case C_DMA_CH4:
		R_DMA4_TRANSPARENT = pattern;
		break;
	case C_DMA_CH5:
		R_DMA5_TRANSPARENT = pattern;
		break;
	case C_DMA_CH6:
		R_DMA6_TRANSPARENT = pattern;
		break;			
	case C_DMA_CH7:
		R_DMA7_TRANSPARENT = pattern;
		break;			
	default:
		return -1;
	}

	return 0;
}
#endif

#if 1
static INT32S aes_load_key_wait(INT32U time_out)
{
	INT32U i;
	
	for(i=0;i<time_out;i++) 
	{
		if((R_AES_LOAD_KEY & 0x10) == 0) {
			return 0;
		} else {
			INT32U j;
			INT32U loopCnt = 0xf; // [200k]0x4FF;  // [100k]0x3FF;
			
			for(j=0; j<loopCnt; j++) {
				j = j;
			}
		}
	}
	
	return -1;
}
#endif

static INT32S dma_transfer_extend(DMA_STRUCT *dma_struct, INT8U usage, INT32U os_q)
{
	INT32U s_addr = dma_struct->s_addr;
	INT32U t_addr = dma_struct->t_addr;
	INT32U count = dma_struct->count;
	INT32U channel, len, ctrl;
	INT16U src_type, target_type;

	if (!count) {
		if (dma_struct->notify) {
			*(dma_struct->notify) = C_DMA_STATUS_DONE;
		}
		return 0;
	}
	
	if (dma_struct->width == DMA_DATA_WIDTH_1BYTE) {
		ctrl = C_DMA_CTRL_8BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;	
		len = count;
	} else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE) {
		if ((s_addr&0x1) || (t_addr&0x1)) {
			return -1;						// Both source and target address must be 2-byte alignment
		}

		ctrl = C_DMA_CTRL_16BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 1;
	} else if (dma_struct->width == DMA_DATA_WIDTH_4BYTE) {
		if ((s_addr&0x3) || (t_addr&0x3)) {
			return -1;						// Both source and target address must be 4-byte alignment
		}

		ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 2;
	} else {
		return -1;
	}

	if ((s_addr<C_DMA_IO_ADDR_START) || (s_addr>C_DMA_IO_ADDR_END)) {
		src_type = C_DMA_MEMORY;
	} else {
		if (s_addr == (INT32U)P_UART0_BASE) {
			src_type = C_DMA_IO_UART0_RX;
		} else if (s_addr == (INT32U) P_SDC0_DATA_RX) {
			src_type = C_DMA_IO_SDC0;
		} else if (s_addr == (INT32U) P_SDC1_DATA_RX) {
			src_type = C_DMA_IO_SDC1;
		} else if (s_addr == (INT32U) P_ADC_ASADC_DATA) {
			src_type = C_DMA_IO_ADC;
		} else if (s_addr == (INT32U) P_SPI_RX_DATA) {
			src_type = C_DMA_IO_SPI_RX;
		} else if (s_addr == (INT32U) P_UART1_BASE) {
			src_type = C_DMA_IO_UART1_RX;
		} else if (s_addr == (INT32U) P_I2SRX_DATA) {
			src_type = C_DMA_IO_I2S_RX;
		} else {
			return -4;						// Unknow IO
		}
	}
	if ((t_addr<C_DMA_IO_ADDR_START) || (t_addr>C_DMA_IO_ADDR_END)) {
		target_type = C_DMA_MEMORY;
	} else {
		if (src_type != C_DMA_MEMORY) {
			return -1;						// IO to IO is not supported
		}
		
		if (t_addr == (INT32U) P_DAC_CHA_DATA) {
			target_type = C_DMA_IO_DAC_CHA;
		} else if (t_addr == (INT32U) P_UART0_BASE) {
			target_type = C_DMA_IO_UART0_TX;
		} else if (t_addr == (INT32U) P_SDC0_DATA_TX) {
			target_type = C_DMA_IO_SDC0;
		} else if (t_addr == (INT32U) P_SDC1_DATA_TX) {
			target_type = C_DMA_IO_SDC1;
		} else if (t_addr == (INT32U) P_DAC_CHB_DATA) {
			target_type = C_DMA_IO_DAC_CHB;				
		} else if (t_addr == (INT32U) P_SPI_TX_DATA) {
			target_type = C_DMA_IO_SPI_TX;
		} else if (t_addr == (INT32U) P_UART1_BASE) {
			target_type = C_DMA_IO_UART1_TX;
		} else if (t_addr == (INT32U) P_I2STX_DATA) {
			target_type = C_DMA_IO_I2S_TX;
		} else {
			return -5;						// Unknow IO
		}
	}

	if (!dma_init_done) {
		drv_l1_dma_init();
	}
	
#if 0
	// aes only support in dma2
	if(dma_struct->aes && dma_struct->aes->enable) 
	{
		dma_struct->channel = 2;
		if(dma_set_channel(usage, 2) < 0) {
			return -1;							// DMA channel is not available
		}
		
		R_AES_KEY0 = dma_struct->aes->key[0];
		R_AES_KEY1 = dma_struct->aes->key[1];
		R_AES_KEY2 = dma_struct->aes->key[2];
		R_AES_KEY3 = dma_struct->aes->key[3];
		R_AES_LOAD_KEY = 0x01;
		if(aes_load_key_wait(5))
			return -1;
		if(dma_struct->aes->mode)
			R_AES_LOAD_KEY = 0x1100; // Encrypt
		else
			R_AES_LOAD_KEY = 0x1000; //	Discrypt	
	} 
	else
#endif	 
	{
		channel = dma_get_channel(usage);
		dma_struct->channel = channel;
		if  (channel >= C_DMA_CHANNEL_NUM) {
			return -1;							// No free DMA channel is available
		}
	}

#if 0
	// external interrupt trigger mode. 
	if(dma_struct->trigger && dma_struct->trigger->enable) {
		// must use Demand Transfer mode.
		if((src_type == C_DMA_MEMORY) && (target_type == C_DMA_MEMORY)) {
			return -1;
		}
		
		if(dma_struct->trigger->source > 2) {
			return -1;
		}
		
		ctrl |= (0x01 << 19); 		//trigger enable
		ctrl &= ~(0x0F << 20); 		//trigger source, exta, eatb, extc...
		ctrl |= ((INT32U)(dma_struct->trigger->source & 0x0F) << 20);
		if(dma_struct->trigger->edge) {
			ctrl |= (0x01 << 24);	//rising edge
		} else {
			ctrl &= ~(0x01 << 24);	//falling edge
		}
	}
#endif	

	dma_set_notify(channel, (INT32U) dma_struct->notify, os_q);

	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

	// Set transmit counter
	if (dma_set_tx_count(channel, count)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_timeout(channel, dma_struct->timeout)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_sprite_size(channel, 0)) {
		dma_free_channel(channel);
		return -1;
	}
 
	// Prepare control register
	if (src_type != C_DMA_MEMORY) {			// IO to memory
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_IO2M | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, src_type)) {
			dma_free_channel(channel);
			return -1;
		}

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Invalid target memory from cache
		cache_invalid_range(t_addr, len);
	#endif

	} else if (target_type != C_DMA_MEMORY) {	// Memory to IO
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_M2IO | C_DMA_CTRL_DEST_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, target_type)) {
			dma_free_channel(channel);
			return -1;
		}

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Drain source memory from cache
		cache_drain_range(s_addr, len);
	#endif

	} else {								// Memory to memory
		ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Drain source memory and invalid target memory from cache
		cache_drain_range(s_addr, len);
		cache_invalid_range(t_addr, len);
	#endif
	}

	// Check whether burst mode can be used
	if (usage == C_DMA_OCCUPIED) {
		ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
	} else if (!(count & 0x7)) {
		ctrl |= C_DMA_CTRL_BURST8_ACCESS;
	} else if (!(count & 0x3)) {
		ctrl |= C_DMA_CTRL_BURST4_ACCESS;
	} else {
		ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
	}
	// Start DMA now
	dma_set_control(channel, ctrl);
	return 0;
}

/**
 * @brief   transfer data by dma
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer(DMA_STRUCT *dma_struct)
{
	if (!dma_struct) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, NULL);
}

/**********************************************
 * @brief   sdio transfer data by dma
 * @param   dma_struct: dma struct
 * @param   usage: dma use method
 * @return 	result: >=0 is success, <0 is fail.
 **********************************************/
INT32S drv_l1_dma_transfer_sdio(DMA_STRUCT *dma_struct, INT8U usage)
{
	INT32U s_addr = dma_struct->s_addr;
	INT32U t_addr = dma_struct->t_addr;
	INT32U count = dma_struct->count;
	INT32U channel, len, ctrl;
	INT16U src_type, target_type;

	if (!dma_struct) {
		return -1;
	}
    if (dma_struct->width == DMA_DATA_WIDTH_4BYTE) {
		if ((s_addr&0x3) || (t_addr&0x3)) {
			return -1;						// Both source and target address must be 4-byte alignment
		}
		ctrl = C_DMA_CTRL_32BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 2;
	} else if (dma_struct->width == DMA_DATA_WIDTH_2BYTE) {
		if ((s_addr&0x1) || (t_addr&0x1)) {
			return -1;						// Both source and target address must be 2-byte alignment
		}
		ctrl = C_DMA_CTRL_16BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count << 1;
		DBG_PRINT("s_addr %x t_addr %x width %d byte len %d\r\n",s_addr,t_addr,dma_struct->width,len);
	} else if (dma_struct->width == DMA_DATA_WIDTH_1BYTE) {
		ctrl = C_DMA_CTRL_8BIT | C_DMA_CTRL_INT | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_ENABLE;
		len = count ;
		DBG_PRINT("s_addr %x t_addr %x width %d byte len %d\r\n",s_addr,t_addr,dma_struct->width,len);
	} else {
		return -1;
	}

    //set source type
    if ((s_addr<C_DMA_IO_ADDR_START) || (s_addr>C_DMA_IO_ADDR_END)) {
		src_type = C_DMA_MEMORY;
	} else {
		if (s_addr == (INT32U) P_SDC0_DATA_RX) {
			src_type = C_DMA_IO_SDC0;
        } else if (s_addr == (INT32U) P_SDC1_DATA_RX) {
			src_type = C_DMA_IO_SDC1;
		} else {
			return -4;						// unknown IO
		}
	}
	//set target type
	if ((t_addr<C_DMA_IO_ADDR_START) || (t_addr>C_DMA_IO_ADDR_END)) {
		target_type = C_DMA_MEMORY;
	} else {
		if (src_type != C_DMA_MEMORY) {
			return -1;						// IO to IO is not supported
		}
		if (t_addr == (INT32U) P_SDC0_DATA_TX) {
			target_type = C_DMA_IO_SDC0;
        } else if (t_addr == (INT32U) P_SDC1_DATA_TX) {
			target_type = C_DMA_IO_SDC1;
		} else {
			return -5;						// unknown IO
		}
    }

    if (!dma_init_done) {
        drv_l1_dma_init();
	}
    //channel = dma_get_channel(C_DMA_NORMAL_USED, t_addr, s_addr);
    channel = dma_get_channel(C_DMA_NORMAL_USED);
	dma_struct->channel = channel;
	if (channel >= C_DMA_CHANNEL_NUM) {
        return -1;							// No free DMA channel is available
    }
 	dma_set_notify(channel, (INT32U) dma_struct->notify, 0);
/*
	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

	// Set transmit counter
	if (dma_set_tx_count(channel, count)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_timeout(channel, dma_struct->timeout)) {
		dma_free_channel(channel);
		return -1;
	}

	if (dma_set_sprite_size(channel, 0)) {
		dma_free_channel(channel);
		return -1;
	}
*/
    dma_setting(channel, s_addr, t_addr, count, dma_struct->timeout, 0);
	// Prepare control register
	if (src_type != C_DMA_MEMORY) {			// IO to memory
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_IO2M | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, src_type)) {
			dma_free_channel(channel);
			return -1;
		}

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Invalid target memory from cache
		cache_invalid_range(t_addr, len);
	#endif

	} else if (target_type != C_DMA_MEMORY) {	// Memory to IO
		ctrl |= C_DMA_CTRL_DEMAND_TRANS | C_DMA_CTRL_M2IO | C_DMA_CTRL_DEST_FIX | C_DMA_CTRL_EXTERNAL;

		if (dma_set_device(channel, target_type)) {
			dma_free_channel(channel);
			return -1;
		}

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Drain source memory from cache
		cache_drain_range(s_addr, len);
	#endif

	}
	else
	{								// Memory to memory
		ctrl |= C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_SOFTWARE;

	#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
		// Drain source memory and invalid target memory from cache
		cache_drain_range(s_addr, len);
		cache_invalid_range(t_addr, len);
	#endif
	}

    // Check whether burst mode can be used
    if (usage == C_DMA_OCCUPIED) {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    } else if (!(count & 0x7)) {
        ctrl |= C_DMA_CTRL_BURST8_ACCESS;
    } else if (!(count & 0x3)) {
        ctrl |= C_DMA_CTRL_BURST4_ACCESS;
    } else {
        ctrl |= C_DMA_CTRL_SINGLE_ACCESS;
    }

	// Start DMA now
	dma_set_control(channel, ctrl);
    return 0;
}

/**
 * @brief   dma transfer data and wait transfer finish
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_wait_ready(DMA_STRUCT *dma_struct)
{
	volatile INT8S notify;
#if _OPERATING_SYSTEM != _OS_NONE
  	INT32U q_index;
  	INT32S mask;
	INT32S result;
	INT8U error;
#endif

	if (!dma_struct) {
		return -1;
	}
	if (!(dma_struct->notify)) {
		dma_struct->notify = &notify;
	}
	*(dma_struct->notify) = C_DMA_STATUS_WAITING;
#if _OPERATING_SYSTEM != _OS_NONE
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}
  	if (dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) dma_driver_queue[q_index])) {
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}
  		dma_free_queue(q_index);

		return -1;
	}
  	result = (INT32S) OSQPend(dma_driver_queue[q_index], 0, &error);
  	dma_free_queue(q_index);
	if (error == OS_NO_ERR) {
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}
		if (result == C_DMA_STATUS_DONE) {
			return 0;
		}

		return -1;						// DMA timeout
  	}

	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	mask = dma_device_protect();
	dma_free_channel((INT32U) dma_struct->channel);
	dma_device_unprotect(mask);

#else
	if (dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, NULL)) {
		return -1;
	}
#endif

	while (*((volatile INT8S *) dma_struct->notify) == C_DMA_STATUS_WAITING) ;
	if (*(dma_struct->notify) == C_DMA_STATUS_DONE) {
  		if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
  			dma_struct->notify = NULL;
  		}

		return 0;
	}
	if (dma_struct->notify == &notify) {		// Restore dma_struct->notify
		dma_struct->notify = NULL;
	}

	return -1;								// DMA timeout
}

#if _OPERATING_SYSTEM != _OS_NONE
/**
 * @brief   dma transfer data with queue
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_with_queue(DMA_STRUCT *dma_struct, OS_EVENT *os_q)
{
	if (!dma_struct || !os_q) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_NORMAL_USED, (INT32U) os_q);
}

/**
 * @brief   dma transfer data with queue in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_with_double_buf(DMA_STRUCT *dma_struct, OS_EVENT *os_q)
{
	if (!dma_struct || !os_q) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_OCCUPIED, (INT32U) os_q);
}

#else
/**
 * @brief   dma transfer data in double buffer mode.
 * @param   dma_struct: dma struct
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_with_double_buf(DMA_STRUCT *dma_struct)
{
	if (!dma_struct) {
		return -1;
	}
	return dma_transfer_extend(dma_struct, C_DMA_OCCUPIED, NULL);
}
#endif

/**
 * @brief   dma transfer next data in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_double_buf_set(DMA_STRUCT *dma_struct)
{
  	INT32S mask;

	if (!dma_struct || dma_struct->channel>=C_DMA_Q_NUM) {
		return -1;
	}
	if (dma_usage[dma_struct->channel] != C_DMA_OCCUPIED) {
		return -1;
	}
#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
	cache_drain_range(dma_struct->s_addr, dma_struct->count*2);
#endif

	dma_set_source((INT32U) dma_struct->channel, dma_struct->s_addr);
	dma_set_target((INT32U) dma_struct->channel, dma_struct->t_addr);
	if (dma_set_tx_count((INT32U) dma_struct->channel, dma_struct->count)) {
		mask = dma_device_protect();
		dma_free_channel((INT32U) dma_struct->channel);
		dma_device_unprotect(mask);

		return -1;
	}

	return 0;
}

/**
 * @brief   dma free channel in double buffer mode.
 * @param   dma_struct: dma struct
 * @param   os_q: notify queue
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S drv_l1_dma_transfer_double_buf_free(DMA_STRUCT *dma_struct)
{
  	INT32S mask;

	if (!dma_struct) {
		return -1;
	}
	if (dma_usage[dma_struct->channel] != C_DMA_OCCUPIED) {
		return -1;
	}

	mask = dma_device_protect();
	dma_free_channel((INT32U) dma_struct->channel);
	dma_device_unprotect(mask);

	return 0;
}

/**
 * @brief   Check double buffer full flag.
 * @param   channel: channel number, C_DMA_CH0 ~ C_DMA_CH7
 * @return 	1: Full, 0: Not full
 */
INT32S drv_l1_dma_dbf_status_get(INT8U channel)
{
	INT32U ctrl;

	if (channel == C_DMA_CH0) {
		ctrl = R_DMA0_CTRL;
	} else if (channel == C_DMA_CH1) {
		ctrl = R_DMA1_CTRL;
	} else if (channel == C_DMA_CH2) {
		ctrl = R_DMA2_CTRL;
	} else if (channel == C_DMA_CH3) {
		ctrl = R_DMA3_CTRL;
	} else if (channel == C_DMA_CH4) {
		ctrl = R_DMA4_CTRL;
	} else if (channel == C_DMA_CH5) {
		ctrl = R_DMA5_CTRL;
	} else if (channel == C_DMA_CH6) {
		ctrl = R_DMA6_CTRL;
	} else if (channel == C_DMA_CH7) {
		ctrl = R_DMA7_CTRL;		
	} else {
		ctrl = 0;
	}

	if (ctrl & C_DMA_CTRL_DBF) {
		return 1;
	}
	return 0;
}

/**
 * @brief   Check DMA channel status
 * @param   channel: channel number, C_DMA_CH0 ~ C_DMA_CH7
 * @return 	1: Busy, 0: Idle
 */
INT32S drv_l1_dma_status_get(INT8U channel)
{
	INT32U ctrl;

	if (channel == C_DMA_CH0) {
		ctrl = R_DMA0_CTRL;
	} else if (channel == C_DMA_CH1) {
		ctrl = R_DMA1_CTRL;
	} else if (channel == C_DMA_CH2) {
		ctrl = R_DMA2_CTRL;
	} else if (channel == C_DMA_CH3) {
		ctrl = R_DMA3_CTRL;
	} else if (channel == C_DMA_CH4) {
		ctrl = R_DMA4_CTRL;
	} else if (channel == C_DMA_CH5) {
		ctrl = R_DMA5_CTRL;
	} else if (channel == C_DMA_CH6) {
		ctrl = R_DMA6_CTRL;
	} else if (channel == C_DMA_CH7) {
		ctrl = R_DMA7_CTRL;		
	} else {
		ctrl = 0;
	}

	if (ctrl & C_DMA_CTRL_BUSY) {
		return 1;
	}
	return 0;
}

/**
 * @brief   Fill memory by DMA
 * @param   t_addr: memory address
 * @param   value: filled value
 * @param   byte_count: data length in byte
 * @return 	0: Success, -1: Fail 
 */
INT32S drv_l1_dma_memory_fill(INT32U t_addr, INT8U value, INT32U byte_count)
{
	INT8U *p8;
	INT32U *p32;
	INT32U ctrl, channel;
	INT32U src_value;
	INT32S ret;
#if _OPERATING_SYSTEM != _OS_NONE
  	INT32U q_index;
	INT32S result;
  	INT32S mask;
  	INT8U error;
#endif
  	volatile INT8S notify;

	if (!dma_init_done) {
		drv_l1_dma_init();
	}

	// If length is less than 16 bytes, uses CPU to set the memory directly
	if (byte_count < 16) {
		p8 = (INT8U *) t_addr;
		while (byte_count--) {
			*p8++ = value;
		}

		return 0;
	}

	// Make sure start address is 4-byte alignment
	while (t_addr & 0x3) {
		*((INT8U *) t_addr++) = value;
		byte_count--;
	}
	// Make sure end address is on 4-byte boundry
	while (byte_count & 0x3) {
		*((INT8U *) t_addr+byte_count-1) = value;
		byte_count--;
	}

	src_value = (value<<24) | (value<<16) | (value<<8) | value;

	// If left length is less than 128 bytes, uses CPU to set the memory directly
	if (byte_count < 128) {
		p32 = (INT32U *) t_addr;
		while (byte_count) {
			*p32++ = src_value;
			byte_count -= 4;
		}
		return 0;
	}

	// Create a queue to receive DMA result
#if _OPERATING_SYSTEM != _OS_NONE
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}
#endif

	// Get a free DMA channel
	channel = dma_get_channel(C_DMA_NORMAL_USED);
	if  (channel >= C_DMA_CHANNEL_NUM) {
	#if _OPERATING_SYSTEM != _OS_NONE
	  	dma_free_queue(q_index);
	#endif

		return -1;							// No free DMA channel is available
	}

	// Set transmit counter
	ret = dma_set_tx_count(channel, byte_count>>2);

	// Disable timeout function
	if (dma_set_timeout(channel, 0)) {
		ret = -1;
	}

	// Disable skip function
	if (dma_set_sprite_size(channel, 0)) {
		ret = -1;
	}

	// Return if any error occurs
	if (ret) {
	#if _OPERATING_SYSTEM != _OS_NONE
	  	dma_free_queue(q_index);
	#endif
		dma_free_channel(channel);

		return -1;
	}

	// Set DMA status to waiting transmit
	notify = C_DMA_STATUS_WAITING;
#if _OPERATING_SYSTEM != _OS_NONE
	dma_set_notify(channel, (INT32U) &notify, (INT32U) dma_driver_queue[q_index]);
#else
	dma_set_notify(channel, (INT32U) &notify, NULL);
#endif

	// Set source address
	dma_set_source(channel, (INT32U) &src_value);

	// Set target address
	dma_set_target(channel, t_addr);

#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
	// Drain source memory and invalid target memory from cache
	cache_drain_range((INT32U) &src_value, 4);
	cache_invalid_range(t_addr, byte_count);
#endif

	// Prepare control register
	if (!((byte_count>>2) & 0x7)) {
		ctrl = C_DMA_CTRL_BURST8_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	} else if (!((byte_count>>2) & 0x3)) {
		ctrl = C_DMA_CTRL_BURST4_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	} else {
		ctrl = C_DMA_CTRL_SINGLE_ACCESS | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_32BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_FIX | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	}
	// Start DMA now
	dma_set_control(channel, ctrl);

	// Wait until DMA finish transmitting or timeout
#if _OPERATING_SYSTEM != _OS_NONE
	result = (INT32S) OSQPend(dma_driver_queue[q_index], 10 * OS_TICKS_PER_SEC, &error);
  	if(error!= 0 && !result)
  		return -1;
	dma_free_queue(q_index);
	if (error == OS_NO_ERR) {
		return 0;				// DMA timeout function is not enabled, DMA transfer must be ok to reach here
	}

	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	mask = dma_device_protect();
	dma_free_channel(channel);
	dma_device_unprotect(mask);

	if (notify == C_DMA_STATUS_DONE) {
		return 0;
	}
	return -1;

#else
	while (notify == C_DMA_STATUS_WAITING) ;

	return 0;					// DMA timeout function is not enabled, DMA transfer must be ok to reach here
#endif
}

/**
 * @brief   Copy a section of data
 * @param   s_addr: Source address
 * @param   t_addr: Destination address
 * @param   byte_count: Data number needed to be copied
 * @param   s_width: Buffer width of source buffer
 * @param   t_width: Buffer width of destination buffer
 * @param   dir:dma direction
 * @param   mode:Prepare control mod
 * @return 	0: Success, -1: Fail 
 */
INT32S dma_buffer_copy_extend(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width, INT32U dir, INT32U mode)
{
	INT32U ctrl, channel;
	INT32S ret;
#if _OPERATING_SYSTEM != _OS_NONE
	INT32U q_index;
	INT32S result;
	INT32S mask;
	INT8U error;
#endif
  	volatile INT8S notify;

	if (!dma_init_done) {
		drv_l1_dma_init();
	}

	// Make sure address and size are 2-byte alignment
	if ((s_addr & 0x1) || (t_addr & 0x1) || (byte_count & 0x1) || (s_width & 0x1) || (t_width & 0x1) || (s_width>t_width)) {
		return -1;
	}

  	// Create a queue to receive DMA result
#if _OPERATING_SYSTEM != _OS_NONE
	q_index = dma_get_queue();
	if (q_index >= C_DMA_Q_NUM) {
		return -1;
	}
#endif

	// Get a free DMA channel
	channel = dma_get_channel(C_DMA_NORMAL_USED);
	if  (channel >= C_DMA_CHANNEL_NUM) {
	#if _OPERATING_SYSTEM != _OS_NONE
	  	dma_free_queue(q_index);
	#endif

		return -1;							// No free DMA channel is available
	}

	// Set transmit counter
	ret = dma_set_tx_count(channel, byte_count>>1);

	// Disable timeout function
	if (dma_set_timeout(channel, 0)) {
		ret = -1;
	}
	
	dma_set_direction(channel, dir);
		
	// Set destination buffer width (1 means 2 bytes)
	if (dma_set_line_length(t_width>>1)) {
		ret = -1;
	}

	// Set source buffer width (1 means 2 bytes)
	if (dma_set_sprite_size(channel, s_width>>1)) {
		ret = -1;
	}

	// Return if any error occurs
	if (ret) {
	#if _OPERATING_SYSTEM != _OS_NONE
	  	dma_free_queue(q_index);
	#endif
		dma_free_channel(channel);

		return -1;
	}

	// Set DMA status to waiting transmit
	notify = C_DMA_STATUS_WAITING;
#if _OPERATING_SYSTEM != _OS_NONE
	dma_set_notify(channel, (INT32U) &notify, (INT32U) dma_driver_queue[q_index]);
#else
	dma_set_notify(channel, (INT32U) &notify, NULL);
#endif

	// Set source address
	dma_set_source(channel, s_addr);

	// Set target address
	dma_set_target(channel, t_addr);

#if (defined _DRV_L1_CACHE) && (_DRV_L1_CACHE == 1)
	// Drain source memory and invalid target memory from cache
	cache_drain_range(s_addr, byte_count);
	cache_invalid_range(t_addr, (byte_count/s_width)*t_width);
#endif

	// Prepare control register, burst is not supported in sprite copy mode
	ctrl = mode | C_DMA_CTRL_SINGLE_TRANS | C_DMA_CTRL_16BIT | C_DMA_CTRL_M2M | C_DMA_CTRL_INT | C_DMA_CTRL_SRC_INCREASE | C_DMA_CTRL_DEST_INCREASE | C_DMA_CTRL_NORMAL_INT | C_DMA_CTRL_SOFTWARE | C_DMA_CTRL_ENABLE;
	// Start DMA now
	dma_set_control(channel, ctrl);

	// Wait until DMA finish transmitting or timeout
#if _OPERATING_SYSTEM != _OS_NONE
	result = (INT32S) OSQPend(dma_driver_queue[q_index], 5 * OS_TICKS_PER_SEC, &error);
  	if(error!= 0 && !result)
  		return -1;
	dma_free_queue(q_index);
	if (error == OS_NO_ERR) {
		return 0;				// DMA timeout function is not enabled, DMA transfer must be ok to reach here
	}

	// If we don't receive response from DMA, we have to reset DMA controller and free the channel by ourselves
	mask = dma_device_protect();
	dma_free_channel(channel);
	dma_device_unprotect(mask);

	if (notify == C_DMA_STATUS_DONE) {
		return 0;
	}
	return -1;

#else
	while (notify == C_DMA_STATUS_WAITING) ;

	return 0;					// DMA timeout function is not enabled, DMA transfer must be ok to reach here
#endif
}

/**
 * @brief   Copy a section of data
 * @param   s_addr: Source address
 * @param   t_addr: Destination address
 * @param   byte_count: Data number needed to be copied
 * @param   s_width: Buffer width of source buffer
 * @param   t_width: Buffer width of destination buffer
 			All parameters must be multiple of 2, 
 			s_width(source buffer width) must <= t_width(target buffer width)
 * @return 	0: Success, -1: Fail 
 */
INT32S drv_l1_dma_buffer_copy(INT32U s_addr, INT32U t_addr, INT32U byte_count, INT32U s_width, INT32U t_width)
{
	return dma_buffer_copy_extend(s_addr, t_addr, byte_count, s_width, t_width, C_DMA_MISC_SPRITE_TO_FB, C_DMA_CTRL_SINGLE_ACCESS);	
}

/**
 * @brief   Copy memory data
 * @param   dest: Destination address
 * @param   src: Source address
 * @param   len: Data number needed to be copied
 * @return 	0: Success, -1: Fail 
 */
INT32S dma_transfer_data(INT32U dest, INT32U src, INT32U len)
{
#if 1
	R_DMA0_CTRL	= C_DMA_CTRL_RESET; /* reset dma */
	R_DMA_INT = R_DMA_INT; /* clear interrupt flag */
	
	R_DMA0_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK); /* set time-out value */
	R_DMA0_MISC |= 0xFF << C_DMA_MISC_TIMEOUT_SHIFT; /* 1 sec */
	
	/* set DMA 0 for transfering data */
	R_DMA0_SRC_ADDR = (INT32U)src;
	R_DMA0_TAR_ADDR = (INT32U)dest;
	R_DMA0_TX_COUNT = len>>1;
	R_DMA0_CTRL     = C_DMA_CTRL_SINGLE_ACCESS|C_DMA_CTRL_SINGLE_TRANS|C_DMA_CTRL_SRC_INCREASE|C_DMA_CTRL_DEST_INCREASE 
	                 |C_DMA_CTRL_SOFTWARE|C_DMA_CTRL_16BIT|C_DMA_CTRL_INT|C_DMA_CTRL_NORMAL_INT|C_DMA_CTRL_ENABLE;
	
	
	while((R_DMA_INT&C_DMA0_INT_PEND) == 0);
	if (R_DMA_INT & C_DMA0_TIMEOUT) {
		return C_DMA0_TIMEOUT;
	}
	R_DMA_INT = C_DMA0_INT_PEND;
		
	return STATUS_OK;
#else
	gp_memcpy((INT8S *)dest, (INT8S *)src, len);
	return STATUS_OK;
#endif
}


INT32S dma2_transfer_data(INT32U dest, INT32U src, INT32U len, INT8U aes_mode)
{
#if 1
	R_DMA2_CTRL	= C_DMA_CTRL_RESET; /* reset dma */
	R_DMA_INT = R_DMA_INT; /* clear interrupt flag */
	
	R_DMA2_MISC &= ~(C_DMA_MISC_TIMEOUT_MASK); /* set time-out value */
	R_DMA2_MISC |= 0xFF << C_DMA_MISC_TIMEOUT_SHIFT; /* 1 sec */
	
#if 1
        R_AES_KEY0 = 0x12222223;//0x10246A0C;
        R_AES_KEY1 = 0x22222222;//0xf0003890;
        R_AES_KEY2 = 0x22222222;//0x09000001;
        R_AES_KEY3 = 0x42222225;//0x70303830;
#else
        R_AES_KEY0 = 0x70303830;//0x10246A0C;
        R_AES_KEY1 = 0x09000001;//0xf0003890;
        R_AES_KEY2 = 0xf0003890;//0x09000001;
        R_AES_KEY3 = 0x10246A0C;//0x70303830;
#endif

        R_AES_LOAD_KEY = 0x01;
	if(aes_load_key_wait(5))
          return -1;
	if(aes_mode)
          R_AES_LOAD_KEY = 0x1100; // Encrypt
	else
          R_AES_LOAD_KEY = 0x1000; //	Discrypt	
        
        
        cache_drain_range(src, len);
        
	/* set DMA 0 for transfering data */
	R_DMA2_SRC_ADDR = (INT32U)src;
	R_DMA2_TAR_ADDR = (INT32U)dest;
	R_DMA2_TX_COUNT = len>>2;
	R_DMA2_CTRL     = C_DMA_CTRL_SINGLE_ACCESS|C_DMA_CTRL_SINGLE_TRANS|C_DMA_CTRL_SRC_INCREASE|C_DMA_CTRL_DEST_INCREASE 
	                 |C_DMA_CTRL_SOFTWARE|C_DMA_CTRL_32BIT|C_DMA_CTRL_NORMAL_INT|C_DMA_CTRL_ENABLE;
	
	
	while((R_DMA_INT&C_DMA2_INT_PEND) == 0);
	if (R_DMA_INT & C_DMA2_TIMEOUT) {
		return C_DMA2_TIMEOUT;
	}
	R_DMA_INT |= C_DMA2_INT_PEND;
	
        cache_invalid_range(dest, len);
	
	return STATUS_OK;
#else
	memcpy((void *)dest, (void *)src, len);
	return STATUS_OK;
#endif
}


#endif
