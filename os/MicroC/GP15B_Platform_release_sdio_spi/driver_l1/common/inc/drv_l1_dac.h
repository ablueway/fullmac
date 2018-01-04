#ifndef __drv_l1_DAC_H__
#define __drv_l1_DAC_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

typedef enum
{
	DAC_ENABLED,
	DAC_DISABLED
} DAC_STATUS;

typedef enum
{
	DAC_CHA,
	DAC_CHB
} DAC_CHANNELS;

typedef struct
{
	INT16S *cha_data;
	INT16S *chb_data;
	INT32U cha_len;
	INT32U chb_len;
	INT32U cha_count;
	INT32U chb_count;
	INT8S *cha_notify;
	INT8S *chb_notify;
} DAC_CTRL;

extern void drv_l1_dac_init(void);
extern void drv_l1_dac_enable_set(BOOLEAN status);
extern void drv_l1_dac_disable(void);
extern void drv_l1_dac_fifo_level_set(INT8U cha_level,INT8U chb_level);
extern void drv_l1_dac_data_signed_set(BOOLEAN flag);
extern void drv_l1_dac_mono_set(void);
extern void drv_l1_dac_stereo_set(void);
extern void drv_l1_dac_stereo_indi_buffer_set(void);
extern void drv_l1_dac_sample_rate_set(INT32U hz);
extern void drv_l1_dac_timer_stop(void);
extern void drv_l1_dac_pga_set(INT8U gain);
extern INT8U drv_l1_dac_pga_get(void);
extern void drv_l1_dac_vref_set(BOOLEAN status);
extern void drv_l1_dac_int_set(INT8U ch, INT8U status);

extern void drv_l1_dac_cha_data_put(INT16S *data,INT32U len, INT8S *notify);
extern void drv_l1_dac_chb_data_put(INT16S *data,INT32U len, INT8S *notify);
extern INT32S drv_l1_dac_cha_data_dma_put(INT16S *data,INT32U len, INT8S *notify);
extern INT32S drv_l1_dac_chb_data_dma_put(INT16S *data,INT32U len, INT8S *notify);

#if _OPERATING_SYSTEM != _OS_NONE
extern INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, OS_EVENT *os_q);
#else 
extern INT32S drv_l1_dac_cha_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify);
#endif
extern INT32S drv_l1_dac_cha_dbf_set(INT16S *data,INT32U len);
extern INT32S drv_l1_dac_dbf_status_get(void);
extern INT32S drv_l1_dac_dma_status_get(void);
extern void drv_l1_dac_dbf_free(void);
#endif		// __drv_l1_DAC_H__