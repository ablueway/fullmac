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
#include "gplib_calendar.h"
#include "drv_l1_rtc.h"

#if (defined GPLIB_CALENDAR_EN) && (GPLIB_CALENDAR_EN == 1) 
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define AUG_8TH_2008         2454687 /* julian date for system init time */
#define DEC_31TH_1799        2378496 /* julian date at 1799/12/31 for conversion */
#define JAN_1ST_1900         2415021
#define DEC_31TH_1979        2444239 
#define DEC_31TH_2007        2454466
#define JAN_01TH_2012        2455928
#define JAN_01TH_2014        2456659
#define JAN_31TH_2014        2456689

/* time zone adj*/
#define SUPPORT_TIME_ZONE_ADJUST 0

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static INT32U date_to_jd(INT32S yyyy, INT32S mm, INT32S dd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U LAST_JULIAN_DATE = JAN_31TH_2014;
const INT8S monday[]= {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#if SUPPORT_TIME_ZONE_ADJUST == 1
static INT8U time_zone = 50;
const int timezone_adj[] =
{
      0         
    ,-(12*60+00)  /* 1(GMT-12:00) Eniwetok,Kwajalain                                         */
    ,-(11*60+00)  /* 2(GMT-11:00) Midway Island,Samoa                                        */
    ,-(10*60+00)  /* 3(GMT-10:00) Hawaii                                                     */
    ,-( 9*60+00)  /* 4(GMT-09:00) Alaska                                                     */
    ,-( 8*60+00)  /* 5(GMT-08:00) Pacific Time (US & Canada);Tijuana                         */
    ,-( 7*60+00)  /* 6(GMT-07:00) Arizona                                                    */
    ,-( 7*60+00)  /* 7(GMT-07:00) Mountain Time (US & Canada)                                */
    ,-( 6*60+00)  /* 8(GMT-06:00) Central Time (US & Canada)                                 */
    ,-( 6*60+00)  /* 9(GMT-06:00) Mexico City,Tegucigalpa                                    */
    ,-( 6*60+00)  /* 10(GMT-06:00) Saskatchewan                                               */
    ,-( 5*60+00)  /* 11(GMT-05:00) Bogota,Lima,Ouito                                          */
    ,-( 5*60+00)  /* 12(GMT-05:00) Eastern Time (US & Canada)                                 */
    ,-( 5*60+00)  /* 13(GMT-05:00) Indiana (East)                                             */
    ,-( 4*60+00)  /* 14(GMT-04:00) Atlantic Time(Canada)                                      */
    ,-( 4*60+00)  /* 15(GMT-04:00) Caracas,La Paz                                             */
    ,-( 4*60+00)  /* 16(GMT-04:00) Sasntiago                                                  */
    ,-( 3*60+30)  /* 17(GMT-03:30) Newfoundlasnd                                              */
    ,-( 3*60+00)  /* 18(GMT-03:00) Brasilia                                                   */
    ,-( 3*60+00)  /* 19(GMT-03:00) Buenos Aires,Georgetown                                    */
    ,-( 2*60+00)  /* 20(GMT-02:00) Mid-Atlantic                                               */
    ,-( 1*60+00)  /* 21(GMT-01:00) Azores,Cape Verde Is.                                      */
    ,0            /* 22(GMT)       Casablance, Monrovia                                       */
    ,0            /* 23(GMT)       Greenwich Mean Time:Dublin,Edinburgh,Lisbon,London         */
    ,( 1*60+00)   /* 24(GMT+01:00) Amsterdam,Berlin,Bern,Rome,Stockholm,Vienna                */
    ,( 1*60+00)   /* 25(GMT+01:00) Belgrade,Bratislava,Budapest,Ljubljana,Prague              */
    ,( 1*60+00)   /* 26(GMT+01:00) Brussels,Copenhagen,Madrid,Paris,Vilnius                   */
    ,( 1*60+00)   /* 27(GMT+01:00) Sarajevo,Skopje,Sofija,Warsaw,Zagreb                       */
    ,( 2*60+00)   /* 28(GMT+02:00) Athens,Istanbul,Minsk                                      */
    ,( 2*60+00)   /* 29(GMT+02:00) Bucharest                                                  */
    ,( 2*60+00)   /* 30(GMT+02:00) Cairo                                                      */
    ,( 2*60+00)   /* 31(GMT+02:00) Harare,Pretoria                                            */
    ,( 2*60+00)   /* 32(GMT+02:00) Helsinki,riga,Tallinn                                      */
    ,( 2*60+00)   /* 33(GMT+02:00) Jerusalem                                                  */
    ,( 3*60+00)   /* 34(GMT+03:00) Baghdad,kuwait,Riyadh                                      */
    ,( 3*60+00)   /* 35(GMT+03:00) Moscow,St.Petersburg,Volgograd                             */
    ,( 3*60+00)   /* 36(GMT+03:00) Nairobi                                                    */
    ,( 3*60+30)   /* 37(GMT+03:30) Tehran                                                     */
    ,( 4*60+00)   /* 38(GMT+04:00) Abu Dhabi,Muscat                                           */
    ,( 4*60+00)   /* 39(GMT+04:00) Baku,Tbilisi                                               */
    ,( 4*60+30)   /* 40(GMT+04:30) Kabul                                                      */
    ,( 5*60+00)   /* 41(GMT+05:00) Ekaterinburg                                               */
    ,( 5*60+00)   /* 42(GMT+05:00) Islamabad,Karachi,Tashkent                                 */
    ,( 5*60+30)   /* 43(GMT+05:30) Bombay,Calcutta,Madras,New Delhi                           */
    ,( 6*60+00)   /* 44(GMT+06:00) Astana,Almaty,Dhaka                                        */
    ,( 6*60+00)   /* 45(GMT+06:00) Colombo                                                    */
    ,( 7*60+00)   /* 46(GMT+07:00) Bangkok,Hanoi,Jakarta                                      */
    ,( 8*60+00)   /* 47(GMT+08:00) Beijing,Chongqing, Hong Kong,Urumqi                        */
    ,( 8*60+00)   /* 48(GMT+08:00) Perth                                                      */
    ,( 8*60+00)   /* 49(GMT+08:00) Singapore                                                  */
    ,( 8*60+00)   /* 50(GMT+08:00) Taipei                                                     */
    ,( 9*60+00)   /* 51(GMT+09:00) Osaka,Sapporo,Tokyo                                        */
    ,( 9*60+00)   /* 52(GMT+09:00) Seoul                                                      */
    ,( 9*60+00)   /* 53(GMT+09:00) Yakutsk                                                    */
    ,( 9*60+30)   /* 54(GMT+09:30) Adelaide                                                   */
    ,( 9*60+30)   /* 55(GMT+09:30) Darwin                                                     */
    ,(10*60+00)   /* 56(GMT+10:00) Brisbane                                                   */
    ,(10*60+00)   /* 57(GMT+10:00) Canberra,Melbourne,Sydney                                  */
    ,(10*60+00)   /* 58(GMT+10:00) Guarn,Port Moresby                                         */
    ,(10*60+00)   /* 59(GMT+10:00) Hobart                                                     */
    ,(10*60+00)   /* 60(GMT+10:00) Vladivostok                                                */
    ,(11*60+00)   /* 61(GMT+11:00) Magadan,Solomon Is. ,New Calcdonia                         */
    ,(12*60+00)   /* 62(GMT+12:00) Auckland,Wellington                                        */
    ,(12*60+00)   /* 63(GMT+12:00) Fiji,Kamchaka,Marshall Is.                                 */
};

#else
static INT8U time_zone = 0;
const int timezone_adj[] =
{
	0
};
#endif


static INT32U date_to_jd(INT32S yyyy, INT32S mm, INT32S dd)
{
	INT32S c,ya;
	INT32U j;
	
	if (mm > 2) {
        mm = mm - 3;
    }
    else {
        mm = mm + 9;
        yyyy = yyyy - 1;
    }

    c = yyyy / 100;
    ya = yyyy - 100 * c;
    j = (146097L * c) / 4 + (1461L * ya) / 4 + (153L * mm + 2) / 5 + dd + 1721119L;
    
    return j;		
}

static void day_count_callback(void)
{
	DBG_PRINT("day++\r\n");		
}

INT32S calendar_init(void)
{
	drv_l1_rtc_callback_set(RTC_DAY_INT_INDEX, day_count_callback);
	drv_l1_rtc_int_set(RTC_DAY_INT_INDEX, ENABLE);
	return STATUS_OK;
}

INT32S cal_time_get(TIME_T *tm)
{
    INT32U	s;
    INT32U	j;
    INT32U	gmts;
    t_rtc   r_time;
    INT32S  JD, y, m, d;
    BOOLEAN over;
   
    if (!tm) {
        return STATUS_FAIL;
    }

    if(drv_l1_idp_rtc_time_get(&r_time) < 0) {
    	return STATUS_FAIL;
    }
    
    j = r_time.rtc_day;
    
    if ((INT32S)j < 0) {
        tm->tm_sec = r_time.rtc_sec;
    	tm->tm_min = r_time.rtc_min;
    	tm->tm_hour = r_time.rtc_hour;
    	tm->tm_year = 2014;;
    	tm->tm_mon = 10;
   	   	tm->tm_mday = 31;
   	  	tm->tm_wday = 5;
    	return STATUS_FAIL;
    }

    j = (j * 24) + r_time.rtc_hour;
    j = (j * 60) + r_time.rtc_min;
    j = (j * 60) + r_time.rtc_sec;
    
	gmts = j;
    gmts += timezone_adj[time_zone] * 60;
    
    s = gmts;
#if 0
	if ((s & 0x80000000) == 0)
    {
        over = TRUE;
    }
    else
    {
        over = FALSE;
    }
#endif
	
	over = FALSE;
    
    if (over)
    {
        s += 16;
    }

    tm->tm_sec = (s % 60);
    s /= 60;

    if (over)
    {
        s += 28;
    }

    tm->tm_min = (s % 60);
    s /= 60;

    if (over)
    {
        s += 6;
    }

    tm->tm_hour = (s % 24);
    s /= 24;

    if (over)
    {
        s += 49710;
    }

    JD = s + LAST_JULIAN_DATE;

    tm->tm_wday = ((JD + 1) % 7);

    j = JD - 1721119;
    y = (4 * j - 1) / 146097;
    j = 4 * j - 1 - 146097 * y;
    d = j / 4;
    j = (4 * d + 3) / 1461;
    d = 4 * d + 3 - 1461 * j;
    d = (d + 4) / 4;
    m = (5 * d - 3) / 153;
    d = 5 * d - 3 - 153 * m;
    d = (d + 5) / 5;
    y = 100 * y + j;
    
    if (m < 10) {
        m = m + 3;
    }
    else {
        m = m - 9;
        y = y + 1;
    }
	
	if((y<2014)||(y>2099)){
		y = 2014;
		m = 1;
		d = 31;
	}

    tm->tm_year = y;
    tm->tm_mon = m;
    tm->tm_mday = d;
            
    return STATUS_OK;
}

INT32S cal_time_set(TIME_T *tm)
{
    INT32S c;
    INT32U j;
    t_rtc  r_time;
	
    if ((tm->tm_mon > 12 || tm->tm_mon < 1) || (tm->tm_mday > monday[tm->tm_mon] || tm->tm_mday < 1) ||
        (tm->tm_hour > 23 || tm->tm_hour < 0) || (tm->tm_min > 59 || tm->tm_min < 0) ||
        (tm->tm_sec > 59 || tm->tm_sec < 0) || (tm->tm_year < 1800)) {
        return STATUS_FAIL;
    }

	j = date_to_jd(tm->tm_year, tm->tm_mon, tm->tm_mday);

    c = tm->tm_sec + tm->tm_min*60 + tm->tm_hour*3600;
    c -= timezone_adj[time_zone] * 60;

	if (c < 0) {
		j--;
		c += 86400;
    }

    r_time.rtc_sec = c%60;
    c /= 60;
	r_time.rtc_min = c%60;
	c /= 60;
	r_time.rtc_hour = c;
	r_time.rtc_day = j - LAST_JULIAN_DATE;
	
    drv_l1_rtc_time_set(&r_time);
	return drv_l1_idp_rtc_time_set(&r_time);
}

INT32S cal_alarm_set(INT32U enable, TIME_T *tm, void (*alarm_isr)(void)) 
{
	TIME_T time;
	t_rtc rtc_time;
	INT32U cjd, tjd;
	
	cal_time_get(&time);
	cjd = date_to_jd(time.tm_year, time.tm_mon, time.tm_mday);
	tjd = date_to_jd(tm->tm_year, tm->tm_mon, tm->tm_mday);

	if(tjd <= cjd) {
		if(enable == 0) {
			drv_l1_rtc_int_set(RTC_ALM_INT_INDEX, DISABLE);
			drv_l1_rtc_callback_clear(RTC_ALM_INT_INDEX);
			return STATUS_OK;
		}	
	
		rtc_time.rtc_sec = tm->tm_sec;
		rtc_time.rtc_min = tm->tm_min;
		rtc_time.rtc_hour = tm->tm_hour;
		drv_l1_rtc_alarm_set(&rtc_time);
		drv_l1_rtc_callback_set(RTC_ALM_INT_INDEX, alarm_isr);
		drv_l1_rtc_int_set(RTC_ALM_INT_INDEX, ENABLE);
	} else {
		if(enable == 0) {
			drv_l1_rtc_int_set(EXT_RTC_ALARM_INT_INDEX, DISABLE);
			drv_l1_rtc_callback_clear(EXT_RTC_ALARM_INT_INDEX);
			return STATUS_OK;
		}
	
		rtc_time.rtc_sec = tm->tm_sec;
		rtc_time.rtc_min = tm->tm_min;
		rtc_time.rtc_hour = tm->tm_hour;
		rtc_time.rtc_day = tjd - LAST_JULIAN_DATE;
		drv_l1_idp_rtc_alarm_set(&rtc_time);
		drv_l1_rtc_callback_set(EXT_RTC_ALARM_INT_INDEX, alarm_isr);
		drv_l1_rtc_int_set(EXT_RTC_ALARM_INT_INDEX, ENABLE);
	}
	return STATUS_OK;
}

void cal_set_time_zone(INT8U t_zone)
{
#if SUPPORT_TIME_ZONE_ADJUST == 1
	time_zone = t_zone;
#endif
}
#endif //(defined GPLIB_CALENDAR_EN) && (GPLIB_CALENDAR_EN == 1)  
