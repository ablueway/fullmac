#ifndef __GPL32_CALENDAR_H__
#define __GPL32_CALENDAR_H__

#include "gplib.h"

typedef struct
{
    INT32S tm_sec;  /* 0-59 */
    INT32S tm_min;  /* 0-59 */
    INT32S tm_hour; /* 0-23 */
    INT32S tm_mday; /* 1-31 */
    INT32S tm_mon;  /* 1-12 */
    INT32S tm_year; /* year */
    INT32S tm_wday; /* 0-6 Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday */
}TIME_T;

extern INT32S calendar_init(void);
extern INT32S cal_time_get(TIME_T *tm);
extern INT32S cal_time_set(TIME_T *tm);
extern INT32S cal_alarm_set(INT32U enable, TIME_T *tm, void (*alarm_func)(void));
extern void cal_set_time_zone(INT8U t_zone);
#endif 		/* __GPL32_CALENDAR_H__ */