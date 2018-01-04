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
#include "drv_l1_sfr.h"
#include "drv_l1_mipi.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_system.h"

#if (defined _DRV_L1_MIPI) && (_DRV_L1_MIPI == 1)
/****************************************************
*		constant 									*
****************************************************/
/* mipi lane */
#define MIPI_1_LANE				0x0
#define MIPI_2_LANE				0x1

/****************************************************
*		varaible 									*
*****************************************************/
extern INT32U MCLK;
static void (*mipi_isr_callback)(INT32U event);


static void mipi_isr(void)
{
	INT32U IRQ_Status = R_MIPI_INT_STATUS;
	INT32U IRQ_Enable = R_MIPI_INT_ENABLE;
	
	R_MIPI_INT_SOURCE = IRQ_Status; 
	IRQ_Status &= IRQ_Enable;
	if(mipi_isr_callback) {
		mipi_isr_callback(IRQ_Status);
	}	
}

/**
 * @brief   mipi init
 * @param   enable[in]: mipi enable or disable
 * @return 	none
 */
void drv_l1_mipi_init(INT8U enable)
{
	if(enable == 0) {
		R_MIPI_GLB_CSR = 0;
		R_MIPI_INT_SOURCE = 0x3F; 
		R_MIPI_INT_ENABLE = 0; 
		vic_irq_unregister(VIC_MIPI);
		vic_irq_disable(VIC_MIPI);
		return; 
	}
	
	R_MIPI_GLB_CSR = 0x01; 
	R_MIPI_ECC_ORDER = 0;
	R_MIPI_CCIR601_TIMING = 0;
	R_MIPI_IMG_SIZE = 0;
	R_MIPI_DATA_FMT = 0;
	R_MIPI_INT_SOURCE = 0x3F; 
	R_MIPI_INT_ENABLE = 0; 
	vic_irq_register(VIC_MIPI, mipi_isr);
	vic_irq_enable(VIC_MIPI);
}

/**
 * @brief   mipi isr function register
 * @param   user_isr[in]: isr function
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_mipi_isr_register(void (*user_isr_callback)(INT32U event))
{
	if(user_isr_callback == 0) {
		return STATUS_FAIL;
	}
	
	mipi_isr_callback = user_isr_callback;
	return STATUS_OK;
}

/**
 * @brief   mipi isr function unregister
 * @param   none
 * @return 	none
 */
void drv_l1_mipi_isr_unregister(void)
{
	mipi_isr_callback = 0;
}

/**
 * @brief   mipi isr function unregister
 * @param   enable[in]: enable or disable
 * @param   index[in]: interrupt bit
 * @return 	STATUS_OK/STATUS_FAIL
 */
INT32S drv_l1_mipi_set_irq_enable(INT32U enable, INT32U bit)
{
	if(enable) {
		R_MIPI_INT_SOURCE |= bit & 0x3F;
		R_MIPI_INT_ENABLE |= bit & 0x3F; 
	} else {
		R_MIPI_INT_ENABLE &= ~bit & 0x3F; 	
		R_MIPI_INT_SOURCE = bit & 0x3F;
	}
	
	return STATUS_OK;
}

/**
 * @brief   set mipi global configure 
 * @param   low_power_en[in]: low power enable or disable
 * @param   byte_clk_edge[in]: byt clock edge set, 0: posedge, 1: negedge
 * @return 	none
 */
void drv_l1_mipi_set_global_cfg(INT8U low_power_en, INT8U byte_clk_edge)
{
	INT32U reg = R_MIPI_GLB_CSR;
	
	if(low_power_en) {
		reg |= (1<<4);
	} else {
		reg &= ~(1<<4);
	}
	
	if(byte_clk_edge) {
		reg |= (1<<5);
	} else {
		reg &= ~(1<<5);
	}
	
	reg &= ~(1<<8);	//only support 1 lane
	R_MIPI_GLB_CSR = reg;	
} 

/**
 * @brief   set mipi ecc configure
 * @param   ecc_order[in]: ecc order, MIPI_ECC_ORDER0 ~ MIPI_ECC_ORDER3
 * @param   ecc_check_en[in]: ecc check enable/disable
 * @return 	none
 */
void drv_l1_mipi_set_ecc(INT8U ecc_order, INT8U ecc_check_en)
{
	INT32U reg = R_MIPI_ECC_ORDER;
	
	reg &= ~0x7;
	reg |= ((ecc_check_en & 0x01) << 2) | (ecc_order & 0x03);						
	R_MIPI_ECC_ORDER = reg;
}

/**
 * @brief   set mipi ecc configure
 * @param   da_mask_cnt[in]: LP to HS mask count
 * @param   check_hs_seq[in]: 1: Check HS sequence. 0: just check LP. when enter HS mode
 * @return 	none
 */
void drv_l1_mipi_set_mask_cnt(INT8U da_mask_cnt, INT8U check_hs_seq)
{
	INT32U reg = R_MIPI_ECC_ORDER;
	
	reg &= ~(0x1FF << 8);
	reg |= (((UINT32)check_hs_seq & 0x01) << 16) | (((UINT32)da_mask_cnt & 0xFF) << 8);				
	R_MIPI_ECC_ORDER = reg;
}

/**
 * @brief   mipi set ccir601 interface
 * @param   h_back_proch[in]: horizontal back proch
 * @param   h_front_proch[in]: horizontal front proch
 * @param   blanking_line_en[in]: blanking line enable, 0 mask hsync when vsync
 * @return 	none 
 */
void drv_l1_mipi_set_ccir601_if(INT8U h_back_porch, INT8U h_front_porch, INT8U blanking_line_en)
{
	INT32U reg = R_MIPI_CCIR601_TIMING;
	
	reg &= ~(0x0F << 0);
	reg |= (h_back_porch & 0x0F);
	
	reg &= ~(0x0F << 8);
	reg |= ((INT32U)(h_front_porch & 0x0F) << 8);
	
	if(blanking_line_en) {
		reg |= (1<<16);
	} else {
		reg &= ~(1<<16);
	}

	R_MIPI_CCIR601_TIMING = reg;
}

/**
 * @brief   mipi set image size
 * @param   h_size[in]: horizontal size set
 * @param   v_size[in]: vertical size set
 * @return 	none  
 */
void drv_l1_mipi_set_image_size(INT16U h_size, INT16U v_size)
{
	if(h_size == 0) {
		h_size = 1;
	}
	
	if(v_size == 0) {
		v_size = 1;
	}
		
	h_size &= 0xFFFF;
	v_size &= 0xFFFF;
	R_MIPI_IMG_SIZE = h_size | ((INT32U)v_size << 16);
}

/**
 * @brief   mipi set data format
 * @param   data_from_mmr[in]: 0: auto detect data type, 1: user define 
 * @param   data_type_mmr[in]: user define data format. MIPI_YUV422 ~ MIPI_RAW12
 * @param   data_type_cdsp[in]: for cdsp 10bit data bus, 1:data[7:0]+2':00, 0: 2'00+data[7:0] 
 * @return 	none 
 */
void drv_l1_mipi_set_data_fmt(INT8U data_from_mmr, INT8U data_type_mmr, INT8U data_type_cdsp)
{
	INT32U reg = R_MIPI_DATA_FMT;

	if(data_from_mmr) {
		reg |= (1<<0);
	} else {
		reg &= ~(1<<0);
	}
	
	reg &= ~(0x07 << 4);
	reg |= (data_type_mmr & 0x07) << 4;
	
	if(data_type_cdsp) {
		reg |= (1 << 7);
	} else {
		reg &= ~(1 << 7);
	}

	R_MIPI_DATA_FMT = reg;
}

/**
 * @brief   mipi set parameter
 * @param   argp[in]: parameter 
 * @return 	STATUS_OK / STATUS_FAIL 
 */
INT32S drv_l1_mipi_set_parameter(mipi_config_t *argp)
{
	INT32U delta_t, data_mask_cnt, remain; 

	if(argp == 0) {
		return STATUS_FAIL;
	}

	drv_l1_mipi_init(ENABLE);
	drv_l1_mipi_set_global_cfg(argp->low_power_en, argp->byte_clk_edge);

	/* change to ns */
	delta_t = 1000*1000000 / MCLK;
	
	/* must > 40ns */
	if(argp->data_mask_time < 40) {
		argp->data_mask_time = 40;
	}
	
	/* get data mask count */
	data_mask_cnt = argp->data_mask_time / delta_t;
	remain = argp->data_mask_time % delta_t;
	if(remain) {
		data_mask_cnt++;
	}
	
	drv_l1_mipi_set_ecc(argp->ecc_order, argp->ecc_check_en);
	drv_l1_mipi_set_mask_cnt(data_mask_cnt, argp->check_hs_seq);
	drv_l1_mipi_set_ccir601_if(argp->h_back_porch, argp->h_front_porch, argp->blanking_line_en);
	if(argp->data_from_mmr) {
		// user define format
		drv_l1_mipi_set_data_fmt(argp->data_from_mmr, argp->data_type, argp->data_type_to_cdsp);
		if(argp->data_type == MIPI_YUV422) {
			drv_l1_mipi_set_image_size(argp->h_size << 1, argp->v_size);
		} else {
			drv_l1_mipi_set_image_size(argp->h_size, argp->v_size);
		}
	} else {
		// auto detect
		drv_l1_mipi_set_data_fmt(0, 0, argp->data_type_to_cdsp);
		drv_l1_mipi_set_image_size(argp->h_size, argp->v_size);
	}

	return STATUS_OK;
}
#endif