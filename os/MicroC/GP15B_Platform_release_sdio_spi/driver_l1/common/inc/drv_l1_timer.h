#ifndef __drv_l1_TIMER_H__
#define __drv_l1_TIMER_H__

#include "driver_l1.h"

#define TIMER_FAIL  	0
#define TIMER_OK    	1

#define TIME_BASE_FAIL  0
#define TIME_BASE_OK	1

typedef enum 
{
	SRCBSEL_DEFAULT=0,
	SRCB_2048Hz_SELECT=SRCBSEL_DEFAULT,
	SRCB_1024Hz_SELECT,
	SRCB_256Hz_SELECT,
	SRCB_TIMEBASE_B_SELECT,
	SRCB_TIMEBASE_A_SELECT,
	SRCB_0_,
	SRCB_1_,
	SRCB_SELECT_MAX=SRCB_1_
} SRCBSEL_ENUM;

typedef enum 
{
	SRCASEL_DEFAULT=0,
	SRCA_SYSCLK_DIV2_SELECT=SRCASEL_DEFAULT,
	SRCA_SYSCLK_DIV256_SELECT,
	SRCA_32768Hz_SELECT,
	SRCA_8192Hz_SELECT,
	SRCA_4096Hz_SELECT,
	SRCA_1_,
	SRCA_TIMER_OVERFLOW,
	SRCA_EXTA_WITH_PRESCALE,
	SRCA_0_,
	SRCA_SELECT_MAX=SRCA_0_
} SRCASEL_ENUM;

typedef enum 
{
	EXTBSEL_DEFAULT=0,  
	EXTB_EVERY_FALLING=EXTBSEL_DEFAULT,
	EXTB_EVERY_RISING,
	EXTB_EVERY_4_RISING,
	EXTB_EVERY_16_RISING,
	EXTB_EVERY_MAX
} EXTBSEL_ENUM;

typedef enum 
{
	EXTASEL_DEFAULT=0,  
	EXTA_EVERY_FALLING=EXTASEL_DEFAULT,
	EXTA_EVERY_RISING,
	EXTA_EVERY_4_RISING,
	EXTA_EVERY_16_RISING,
	EXTA_EVERY_MAX
} EXTASEL_ENUM;

typedef enum 
{
	TIMER_DISABLE=0,
	TIMER_ENABLE
} TMXEN_ENUM;   /*if enable, timer will start counting*/

typedef enum 
{
	TIMER_INT_DISABLE=0,
	TIMER_INT_ENABLE
} TMXIE_ENUM;   /*if enable, timer interrupt will start counting*/

typedef enum 
{
	TIMER_INT_FLAG_DEFAULT=0,
	TIMER_INT_FLAG_FALSE=TIMER_INT_FLAG_DEFAULT,
	TIMER_INT_FLAG_TRUE
} TMXIF_ENUM;   

typedef enum
{
	CMP_HIGH_PULSE_ON_CCP=0,
	CMP_LOW_PULSE_ON_CCP,
	CMP_UNAFFECTED_ON_CCP  
} CMPXSEL_ENUM;

typedef enum
{
	CAP_EVERY_FALLING=0,
	CAP_EVERY_RISING  
} CAPSEL_ENUM;

typedef enum
{
	CCP_MODE_DISABLED=0,
	CAPTURE_ENABLED=0x1,
	COMPARISON_ENABLED=0x2,
	PWM_ENABLED=0x3,
	CCPXEN_MAX
} CCPXEN_ENUM;

/* Time base enum begin */
typedef enum 
{
	TMBXEN_DISABLE=0,
	TMBXEN_ENABLE
} TMBXEN_ENUM; 

typedef enum 
{
	TMBXIE_DISABLE=0,
	TMBXIE_ENABLE
} TMBXIE_ENUM; 

typedef enum 
{
	TMBXIF_DISABLE=0,
	TMBXIF_ENABLE
} TMBXIF_ENUM; 

typedef enum
{
	TIMER_SOURCE_256HZ=0,
	TIMER_SOURCE_1024HZ,
	TIMER_SOURCE_2048HZ,
	TIMER_SOURCE_4096HZ,
	TIMER_SOURCE_8192HZ,
	TIMER_SOURCE_32768HZ,
	TIMER_SOURCE_MCLK_DIV_256,
	TIMER_SOURCE_MCLK_DIV_2,
	TIMER_SOURCE_MAX
} TIMER_SOURCE_ENUM;

typedef enum
{
	TMBAS_1HZ=1,
	TMBAS_2HZ,
	TMBAS_4HZ,
	TMBBS_8HZ=0,
	TMBBS_16HZ,
	TMBBS_32HZ,
	TMBBS_64HZ,
	TMBCS_128HZ=0,
	TMBCS_256HZ,
	TMBCS_512HZ,
	TMBCS_1024HZ,
	TMBXS_MAX
} TMBXS_ENUM;

typedef enum
{
	TIMER_A=0,
	TIMER_B,
	TIMER_C,
	TIMER_D,
	TIMER_E,
	TIMER_F,
	TIMER_RTC,
	TIMER_ID_MAX
} TIMER_ID_ENUM;

typedef enum
{
	TIME_BASE_A=0,
	TIME_BASE_B,
	TIME_BASE_C,
	TIME_BASE_ID_MAX
} TIME_BASE_ID_ENUM;

typedef enum
{
	PWM_NRO_OUTPUT=0,
	PWM_NRZ_OUTPUT  
} PWMXSEL_ENUM;

extern void timer_init(void);
extern INT32S timer_freq_setup(INT32U timer_id, INT32U freq_hz, INT32U times_counter, void(* TimerIsrFunction)(void));
extern INT32S timer_msec_setup(INT32U timer_id, INT32U msec, INT32U times_counter,void (*TimerIsrFunction)(void));
extern INT32S dac_timer_freq_setup(INT32U freq_hz);  /* for dac timer (TimerE)*/
extern INT32S adc_timer_freq_setup(INT32U timer_id, INT32U freq_hz);
extern INT32S timer_stop(INT32U timer_id);
extern INT32S time_base_setup(INT32U time_base_id, TMBXS_ENUM tmbxs_enum_hz, INT32U times_counter, void(* TimeBaseIsrFunction)(void));
extern INT32S time_base_stop(INT32U time_base_id);
extern INT32U TimerCountReturn(INT32U Timer_Id);
extern void TimerCountSet(INT32U Timer_Id, INT32U set_count);
extern void time_base_reset(void);
extern INT32S timer_start_without_isr(INT32U timer_id, TIMER_SOURCE_ENUM timer_source);
extern void timer_counter_reset(INT32U timer_id);
extern INT32S timer_pwm_setup(INT32U timer_id, INT32U freq_hz, INT8U pwm_duty_cycle_percent, PWMXSEL_ENUM NRO_NRZ);
extern INT32S timerD_counter_init(void);
extern INT32S tiny_counter_get(void);
extern INT32S timer_freq_setup_lock(TIMER_ID_ENUM timer_id);
extern INT32S timer_freq_setup_unlock(TIMER_ID_ENUM timer_id);
extern INT32S timer_freq_setup_status_get(TIMER_ID_ENUM timer_id);
extern void drv_msec_wait(INT32U m_sec) ;
#endif  /*__drv_l1_TIMER_H__*/
