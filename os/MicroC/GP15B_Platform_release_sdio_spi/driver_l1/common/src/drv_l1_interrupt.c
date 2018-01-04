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
#include "drv_l1_interrupt.h"
#if _OPERATING_SYSTEM != _OS_NONE	
#include "os.h"
#endif 

#if (defined _DRV_L1_INTERRUPT) && (_DRV_L1_INTERRUPT == 1)
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
void (*irq_isr[VIC_MAX_IRQ])(void);
void (*fiq_isr[VIC_MAX_FIQ])(void);


/**
 * @brief   set global interrupt disable
 * @param   none
 * @return 	global interrupt value
 */
INT32U vic_global_mask_set(void)
{
	INT32S old_mask;

  #if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	OSSchedLock();
  #endif
	old_mask = R_INT_GMASK & C_VIC_GLOBAL_MASK_SET;
	R_INT_GMASK = C_VIC_GLOBAL_MASK_SET;
  #if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
  #endif

	return old_mask;
}

/**
 * @brief   set global interrupt enable
 * @param   old_mask: make value
 * @return 	none
 */
void vic_global_mask_restore(INT32U old_mask)
{
  #if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	OSSchedLock();
  #endif
	if (old_mask & C_VIC_GLOBAL_MASK_SET) {
		R_INT_GMASK = C_VIC_GLOBAL_MASK_SET;
	} else {
		R_INT_GMASK = C_VIC_GLOBAL_MASK_CLEAR;
	}
  #if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
  #endif
}

/**
 * @brief   irq service function register
 * @param   irc_num: irq vector number
 * @param   isr: service function
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_register(INT32U irc_num, void (*isr)(void))
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		irq_isr[irc_num] = isr;
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service function unregister
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_unregister(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		irq_isr[irc_num] = NULL;
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service vector enable
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_enable(INT32U irc_num)
{
	INT32S old_mask;

	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num = VIC_MAX_IRQ - irc_num - 1;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		old_mask = (R_INT_IRQMASK >> irc_num) & 0x1;
		R_INT_IRQMASK &= ~(1 << irc_num);
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return old_mask;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service vector disable
 * @param   irc_num: irq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_irq_disable(INT32U irc_num)
{
	INT32S old_mask;

	if (irc_num && (irc_num<VIC_MAX_IRQ)) {
		irc_num = VIC_MAX_IRQ - irc_num - 1;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		old_mask = (R_INT_IRQMASK >> irc_num) & 0x1;
		R_INT_IRQMASK |= (1 << irc_num);
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return old_mask;
	}

	return STATUS_FAIL;
}

/**
 * @brief   fiq service function register
 * @param   irc_num: fiq vector number
 * @param   isr: fiq service function 
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_register(INT32U irc_num, void (*isr)(void))
{
	if (irc_num && (irc_num<VIC_MAX_FIQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		fiq_isr[irc_num] = isr;
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   fiq service function unregister
 * @param   irc_num: fiq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_unregister(INT32U irc_num)
{
	if (irc_num && (irc_num<VIC_MAX_FIQ)) {
		irc_num--;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		fiq_isr[irc_num] = NULL;
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return STATUS_OK;
	}

	return STATUS_FAIL;
}

/**
 * @brief   fiq service vector enable
 * @param   irc_num: fiq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_enable(INT32U irc_num)
{
	INT32S old_mask;
	
	if (irc_num && (irc_num<VIC_MAX_FIQ)) {
		irc_num = VIC_MAX_FIQ - irc_num - 1;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		old_mask = (R_INT_FIQMASK >> irc_num) & 0x1;
		R_INT_FIQMASK &= ~(1 << irc_num);
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return old_mask;
	}

	return STATUS_FAIL;
}

/**
 * @brief   fiq service vector disable
 * @param   irc_num: fiq vector number
 * @return 	result: >=0 is success, <0 is fail.
 */
INT32S vic_fiq_disable(INT32U irc_num)
{
	INT32S old_mask;

	if (irc_num && (irc_num<VIC_MAX_FIQ)) {
		irc_num = VIC_MAX_FIQ - irc_num - 1;

	  #if _OPERATING_SYSTEM != _OS_NONE			// Soft Protect for critical section
		OSSchedLock();
	  #endif
		old_mask = (R_INT_FIQMASK >> irc_num) & 0x1;
		R_INT_FIQMASK |= (1 << irc_num);
	  #if _OPERATING_SYSTEM != _OS_NONE
		OSSchedUnlock();
	  #endif

		return old_mask;
	}

	return STATUS_FAIL;
}

/**
 * @brief   irq service routine
 * @param   none
 * @return 	none
 */
#if _OPERATING_SYSTEM != _OS_NONE
void irq_dispatcher(void)
#else
void IRQ irq_dispatcher(void)
#endif
{
	INT32U vector;

	if (R_INT_GMASK & C_VIC_GLOBAL_MASK_SET) {		// Spurious interrupt. Do nothing.
		return;
	}

	vector = R_INT_IRQNUM;
	while (vector) {
		if (vector < VIC_MAX_IRQ) {
			vector--;
			if (irq_isr[vector]) {				// Check IRQ status register to see which interrupt source is set
				// TBD: Enable Device Protect and then disable Hard Protect here for nesting interrupt
				(*irq_isr[vector])();
				// TBD: Enable Hard Protect and then disable Device Protect here for nesting interrupt
			}
		}

		vector = R_INT_IRQNUM;				// Get next pending interrupt source
	}
}

/**
 * @brief   fiq service routine
 * @param   none
 * @return 	none
 */
#if _OPERATING_SYSTEM != _OS_NONE
void fiq_dispatcher(void)
#else
void IRQ fiq_dispatcher(void)
#endif
{
	INT32U vector;

	if (R_INT_GMASK & C_VIC_GLOBAL_MASK_SET) {		// Spurious interrupt. Do nothing.
		return;
	}

	vector = R_INT_FIQNUM;
	while (vector) {
		if (vector < VIC_MAX_FIQ) {
			vector--;
			if (fiq_isr[vector]) {				// Check FIQ status register to see which interrupt source is set
				// TBD: Enable Device Protect and then disable Hard Protect here for nesting interrupt
				(*fiq_isr[vector])();
				// TBD: Enable Hard Protect and then disable Device Protect here for nesting interrupt
			}
		}

		vector = R_INT_FIQNUM;
	}
}

/**
 * @brief   interrupt vector init
 * @param   none
 * @return 	none
 */
void vic_init(void)
{
	INT32U i;

	R_INT_IRQMASK = C_VIC_IRQ_MASK_SET_ALL;	// Mask all interrupt
	R_INT_FIQMASK = C_VIC_FIQ_MASK_SET_ALL;	// Mask all fast interrupt
	R_INT_GMASK = C_VIC_GLOBAL_MASK_CLEAR;	// Clear global mask
	R_INT_FIQMASK2 = C_VIC_FIQ_MASK2_SET_ALL;
	for (i=0; i<VIC_MAX_IRQ; i++) {
		irq_isr[i] = NULL;
	}
	for (i=0; i<VIC_MAX_FIQ; i++) {
		fiq_isr[i] = NULL;
	}

  #if _OPERATING_SYSTEM != _OS_NONE
  	register_exception_table(EXCEPTION_IRQ, (INT32U) OS_CPU_IRQ_ISR);
	register_exception_table(EXCEPTION_FIQ, (INT32U) OS_CPU_FIQ_ISR);
  #else
	register_exception_table(EXCEPTION_IRQ, (INT32U) irq_dispatcher);
	register_exception_table(EXCEPTION_FIQ, (INT32U) fiq_dispatcher);
  #endif
}
#endif
