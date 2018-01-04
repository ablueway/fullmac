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
#include "drv_l1_adc.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_dma.h"
#include "gp_stdlib.h"

#if (defined _DRV_L1_ADC) && (_DRV_L1_ADC == 1)                   
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// P_ADC_SETUP 
#define ADC_ADBEN			(1 << 15)	//AD Bias Reference Voltage Enable
#define ADC_ADCEN			(1 << 14)	//ADC Enable
#define ADC_CLKSEL			(0x07 << 8)	//ADC Conversion Time Select
#define ADC_ASEN			(1 << 7) 	//Auto Sampling Mode Enable
#define ADC_AUTO_CH_SEL		(0x07 << 4)	//Auto Sample Mode ADC channel selection
#define ADC_ASMEN			(1 << 3)	//ADC Auto Sampling Mode Enable
#define ADC_ASTMS			(0x07)		//ADC Auto Sampling Timer Selection

// P_ADC_MADC_CTRL
#define ADC_ADCRIF			(1 << 15)	//AD Conversion Ready Interrupt Flag & Clear
#define ADC_ADCRIEN			(1 << 14)	//AD Conversion Ready Interrupt Enable
#define ADC_CNVRDY			(1 << 7) 	//AD Conversion Ready 
#define ADC_STRCNV			(1 << 6)	//Manual Start AD Conversion
#define ADC_MIASE			(1 << 5)	//AD Conversion Error Flag1
#define ADC_ASIME			(1 << 4)	//AD Conversion Error Flag2
#define ADC_MANUAL_CH_SEL	(0x07)

// P_ADC_ASADC_CTRL
#define ADC_AUTO_ASIF		(1 << 15)	//Auto Sample Mode FIFO Full interrupt flag 
#define ADC_AUTO_ASIEN		(1 << 14)	//Auto Sample Mode FIFO Full Interrupt Enable
#define ADC_ASFF			(1 << 13)	//Auto Sample Mode FIFO Full Flag
#define ADC_ASFOV			(1 << 12)	//Auto Sample Mode FIFO Overflow Flag
#define ADC_ASADC_DMA		(1 << 11)	//DMA mode
#define ADC_OVER			(1 << 10)	//Auto Sample FIFO Over Write Mode, 0:skip
#define ADC_FIFO_LEVEL		(0xF << 4) 	//FIFO level

// P_ADC_TP_CTRL
#define ADC_TP_IF			(1 << 15)	//Touch Panel Interrupt Flag
#define ADC_TP_INT_EN      	(1 << 14)	//Touch Panel Interrupt Enable
#define ADC_TP_EN			(1 << 13)	//Touch Panel Interface Enable
#define ADC_TP_MODE			(1 << 11)	//Touch Panel Mode
#define ADC_TP_DBEN			(1 << 4)	//De-bounce Enable
#define ADC_TP_DBTSEL		(0x03 << 4)	//De-bounce Timing Selection

#define ADC_ERR_WRONG_LEN  (-1)
#define ADC_BLOCK_LEN      2048

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		gp_memset((INT8S *)ptr, 0x00, cblen) 

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void adc_auto_isr(void);
static void adc_manual_isr(void);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static ADC_CTRL adc_ctrl;
static DMA_STRUCT dma_adc_dbf;
static void (*adc_user_isr)(INT16U data);
static void (*tp_user_isr)(void);


/**
 * @brief   adc initialize.  
 * @param   none
 * @return 	none
 */
void drv_l1_adc_init(void)
{
	R_ADC_SETUP = 0;
	R_ADC_ASADC_CTRL = 0;
	R_ADC_MADC_CTRL = 0;
	R_ADC_TP_CTRL = 0;

	R_ADC_SETUP |= ADC_ADBEN; // AD Bias Reference Voltage Enable 		
	R_ADC_USELINEIN = 0;
	R_ADC_SH_WAIT = 0x0807;

	adc_user_isr = 0;
	CLEAR(&adc_ctrl, sizeof(ADC_CTRL)); 
	CLEAR(&dma_adc_dbf, sizeof(DMA_STRUCT)); 
	dma_adc_dbf.channel = 0xff;
	
	vic_irq_register(VIC_ADCF, adc_auto_isr);	/* register auto smaple isr */
	vic_irq_register(VIC_ADC, adc_manual_isr);	/* register manual smaple isr */
	vic_irq_enable(VIC_ADCF);
	vic_irq_enable(VIC_ADC);
}	

static void adc_auto_isr(void)
{
	while(((R_ADC_ASADC_CTRL & 0xF) != 0) && (adc_ctrl.count < adc_ctrl.data_len)) {
			adc_ctrl.adc_data[adc_ctrl.count++] = R_ADC_ASADC_DATA;
	}
	R_ADC_ASADC_CTRL |= ADC_AUTO_ASIF;  /* clear flag must after getting data */
	
	if (adc_ctrl.count >= adc_ctrl.data_len) {
		drv_l1_adc_auto_int_set(FALSE); /* disable fifo interrupt */
		*adc_ctrl.notify = 1;
	}	
}

static void adc_manual_isr(void)
{
	if (R_ADC_TP_CTRL & ADC_TP_IF) {
		R_ADC_TP_CTRL |= ADC_TP_IF;
        (*tp_user_isr)();
	}	
	
	if (R_ADC_MADC_CTRL & ADC_ADCRIF) {
		if(R_ADC_MADC_CTRL & (ADC_MIASE | ADC_ASIME)) {
			drv_l1_adc_manual_sample_start();
			return;
		}

		if((R_ADC_MADC_CTRL & ADC_CNVRDY) == 0) {
			drv_l1_adc_manual_sample_start();
			return;
		}
		
	    R_ADC_MADC_CTRL |= ADC_ADCRIF;
	    (*adc_user_isr)((INT16U)R_ADC_MADC_DATA);
	    R_ADC_MADC_CTRL &= ~ADC_ADCRIEN; /* disable manual interrupt */		
	}
}

/**
 * @brief   set adc vref enable.  
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_adc_vref_enable_set(BOOLEAN status)
{
	if (status == TRUE) {
		R_ADC_SETUP |= ADC_ADBEN; 	// AD Bias Reference Voltage Enable
	} else {
		R_ADC_SETUP &= ~ADC_ADBEN;	// AD Bias Reference Voltage Disable
	}
}

/**
 * @brief   set adc convert time.  
 * @param   value[in]: 512xClk ~ 2048xClk
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_adc_conv_time_sel(INT32U value)
{
	INT32S ret = STATUS_OK;

	if (value > 5) {
		ret = STATUS_FAIL;
	} else {
		R_ADC_SETUP &= ~ADC_CLKSEL;
		R_ADC_SETUP |= (value<<8);
	}
	
	return ret;
}

/**
 * @brief   set adc fifo threshold.  
 * @param   level[in]: 0 ~ 0xF
 * @return 	none
 */
void drv_l1_adc_fifo_level_set(INT8U level)
{
	R_ADC_ASADC_CTRL &= ~ADC_FIFO_LEVEL;
	R_ADC_ASADC_CTRL |= (level << 4); 
}

/**
 * @brief   clear adc fifo   
 * @param   none
 * @return 	none
 */
void drv_l1_adc_fifo_clear(void)
{
	INT16U dummy;
	while((R_ADC_ASADC_CTRL & 0xF) != 0) {
		dummy = R_ADC_ASADC_DATA;
		if(dummy){};	//for remove compiler warning
	}	
}

/**
 * @brief   set adc auto channel number.  
 * @param   ch[in]: 0 ~ 3
 * @return 	none
 */
void drv_l1_adc_auto_ch_set(INT8U ch)
{
	R_ADC_SETUP &= ~ADC_AUTO_CH_SEL;
	R_ADC_SETUP |= (ch << 4);
}

/**
 * @brief   set adc manual channel number.  
 * @param   ch[in]: 0 ~ 3
 * @return 	none
 */
void drv_l1_adc_manual_ch_set(INT8U ch)
{
	R_ADC_MADC_CTRL &= ~ADC_MANUAL_CH_SEL;
	R_ADC_MADC_CTRL |= ch;
}

/**
 * @brief   set adc auto interrupt enable.  
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_adc_auto_int_set(BOOLEAN status)
{
	if (status == TRUE) {
		R_ADC_ASADC_CTRL |= ADC_AUTO_ASIEN;
	} else {
		R_ADC_ASADC_CTRL &= ~ADC_AUTO_ASIEN;
	}
}

/**
 * @brief   set adc manual mode callback function.  
 * @param   user_isr[in]: function
 * @return 	none
 */
void drv_l1_adc_manual_callback_set(void (*user_isr)(INT16U data))
{
	if (user_isr != 0) {
		adc_user_isr = user_isr;
	}
}

/**
 * @brief   set adc used channel.  
 * @param   line_id[in]: ADC_LINE_0 ~ ADC_LINE_1
 * @param   status[in]: enable or disable
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_user_line_in_en(INT8U ch, INT32U enable)
{	
	switch(ch) 
	{
	case ADC_LINE_0:
	case ADC_LINE_1:
		if (enable) {
			R_ADC_USELINEIN |= (1 << ch); 
		} else {
			R_ADC_USELINEIN &= ~(1 << ch); 
		}
		break;	
	default:
		return STATUS_FAIL;
	}
	
	return STATUS_OK;
}

/**
 * @brief   set adc manual sample start.  
 * @param   none
 * @return 	STATUS_OK
 */
INT32S drv_l1_adc_manual_sample_start(void)
{	
	R_ADC_MADC_CTRL |= ADC_ADCRIF|ADC_MIASE|ADC_ASIME;	// clear flag
	R_ADC_MADC_CTRL |= ADC_ADCRIEN;	// enable interrupt
	R_ADC_MADC_CTRL |= ADC_STRCNV;	// start manual sample
	return STATUS_OK;
}

/**
 * @brief   set adc auto sample start.  
 * @param   none
 * @return 	STATUS_OK
 */
INT32S drv_l1_adc_auto_sample_start(void)
{
	R_ADC_SETUP |= ADC_ADCEN|ADC_ADBEN|ADC_ASEN; 
	R_ADC_ASADC_CTRL |= ADC_AUTO_ASIF;	// clear fifo interrupt flag
	R_ADC_ASADC_CTRL |= ADC_ASADC_DMA;	// DMA mode enable
	R_ADC_SETUP |= ADC_ASMEN;			// start auto sample
	return STATUS_OK;
}

/**
 * @brief   adc auto sample stop.  
 * @param   none
 * @return 	none
 */
void drv_l1_adc_auto_sample_stop(void)
{
	R_ADC_SETUP &= ~ADC_ASMEN;	
}

/**
 * @brief   adc sample rate and use timer set start.  
 * @param   timer_id[in]: timerA ~ timerF
 * @param   hz[in]: sample rate
 * @return 	none
 */
INT32S drv_l1_adc_sample_rate_set(INT8U timer_id, INT32U hz)
{
	R_ADC_SETUP &= ~ADC_ASTMS;
	R_ADC_SETUP |= timer_id;
	return adc_timer_freq_setup(timer_id, hz);
}

/**
 * @brief   adc used timer set stop.  
 * @param   timer_id[in]: timerA ~ timerF
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_timer_stop(INT8U timer_id)
{
	return timer_stop(timer_id);	
}

/**
 * @brief   set adc touch panel enable.  
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_tp_en(BOOLEAN status)
{
	if (status == TRUE) {
		R_ADC_TP_CTRL |= ADC_TP_EN;
		//R_ADC_TP_CTRL |= ADC_TP_MODE;
	} else {
		R_ADC_TP_CTRL &= ~ADC_TP_EN;
		//R_ADC_TP_CTRL &= ~ADC_TP_MODE;
	}
}

/**
 * @brief   set adc touch panel interrupt enable.  
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_tp_int_en(BOOLEAN status)
{
	if (status == TRUE) {
		R_ADC_TP_CTRL |= ADC_TP_INT_EN;
	} else {
		R_ADC_TP_CTRL &= ~ADC_TP_INT_EN;
	}
}

/**
 * @brief   set adc touch panel callback function.  
 * @param   user_isr[in]: function pointer
 * @return 	none
 */
void drv_l1_tp_callback_set(void (*user_isr)(void))
{
	if (user_isr != 0) {
		tp_user_isr = user_isr;
	}
}

/**
 * @brief   adc get data.  
 * @param   data[in]: data pointer
 * @param   len[in]: length
 * @param   notify[in]: notify pointer
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_auto_data_get(INT16U *data, INT32U len, INT8S *notify)
{
	if (len > ADC_BLOCK_LEN) {
		return ADC_ERR_WRONG_LEN;
	}
	R_ADC_ASADC_CTRL &= ~ADC_ASADC_DMA; /* DMA mode disable */
	
	adc_ctrl.adc_data = data;
	adc_ctrl.data_len = len;
	adc_ctrl.count = 0;
	adc_ctrl.notify = notify;
	*adc_ctrl.notify = 0;
	
	if (len != 0) {
		drv_l1_adc_auto_int_set(TRUE); /* enable fifo interrupt */
	}
	return STATUS_OK;	
}

/**
 * @brief   adc get data by dma.  
 * @param   data[in]: data pointer
 * @param   len[in]: length
 * @param   notify[in]: notify pointer
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_adc_auto_data_dma_get(INT16U *data, INT32U len, INT8S *notify)
{
	INT32S status;
	DMA_STRUCT dma_struct;
	
	if (len > ADC_BLOCK_LEN) {
		return ADC_ERR_WRONG_LEN;
	}
	
	*notify = C_DMA_STATUS_WAITING;
	dma_struct.s_addr = (INT32U) P_ADC_ASADC_DATA;
	dma_struct.t_addr = (INT32U) data;
	dma_struct.width = DMA_DATA_WIDTH_2BYTE;		// DMA_DATA_WIDTH_1BYTE or DMA_DATA_WIDTH_2BYTE or DMA_DATA_WIDTH_4BYTE
	dma_struct.count = (INT32U) len;
	dma_struct.notify = notify;
	dma_struct.timeout = 0;
	dma_struct.aes = 0;
	dma_struct.trigger = 0;
	
	status = drv_l1_dma_transfer(&dma_struct);
	if (status != 0) {
		return status;
	}
		
	return STATUS_OK;	
}

/**
 * @brief   get data from mic by dma with double buffer mode 
 * @param   data: data pointer
 * @param   len: length
 * @param   os_q: os queue
 * @param   notify: notify pointer
 * @return 	STATUS_OK / STATUS_FAIL
 */
#if _OPERATING_SYSTEM != _OS_NONE
INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, OS_EVENT *os_q)
#else 
INT32S drv_l1_adc_dbf_put(INT16S *data,INT32U len, volatile INT8S *notify)
#endif
{
	INT32S status;
	
	dma_adc_dbf.s_addr = (INT32U) P_ADC_ASADC_DATA;
	dma_adc_dbf.t_addr = (INT32U) data;
	dma_adc_dbf.width = DMA_DATA_WIDTH_2BYTE;
	dma_adc_dbf.count = (INT32U) len;
	dma_adc_dbf.notify = NULL;
	dma_adc_dbf.timeout = 0;
	
#if _OPERATING_SYSTEM != _OS_NONE		
	status = drv_l1_dma_transfer_with_double_buf(&dma_adc_dbf, os_q);
#else
	dma_adc_dbf.notify = notify;
	status = drv_l1_dma_transfer_with_double_buf(&dma_adc_dbf);
#endif	
	if(status != 0) {
		return status;
	}
		
	return STATUS_OK;
}

/**
 * @brief   set data from adc by dma with double buffer mode 
 * @param   data: data pointer
 * @param   len: length
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_adc_dbf_set(INT16S *data,INT32U len)
{
	INT32S status;
	
	dma_adc_dbf.t_addr = (INT32U) data;
	dma_adc_dbf.count = (INT32U) len;
	dma_adc_dbf.notify = NULL;
	dma_adc_dbf.timeout = 0;
	
	status = drv_l1_dma_transfer_double_buf_set(&dma_adc_dbf);
	if(status != 0) {
		return status;
	}
		
	return STATUS_OK;
}

/**
 * @brief   get mic dma double buffer status  
 * @param   none
 * @return 	1 is full, 0 is empty
 */
INT32S drv_l1_adc_dbf_status_get(void)
{
	if (dma_adc_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_dbf_status_get(dma_adc_dbf.channel);	
}

/**
 * @brief   get adc dma status  
 * @param   none
 * @return 	1 is busy, 0 is idle
 */
INT32S drv_l1_adc_dma_status_get(void)
{
	if (dma_adc_dbf.channel == 0xff) {
		return -1;
	}
	return drv_l1_dma_status_get(dma_adc_dbf.channel);	
}

/**
 * @brief   release double buffer dma channel  
 * @param   none
 * @return 	none
 */
void drv_l1_adc_dbf_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&dma_adc_dbf);
	dma_adc_dbf.channel = 0xff;
}
#endif //(defined _DRV_L1_ADC) && (_DRV_L1_ADC == 1)             

