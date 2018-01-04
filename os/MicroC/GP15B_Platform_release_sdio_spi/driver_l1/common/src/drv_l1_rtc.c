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
#include "drv_l1_rtc.h"
#include "drv_l1_sfr.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"

#if (defined _DRV_L1_RTC) && (_DRV_L1_RTC == 1)
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// internal rtc
#define RTC_RTCEN			(1 << 15)  /* RTC enable */
#define RTC_SCHSEL			(7 <<  0)  /* schedule time period selection */

#define RTC_SEC_BUSY		(1 << 15)  /* RTC second busy flag */
#define RTC_MIN_BUSY		(1 << 14)  /* RTC min busy flag */
#define RTC_HR_BUSY			(1 << 13)  /* RTC hour busy flag */

#define RTC_ALMEN			(1 << 10)  /* alarm function enable */
#define RTC_HMSEN			(1 <<  9)  /* H/M/S function enable */
#define RTC_SCHEN			(1 <<  8)  /* scheduler function enbale */

#define RTC_ALM_IEN			(1 << 10)  /* alarm interrupt enbale */
#define RTC_SCH_IEN			(1 <<  8)  /* scheduler interrupt enbale */
#define RTC_DAY_IEN			(1 <<  4)  /* hour interrupt enbale */
#define RTC_HR_IEN			(1 <<  3)  /* hour interrupt enbale */
#define RTC_MIN_IEN			(1 <<  2)  /* min interrupt enbale */
#define RTC_SEC_IEN			(1 <<  1)  /* alarm interrupt enbale */
#define RTC_HALF_SEC_IEN	(1 <<  0)  /* alarm interrupt enbale */

// external rtc
#define EXT_RTC_SEC_IEN			(1 <<  0)  /* second interrupt enbale */
#define EXT_RTC_ALARM_IEN		(1 <<  1)  /* alarm interrupt enbale */
#define EXT_RTC_WAKEUP_IEN		(1 <<  2)  /* wakeup interrupt enbale */

#define R_EXTRTC_CTRL			0x00
#define R_EXTRTC_RRP			0x01
#define R_EXTRTC_RELIABLE		0x02
#define R_EXTRTC_LVR			0x03

#define R_EXTRTC_LOAD_TIME0		0x10
#define R_EXTRTC_LOAD_TIME1		0x11
#define R_EXTRTC_LOAD_TIME2		0x12
#define R_EXTRTC_LOAD_TIME3		0x13
#define R_EXTRTC_LOAD_TIME4		0x14
#define R_EXTRTC_LOAD_TIME5		0x15

#define R_EXTRTC_ALARM0			0x21
#define R_EXTRTC_ALARM1			0x22
#define R_EXTRTC_ALARM2			0x23
#define R_EXTRTC_ALARM3			0x24
#define R_EXTRTC_ALARM4			0x25

#define R_EXTRTC_TIME0			0x30
#define R_EXTRTC_TIME1			0x31
#define R_EXTRTC_TIME2			0x32
#define R_EXTRTC_TIME3			0x33
#define R_EXTRTC_TIME4			0x34
#define R_EXTRTC_TIME5			0x35

#define R_EXTRTC_INT_STATUS		0x40
#define R_EXTRTC_INT_ENABLE		0x50

#define R_EXTRTC_PWM_CTRL       0x60
#define R_EXTRTC_PWM0_PERIOD_L  0x62
#define R_EXTRTC_PWM0_PERIOD_H  0x63
#define R_EXTRTC_PWM0_PRELOAD_L 0x64
#define R_EXTRTC_PWM0_PRELOAD_H 0x65
#define R_EXTRTC_PWM1_PERIOD_L  0x66
#define R_EXTRTC_PWM1_PERIOD_H  0x67
#define R_EXTRTC_PWM1_PRELOAD_L 0x68
#define R_EXTRTC_PWM1_PRELOAD_H 0x69


#define REALIBABLE 				0x15


#define RTC_EN				0xFFFFFFFF
#define RTC_DIS				0
#define RTC_EN_MASK			0xFF
#define RTC_BUSY			-1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CHECK_RETURN(x) \
{\
	ret = x;\
	if(ret < 0) {\
		goto __exit;\
	}\
}\

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32S idp_rtc_set_reliable(INT8U byReliable);
static INT32S idp_rtc_get_reliable(INT8U *pbyReliable);
static INT32S idp_rtc_read(INT8U addr, INT8U *data);
static INT32S idp_rtc_write(INT8U addr, INT8U data);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern INT32U MCLK;
void (*rtc_user_isr[RTC_INT_MAX])(void);
#if _OPERATING_SYSTEM == 1	
static OS_EVENT *Rtc_Sema = NULL;
#endif

static void rtc_isr(void)
{
	INT32U int_status = R_RTC_INT_STATUS;
	INT32U int_ctrl = R_RTC_INT_CTRL;
	INT32U int_en = int_status & int_ctrl;
	
	if (int_en & RTC_HALF_SEC_IEN) {
		R_RTC_INT_STATUS = RTC_HALF_SEC_IEN;
		if (rtc_user_isr[RTC_HSEC_INT_INDEX]) {
			(*rtc_user_isr[RTC_HSEC_INT_INDEX])();
		}
	}

	if (int_en & RTC_SEC_IEN) {
		R_RTC_INT_STATUS = RTC_SEC_IEN;
		if (rtc_user_isr[RTC_SEC_INT_INDEX]) {
			(*rtc_user_isr[RTC_SEC_INT_INDEX])();
		}
	}

	if (int_en & RTC_MIN_IEN) {
		R_RTC_INT_STATUS = RTC_MIN_IEN;
		if (rtc_user_isr[RTC_MIN_INT_INDEX]) {
			(*rtc_user_isr[RTC_MIN_INT_INDEX])();
		}
	}

	if (int_en & RTC_HR_IEN) {
		R_RTC_INT_STATUS = RTC_HR_IEN;
		if (rtc_user_isr[RTC_HR_INT_INDEX]) {
			(*rtc_user_isr[RTC_HR_INT_INDEX])();
		}
	}

	if (int_en & RTC_DAY_IEN) {
		R_RTC_INT_STATUS = RTC_DAY_IEN;
		if (rtc_user_isr[RTC_DAY_INT_INDEX]) {
			(*rtc_user_isr[RTC_DAY_INT_INDEX])();
		}
	}

	if (int_en & RTC_SCH_IEN) {
		R_RTC_INT_STATUS = RTC_SCH_IEN;
		if (rtc_user_isr[RTC_SCH_INT_INDEX]) {
			(*rtc_user_isr[RTC_SCH_INT_INDEX])();
		}
	}

	if (int_en & RTC_ALM_IEN) {
		R_RTC_INT_STATUS = RTC_ALM_IEN;
		if (rtc_user_isr[RTC_ALM_INT_INDEX]) {
			(*rtc_user_isr[RTC_ALM_INT_INDEX])();
		}
	}
}

static void idp_rtc_isr(void)
{
	INT8U status;
	INT8U enable;
	INT32S ret;

	ret = idp_rtc_read(R_EXTRTC_INT_STATUS, &status);
	if(ret < 0) {
		goto __fail;
	}
	
	ret = idp_rtc_read(R_EXTRTC_INT_ENABLE, &enable);
	if(ret < 0) {
		goto __fail;
	}
	
	//clear flag
	status &= enable;
	idp_rtc_write(R_EXTRTC_INT_STATUS, 0x00);
		
	if (status & EXT_RTC_SEC_IEN) {
		if (rtc_user_isr[EXT_RTC_SEC_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_SEC_INT_INDEX])();
		}
	}

	if (status & EXT_RTC_ALARM_IEN) {
		if (rtc_user_isr[EXT_RTC_ALARM_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_ALARM_INT_INDEX])();
		}
	}

	if (status & EXT_RTC_WAKEUP_IEN) {
		if (rtc_user_isr[EXT_RTC_WAKEUP_INT_INDEX]) {
			(*rtc_user_isr[EXT_RTC_WAKEUP_INT_INDEX])();
		}
	}
	return;
	
__fail:	
	idp_rtc_write(R_EXTRTC_INT_STATUS, 0x00);
	return;
}


static void drv_l1_rtc_lock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT8U err;
	
	OSSemPend(Rtc_Sema, 0, &err);
#endif	
}

static void drv_l1_rtc_unlock(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSemPost(Rtc_Sema);
#endif	
}

/**
 * @brief	init rtc and idp rtc
 * @param 	none
 * @return 	none
 */
void drv_l1_rtc_init(void)
{
	t_rtc rtc_time;

#if _OPERATING_SYSTEM == _OS_UCOS2	
	if(Rtc_Sema == 0) {
		Rtc_Sema = OSSemCreate(1);
	}
#endif
	
	// internal rtc init
	R_RTC_CTRL= 0;							// disable all RTC function
	R_RTC_CTRL |= RTC_HMSEN | RTC_RTCEN;	// enable RTC
	R_RTC_INT_STATUS = 0x3F;				// clear int 

	// register internal RTC isr
	vic_irq_register(VIC_ALM_SCH_HMS, rtc_isr);
	vic_irq_enable(VIC_ALM_SCH_HMS);
	
	// init external rtc
	drv_l1_idp_rtc_init();
	
	// get external rtc time and set to internal rtc time
	drv_l1_idp_rtc_time_get(&rtc_time);
	drv_l1_rtc_time_set(&rtc_time);	
}

/**
 * @brief	set rtc isr callback
 * @param 	int_idx[in]: int index
 * @param 	user_isr[in]: callback function
 * @return 	none
 */
INT32S drv_l1_rtc_callback_set(RTC_INT_INDEX int_idx, void (*user_isr)(void))
{
	INT32S status = STATUS_OK;

	if (int_idx > RTC_INT_MAX) {
		return STATUS_FAIL;
	}

#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedLock();
#endif

	if (rtc_user_isr[int_idx] != 0) {
		status = STATUS_FAIL;
	}
	else {
		rtc_user_isr[int_idx] = user_isr;
	}
	
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#endif
	return status;
}

/**
 * @brief	clear rtc isr callback
 * @param 	int_idx[in]: int index
 * @return 	none
 */
INT32S drv_l1_rtc_callback_clear(RTC_INT_INDEX int_idx)
{
	if (int_idx > RTC_INT_MAX) {
		return STATUS_FAIL;
	}
	
#if _OPERATING_SYSTEM == _OS_UCOS2				// Soft Protect for critical section
	OSSchedLock();
#endif
	
	rtc_user_isr[int_idx] = 0;
	
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#endif

	return STATUS_OK;
}

/**
 * @brief	set rtc int enable or disable
 * @param 	int_idx[in]: int index
 * @param 	enable[in]: enable or disable
 * @return 	none
 */
INT32S drv_l1_rtc_int_set(RTC_INT_INDEX int_idx, INT32U enable)
{
	INT32S ret = STATUS_OK;
	
	switch(int_idx)
	{
	case RTC_HSEC_INT_INDEX:
	case RTC_SEC_INT_INDEX:
	case RTC_MIN_INT_INDEX:
	case RTC_HR_INT_INDEX:
	case RTC_DAY_INT_INDEX:
		if(enable) {
			R_RTC_INT_STATUS = (1 << int_idx);
			R_RTC_INT_CTRL |= (1 << int_idx);
		} else {
			R_RTC_INT_CTRL &= ~(1 << int_idx);
			R_RTC_INT_STATUS = (1 << int_idx);
		}
		break;
		
	case RTC_ALM_INT_INDEX:
		if(enable) {
			R_RTC_CTRL |= (1 << 10);
			R_RTC_INT_STATUS = (1 << 10);
			R_RTC_INT_CTRL |= (1 << 10);
		} else {
			R_RTC_CTRL &= ~(1 << 10);
			R_RTC_INT_CTRL &= ~(1 << 10);
			R_RTC_INT_STATUS = (1 << 10);
		}
		break;
	
	case RTC_SCH_INT_INDEX:
		if(enable) {
			R_RTC_INT_STATUS = (1 << 8);
			R_RTC_INT_CTRL |= (1 << 8);
		} else {
			R_RTC_INT_CTRL &= ~(1 << 8);
			R_RTC_INT_STATUS = (1 << 8);
		}
		break;
		
	case EXT_RTC_SEC_INT_INDEX:
	case EXT_RTC_ALARM_INT_INDEX:
	case EXT_RTC_WAKEUP_INT_INDEX:
		ret = drv_l1_idp_rtc_int_set(int_idx, enable);
		break;			
		
	default:
		ret = STATUS_FAIL;
	}

	return ret;
}

/**
 * @brief	set rtc schedule enable or disable
 * @param 	enable[in]: enable or disable
 * @param 	freq[in]: frequency
 * @return 	none
 */
void drv_l1_rtc_schedule_enable(INT32U enable, RTC_SCH_PERIOD freq)
{
	if(enable) {
		R_RTC_CTRL |= RTC_EN;
		R_RTC_CTRL &= ~RTC_SCHSEL;
		R_RTC_CTRL |= freq;
		R_RTC_CTRL |= RTC_SCHEN;
		R_RTC_INT_CTRL |= RTC_SCH_IEN;
	} else {
		R_RTC_CTRL &= ~RTC_SCHEN;
		R_RTC_INT_CTRL &= ~RTC_SCH_IEN;
	}
}

/**
 * @brief	set rtc time
 * @param 	rtc_time[in]: time 
 * @return 	none
 */
void drv_l1_rtc_time_set(t_rtc *rtc_time)
{	
	vic_irq_disable(VIC_ALM_SCH_HMS);
#if _OPERATING_SYSTEM == _OS_UCOS2				
	OSSchedLock();
#endif

	R_RTC_MIN = (INT32U)rtc_time->rtc_min;
	while(R_RTC_BUSY & RTC_MIN_BUSY);

	R_RTC_HOUR = (INT32U)rtc_time->rtc_hour;
	while(R_RTC_BUSY & RTC_HR_BUSY);

	R_RTC_SEC = (INT32U)rtc_time->rtc_sec;
	while(R_RTC_BUSY & RTC_SEC_BUSY);

#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#endif
	vic_irq_enable(VIC_ALM_SCH_HMS);
}

/**
 * @brief	set rtc alarm time
 * @param 	rtc_time[in]: time 
 * @return 	none
 */
void drv_l1_rtc_alarm_set(t_rtc *rtc_time)
{
	vic_irq_disable(VIC_ALM_SCH_HMS);
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedLock();
#endif

	R_RTC_ALARM_SEC = (INT32U)rtc_time->rtc_sec;
	R_RTC_ALARM_MIN = (INT32U)rtc_time->rtc_min;
	R_RTC_ALARM_HOUR = (INT32U)rtc_time->rtc_hour;

#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#endif
	vic_irq_enable(VIC_ALM_SCH_HMS);
}

/**
 * @brief	get rtc time
 * @param 	rtc_time[out]: time 
 * @return 	none
 */
void drv_l1_rtc_time_get(t_rtc *rtc_time)
{
	vic_irq_disable(VIC_ALM_SCH_HMS);
#if _OPERATING_SYSTEM == _OS_UCOS2				// Soft Protect for critical section
	OSSchedLock();
#endif

	rtc_time->rtc_sec = R_RTC_SEC;
	rtc_time->rtc_min = R_RTC_MIN;
	rtc_time->rtc_hour = R_RTC_HOUR;

#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSchedUnlock();
#endif
	vic_irq_enable(VIC_ALM_SCH_HMS);
}

/**
 * @brief	init rtc and idp rtc
 * @param 	none
 * @return 	none
 */
void drv_l1_idp_rtc_init(void)
{
	INT8U byReliable=0;
	
	// enable ext rtc clock 
	R_EXT_RTC_CTRL = 0x01;					

	// iRTC serial command clock divider.
	if(MCLK == 144000000) {
		(*((volatile INT32U *) 0xC009002C)) = 0x20;
	}
	
	// rtc enable and Up Count 
	idp_rtc_write(R_EXTRTC_CTRL, 0x15);     

	// check reliable
	if (idp_rtc_get_reliable(&byReliable) < 0) {
		idp_rtc_set_reliable(REALIBABLE);	
	}
	 	
	if (byReliable != REALIBABLE) {
		idp_rtc_set_reliable(REALIBABLE);
		idp_rtc_get_reliable(&byReliable);
		//if (byReliable != REALIBABLE) {
		//	while(1);
		//}
	} 
	
	// register external RTC isr
	vic_irq_register(VIC_RTC, idp_rtc_isr);
	vic_irq_enable(VIC_RTC);
}

INT32S drv_l1_idp_rtc_int_set(RTC_INT_INDEX int_idx, INT32U enable)
{
	INT8U EnBit;
	INT32S ret;
			
	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);
		
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_INT_ENABLE, &EnBit));
	if(enable) {
		EnBit |= (1 << (int_idx - EXT_RTC_SEC_INT_INDEX));
		CHECK_RETURN(idp_rtc_write(R_EXTRTC_INT_STATUS, EnBit));
    	CHECK_RETURN(idp_rtc_write(R_EXTRTC_INT_ENABLE, EnBit));
    } else {
		EnBit &= ~(1 << (int_idx - EXT_RTC_SEC_INT_INDEX));
    	CHECK_RETURN(idp_rtc_write(R_EXTRTC_INT_ENABLE, EnBit));
		CHECK_RETURN(idp_rtc_write(R_EXTRTC_INT_STATUS, EnBit));
	}

__exit:
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
	return ret;
}

/**
 * @brief	set idp rtc time
 * @param 	rtc_time[in]: time 
 * @return 	none
 */
INT32S drv_l1_idp_rtc_time_set(t_rtc *rtc_time)
{
	INT32S ret;
	INT64U sec;
		
	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);	

	sec = (rtc_time->rtc_day * 60 * 60 * 24) + 
			(rtc_time->rtc_hour * 60 * 60) + 
			(rtc_time->rtc_min * 60) + 
			rtc_time->rtc_sec; 

	sec <<= 15;
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME0, (sec >> 0) & 0xFF)); 
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME1, (sec >> 8) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME2, (sec >> 16) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME3, (sec >> 24) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME4, (sec >> 32) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_LOAD_TIME5, (sec >> 40) & 0xFF));

__exit:
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
	return ret;
}

/**
 * @brief	get idp rtc time
 * @param 	rtc_time[out]: time 
 * @return 	none
 */
INT32S drv_l1_idp_rtc_time_get(t_rtc *rtc_time)
{
	INT8U temp;
	INT32S ret;
	INT64U sec;
		
	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);
	
	sec = 0;
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME0, &temp));
	sec += (INT64U)temp;
	
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME1, &temp));
	sec += (INT64U)temp << 8;

	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME2, &temp));
	sec += (INT64U)temp << 16;

	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME3, &temp));
	sec += (INT64U)temp << 24;

	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME4, &temp));
	sec += (INT64U)temp << 32;

	CHECK_RETURN(idp_rtc_read(R_EXTRTC_TIME5, &temp));
	sec += (INT64U)temp << 40;

	sec >>= 15;
	rtc_time->rtc_day = (INT64U)sec/(60*60*24);

	sec -= (INT64U)(rtc_time->rtc_day * 60 * 60 * 24);
	rtc_time->rtc_hour = (INT64U)sec / (60*60);

	sec -= (INT64U)(rtc_time->rtc_hour * 60 * 60);
	rtc_time->rtc_min = (INT64U)sec / 60;

	sec -= (INT64U)(rtc_time->rtc_min * 60);
	rtc_time->rtc_sec = sec;

__exit:
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
	return ret;
}

/**
 * @brief	set idp rtc alarm time
 * @param 	rtc_time[in]: time 
 * @return 	none
 */
INT32S drv_l1_idp_rtc_alarm_set(t_rtc *rtc_time)
{
	INT32S ret;
	INT64U sec;
	
	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);

	sec = (rtc_time->rtc_day * 60 * 60 * 24) + 
			(rtc_time->rtc_hour * 60 * 60) + 
			(rtc_time->rtc_min * 60) + 
			rtc_time->rtc_sec; 

	sec <<= 15;
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_ALARM0, (sec >> 8) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_ALARM1, (sec >> 16) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_ALARM2, (sec >> 24) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_ALARM3, (sec >> 32) & 0xFF));
	CHECK_RETURN(idp_rtc_write(R_EXTRTC_ALARM4, (sec >> 40) & 0xFF));
	
__exit:
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
	return ret;
}

/**
 * @brief	get idp rtc alarm time
 * @param 	rtc_time[out]: time 
 * @return 	none
 */
INT32S drv_l1_idp_rtc_alarm_get(t_rtc *rtc_time)
{
	INT8U temp;
	INT32S ret;
	INT64U sec;

	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);
		
	sec = 0;
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_ALARM0, &temp));
	sec += (INT64U)temp << 8;
	
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_ALARM1, &temp));
	sec += (INT64U)temp << 16;
	
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_ALARM2, &temp));
	sec += (INT64U)temp << 24;
	
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_ALARM3, &temp));
	sec += (INT64U)temp << 32;
	
	CHECK_RETURN(idp_rtc_read(R_EXTRTC_ALARM4, &temp));
	sec += (INT64U)temp << 40;
	
	sec >>= 15;
	rtc_time->rtc_day = (INT64U)sec/(60*60*24);

	sec -= (INT64U)(rtc_time->rtc_day * 60 * 60 * 24);
	rtc_time->rtc_hour = sec / (60*60);

	sec -= (INT64U)(rtc_time->rtc_hour * 60 * 60);
	rtc_time->rtc_min = sec / 60;

	sec -= (INT64U)(rtc_time->rtc_min * 60);
	rtc_time->rtc_sec = sec;

__exit:
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
	return ret;
}

INT32S drv_l1_idp_rtc_pwm0_enable(INT8U byPole, INT16U wPeriod, INT16U wPreload, INT8U byEnable)
{
    INT8U byValue;
    INT32S ret;
    
	drv_l1_rtc_lock();
	vic_irq_disable(VIC_RTC);
	    
    if (byEnable) {
        // period
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM0_PERIOD_L, wPeriod&0xFF));
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM0_PERIOD_H, (wPeriod>>8)&0xFF));
        // preload
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM0_PRELOAD_L, wPreload&0xFF));
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM0_PRELOAD_H, (wPreload>>8)&0xFF));
    }
    
    CHECK_RETURN(idp_rtc_read(R_EXTRTC_PWM_CTRL, &byValue));
	if (byEnable) {
    	byValue |= 0x01 ;
    } else {
        byValue &= ~0x01 ;
    }
    
    if (byPole) {
    	byValue |= 0x04 ;
    } else {
        byValue &= ~0x04 ;
    }
    
    CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM_CTRL, byValue)) ;
    ret = STATUS_OK;
    
__exit:    
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
    return ret;
}

INT32S drv_l1_idp_rtc_pwm1_enable(INT8U byPole, INT16U wPeriod, INT16U wPreload, INT8U byEnable)
{
    INT8U byValue;
    INT32S ret;
    
	drv_l1_rtc_lock();   
    vic_irq_disable(VIC_RTC);
    
    if (byEnable) {
        // period
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM1_PERIOD_L, wPeriod&0xFF));
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM1_PERIOD_H, (wPeriod>>8)&0xFF));
        // preload
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM1_PRELOAD_L, wPreload&0xFF));
        CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM1_PRELOAD_H, (wPreload>>8)&0xFF));
    }
    
    CHECK_RETURN(idp_rtc_read(R_EXTRTC_PWM_CTRL, &byValue));
	if (byEnable) {
    	byValue |= 0x02 ;
    } else {
        byValue &= ~0x02 ;
    }
    
    if (byPole) {
    	byValue |= 0x08 ;
    } else {
        byValue &= ~0x08 ;
    }
    
    CHECK_RETURN(idp_rtc_write(R_EXTRTC_PWM_CTRL, byValue)) ;
    ret = STATUS_OK;
    
__exit:    
	vic_irq_enable(VIC_RTC);
	drv_l1_rtc_unlock();
    return ret;
}

INT32S drv_l1_idp_rtc_wakeup_int_enable(INT8U byEnable, void (*ext_rtc_wakeup_isr)(void))
{
    INT32S ret;	

	drv_l1_rtc_callback_clear(EXT_RTC_WAKEUP_INT_INDEX);
	drv_l1_rtc_callback_set(EXT_RTC_WAKEUP_INT_INDEX, ext_rtc_wakeup_isr);
	ret = drv_l1_idp_rtc_int_set(EXT_RTC_WAKEUP_INT_INDEX, byEnable);
	if(ret < 0) {
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

static INT32S idp_rtc_set_reliable(INT8U byReliable)
{
    INT32S ret;
    
    CHECK_RETURN(idp_rtc_write(R_EXTRTC_RELIABLE, byReliable));    
__exit:    
    return ret;
}

static INT32S idp_rtc_get_reliable(INT8U *pbyReliable)
{
    INT32S ret;
    CHECK_RETURN(idp_rtc_read(R_EXTRTC_RELIABLE, pbyReliable));    
__exit:    
    return ret;
}

static INT32S idp_rtc_read(INT8U addr, INT8U *data)
{
	INT32S N = 100;
	
	R_EXT_RTC_ADDR = addr;
	R_EXT_RTC_REQ = 0x02;	// triger read	

	//wait ready
	do {
		if(R_EXT_RTC_READY & 0x01) {
			*data = R_EXT_RTC_RD_DATA;
			return STATUS_OK;
		} else {
			drv_msec_wait(1);
		} 
	} while(--N > 0);
	
	return STATUS_FAIL;
}

static INT32S idp_rtc_write(INT8U addr, INT8U data)
{
	INT32S N = 100;
	
	R_EXT_RTC_ADDR = addr;
	R_EXT_RTC_WR_DATA = data;
	R_EXT_RTC_REQ |= 0x01; //triger write

	//wait ready
	do {
		if(R_EXT_RTC_READY & 0x01) { // check if RTC controller is free
			break;
		} else {
			drv_msec_wait(1);
		} 
	} while(--N > 0);
		
	if(N == 0) {
		return STATUS_FAIL;
	}

	switch(addr)
	{
	case 0x00:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x40:
	case 0x50:
		N = 100;
		do {
			idp_rtc_read(R_EXTRTC_CTRL, &data); 
			if((data & 0x10) == 0) { // check if RTC macro is free
				break;
			} else {
				drv_msec_wait(1);
			} 
		} while(--N > 0);
		
		if(N == 0) {
			return STATUS_FAIL;
		}
		break;
		
	default:
		break;
	}
	
	return STATUS_OK;
}
#endif //(defined _DRV_L1_RTC) && (_DRV_L1_RTC == 1)
