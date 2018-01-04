#ifndef __drv_l1_ADC_H__
#define __drv_l1_ADC_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

#define ADC_LINE_0		0 
#define ADC_LINE_1		1

#define ADC_AS_TIMER_A	0
#define ADC_AS_TIMER_B	1
#define ADC_AS_TIMER_C	2
#define ADC_AS_TIMER_D	3
#define ADC_AS_TIMER_E	4
#define ADC_AS_TIMER_F	5

typedef struct
{
	INT8S *notify;
	INT16U *adc_data;
	INT32U data_len;
	INT32U count;
} ADC_CTRL;

extern void drv_l1_adc_init(void);
extern void drv_l1_adc_vref_enable_set(BOOLEAN status);
extern INT32S drv_l1_adc_conv_time_sel(INT32U value);
extern void drv_l1_adc_fifo_level_set(INT8U level);
extern void drv_l1_adc_fifo_clear(void);
extern void drv_l1_adc_auto_ch_set(INT8U ch);
extern void drv_l1_adc_manual_ch_set(INT8U ch);
extern void drv_l1_adc_auto_int_set(BOOLEAN status);
extern void drv_l1_adc_manual_callback_set(void (*user_isr)(INT16U data));
extern INT32S drv_l1_adc_user_line_in_en(INT8U ch, INT32U enable);
extern INT32S drv_l1_adc_manual_sample_start(void);
extern INT32S drv_l1_adc_auto_sample_start(void);
extern void drv_l1_adc_auto_sample_stop(void);
extern INT32S drv_l1_adc_sample_rate_set(INT8U timer_id, INT32U hz);
extern INT32S drv_l1_adc_timer_stop(INT8U timer_id);

extern void drv_l1_tp_en(BOOLEAN status);
extern void drv_l1_tp_int_en(BOOLEAN status);
extern void drv_l1_tp_callback_set(void (*user_isr)(void));

extern INT32S drv_l1_adc_auto_data_get(INT16U *data, INT32U len, INT8S *notify);
extern INT32S drv_l1_adc_auto_data_dma_get(INT16U *data, INT32U len, INT8S *notify);
#if _OPERATING_SYSTEM != _OS_NONE
extern INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, OS_EVENT *os_q);
#else 
extern INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify);
#endif
extern INT32S drv_l1_adc_dbf_set(INT16S *data,INT32U len);
extern INT32S drv_l1_adc_dbf_status_get(void);
extern INT32S drv_l1_adc_dma_status_get(void);
extern void drv_l1_adc_dbf_free(void);
#endif		// __drv_l1_ADC_H__
