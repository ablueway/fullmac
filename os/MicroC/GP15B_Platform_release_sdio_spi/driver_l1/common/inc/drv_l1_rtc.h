#ifndef __drv_l1_RTC_H__
#define __drv_l1_RTC_H__

#include "driver_l1.h"

typedef struct s_rtc
{
	INT32U rtc_sec;    /* seconds [0,59]  */
	INT32U rtc_min;    /* minutes [0,59]  */
	INT32U rtc_hour;   /* hours [0,23]  */
	INT32U rtc_day;    /* day count,[ 0,4095] */
} t_rtc;

typedef enum
{
	RTC_HSEC_INT_INDEX,
	RTC_SEC_INT_INDEX,
	RTC_MIN_INT_INDEX,
	RTC_HR_INT_INDEX,
	RTC_DAY_INT_INDEX,
	RTC_ALM_INT_INDEX,
	RTC_SCH_INT_INDEX,
	EXT_RTC_SEC_INT_INDEX,
	EXT_RTC_ALARM_INT_INDEX,
	EXT_RTC_WAKEUP_INT_INDEX,
	RTC_INT_MAX
} RTC_INT_INDEX;

typedef enum
{
	RTC_SCH_16HZ,
	RTC_SCH_32HZ,
	RTC_SCH_64HZ,
	RTC_SCH_128HZ,
	RTC_SCH_256HZ,
	RTC_SCH_512HZ,
	RTC_SCH_1024HZ,
	RTC_SCH_2048HZ
} RTC_SCH_PERIOD;

extern void drv_l1_rtc_init(void);
extern INT32S drv_l1_rtc_callback_set(RTC_INT_INDEX int_idx, void (*user_isr)(void));
extern INT32S drv_l1_rtc_callback_clear(RTC_INT_INDEX int_idx);
extern INT32S drv_l1_rtc_int_set(RTC_INT_INDEX int_idx, INT32U enable);
extern void drv_l1_rtc_schedule_enable(INT32U enable, RTC_SCH_PERIOD freq);
extern void drv_l1_rtc_time_set(t_rtc *rtc_time);
extern void drv_l1_rtc_time_get(t_rtc *rtc_time);
extern void drv_l1_rtc_alarm_set(t_rtc *rtc_time);
extern void drv_l1_rtc_time_get(t_rtc *rtc_time);

extern void drv_l1_idp_rtc_init(void);
extern INT32S drv_l1_idp_rtc_int_set(RTC_INT_INDEX int_idx, INT32U enable);
extern INT32S drv_l1_idp_rtc_time_set(t_rtc *rtc_time);
extern INT32S drv_l1_idp_rtc_time_get(t_rtc *rtc_time);
extern INT32S drv_l1_idp_rtc_alarm_set(t_rtc *rtc_time);
extern INT32S drv_l1_idp_rtc_alarm_get(t_rtc *rtc_time);

extern INT32S drv_l1_idp_rtc_pwm0_enable(INT8U byPole, INT16U wPeriod, INT16U wPreload, INT8U byEnable);
extern INT32S drv_l1_idp_rtc_pwm1_enable(INT8U byPole, INT16U wPeriod, INT16U wPreload, INT8U byEnable);
extern INT32S drv_l1_idp_rtc_wakeup_int_enable(INT8U byEnable, void (*ext_rtc_wakeup_isr)(void));
#endif 		/* __drv_l1_RTC_H__ */