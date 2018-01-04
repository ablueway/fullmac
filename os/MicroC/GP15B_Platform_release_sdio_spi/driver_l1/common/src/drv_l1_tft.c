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
#include "drv_l1_tft.h"
#include "drv_l1_timer.h"

#if (defined _DRV_L1_TFT) && (_DRV_L1_TFT == 1)

/**
 * @brief   Initialize TFT controller
 * @param   none
 * @return 	none
 */
void drv_l1_tft_init(void)
{
    R_TFT_CTRL = 0;
    drv_l1_tft_slide_en_set(FALSE); 
}

/**
 * @brief   TFT controller enable
 * @param   enable[in]: 1: enable, 0: disable
 * @return 	none
 */
void drv_l1_tft_en_set(INT32U enable)
{
	if (enable) {
		R_TFT_CTRL |= TFT_EN;
	} else {
		R_TFT_CTRL &= ~TFT_EN;
	}
}

/**
 * @brief   TFT controller set clock 
 * @param   clk[in]: TFT_CLK_DIVIDE_1 ~ TFT_CLK_DIVIDE_32
 * @return 	none
 */
void drv_l1_tft_clk_set(INT32U clk)
{	
	R_TFT_CTRL &= ~TFT_CLK_SEL;
	R_TFT_TS_MISC &= ~0xC0;

	if (clk < TFT_CLK_DIVIDE_9) {
		R_TFT_CTRL |= clk;
	} else {
		R_TFT_CTRL |= clk & 0xF;
		R_TFT_TS_MISC |= (clk & 0x20) << 1;
		R_TFT_TS_MISC |= (clk & 0x10) << 3;
	}
}

/**
 * @brief   TFT set control mode 
 * @param   mode[in]: TFT_MODE_UPS051 ~ TFT_MODE_MEM_ONCE
 * @return 	none
 */
void drv_l1_tft_mode_set(INT32U mode)
{
	R_TFT_CTRL &= ~TFT_MODE;
	R_TFT_CTRL |= mode;		
}

/**
 * @brief   set vsync, hsync, dclk and DE inverse 
 * @param   mask[in]: mask bit
 * @param   value[in]: P_TFT_CTRL signal control bit  
 * @return 	none
 */
void drv_l1_tft_signal_inv_set(INT32U mask, INT32U value)
{
	/*set vsync,hsync,dclk and DE inv */
	R_TFT_CTRL &= ~mask;
	R_TFT_CTRL |= (mask & value);		
}

/**
 * @brief   set horizontal compression  
 * @param   status[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_h_compress_set(INT32U enable)
{
	if (enable) {
		R_TFT_CTRL |= TFT_H_COMPRESS;
	} else {
		R_TFT_CTRL &= ~TFT_H_COMPRESS;
	}
}

/**
 * @brief   set i80 data width 8 or 16bit 
 * @param   data_width[in]: 8 or 16
 * @return 	none
 */
void drv_l1_tft_mem_unit_set(INT8U data_width)
{
	if (data_width == 16) {
		R_TFT_CTRL &= ~TFT_MEM_BYTE_EN; /* word */
	} else {
		R_TFT_CTRL |= TFT_MEM_BYTE_EN;	/* byte */		
	}
}

/**
 * @brief   set tft is interlace or non-interlace
 * @param   interlace[in]: 0 is non interlace, 1 is interlace
 * @return 	none
 */
void drv_l1_tft_interlace_set(INT32U interlace)
{
	if (interlace) {
		R_TFT_CTRL |= TFT_INTERLACE_MOD;
	} else {
		R_TFT_CTRL &= ~TFT_INTERLACE_MOD;
	}
}

/**
 * @brief   set tft unit base dclk or hsync
 * @param   base_dclk[in]: 0: base unit is dclk, 1: base unit is hsync
 * @return 	none
 */
void drv_l1_tft_vsync_unit_set(INT32U base_dclk)
{
	if (base_dclk) {
		R_TFT_CTRL |= TFT_VSYNC_UNIT;
	} else {
		R_TFT_CTRL &= ~TFT_VSYNC_UNIT;
	}
}

/**
 * @brief   set tft data mode
 * @param   mode[in]: TFT_DATA_MODE_8/TFT_DATA_MODE_565/TFT_DATA_MODE_666/TFT_DATA_MODE_888
 * @return 	none
 */
void drv_l1_tft_data_mode_set(INT32U mode)
{
	R_TFT_TS_MISC &= ~TFT_DATA_MODE;
	R_TFT_TS_MISC |= mode;	
}

/**
 * @brief   set R[7:3] switch with B[7:3]
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_rb_switch_set(INT32U enable)
{
	if (enable) {
		R_TFT_TS_MISC |= TFT_SWITCH_EN;
	} else {
		R_TFT_TS_MISC &= ~TFT_SWITCH_EN;
	}
}

/**
 * @brief   set slide window enable
 * @param   enable[in]: enable or disable
 * @return 	none
 */
void drv_l1_tft_slide_en_set(INT32U enable)
{
	if (enable) {
		R_TFT_TS_MISC |= TFT_SLIDE_EN;
	} else {
		R_TFT_TS_MISC &= ~TFT_SLIDE_EN;
	}
}

/**
 * @brief   set TFT DCLK shift selection based on system clock
 * @param   sel[in]: TFT_DCLK_SEL_0 ~ TFT_DCLK_SEL_270
 * @return 	none
 */
void drv_l1_tft_dclk_sel_set(INT32U sel)
{
	R_TFT_TS_MISC &= ~TFT_DCLK_SEL;
	R_TFT_TS_MISC |= sel;		
}

/**
 * @brief   set TFT DCLK shift selection based on DELC cell
 * @param   delay[in]: TFT_DCLK_DELAY_0 ~ TFT_DCLK_DELAY_7
 * @return 	none
 */
void drv_l1_tft_dclk_delay_set(INT32U delay)
{
	R_TFT_TS_MISC &= ~TFT_DCLK_DELAY;
	R_TFT_TS_MISC |= delay;		
}

/**
 * @brief   set TFT display buffer address
 * @param   addr[in]: buffer address
 * @return 	none
 */
void drv_l1_tft_display_buffer_set(INT32U addr)
{
	if(addr) {
		R_TFT_FBI_ADDR = addr;
	}
}

void drv_l1_tft_reg_pol_set(INT8U pol)
{
	if (pol) {
		R_TFT_LINE_RGB_ORDER |= TFT_NEW_POL_EN;	
	} else {
		R_TFT_LINE_RGB_ORDER &= ~TFT_NEW_POL_EN;	
	}
}

/**
 * @brief   set TFT i-80 mode, write command
 * @param   command[in]: command 
 * @return 	none
 */
void drv_l1_tft_i80_wr_cmd(INT16U command)
{
	INT32U reg = R_TFT_CTRL;
	
	R_TFT_MEM_BUFF_WR = command;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_CMD_WR | TFT_EN;
	R_TFT_CTRL = reg;
}

/**
 * @brief   set TFT i-80 mode, write data
 * @param   data[in]: data 
 * @return 	none
 */
void drv_l1_tft_i80_wr_data(INT16U data)
{
	INT32U reg = R_TFT_CTRL;
	
	R_TFT_MEM_BUFF_WR = data;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_DATA_WR | TFT_EN;
	R_TFT_CTRL = reg;
}

/**
 * @brief   set TFT i-80 mode, read data
 * @param   command[in]: command 
 * @return 	data
 */
INT16U drv_l1_tft_i80_read_data(INT16U command)
{
	INT32U reg, data;
	
	drv_l1_tft_i80_wr_cmd(command);
	
	drv_msec_wait(1);
	
	reg = R_TFT_CTRL;
	reg &= ~(0xF << 4);
	reg |= TFT_MODE_MEM_DATA_RD | TFT_EN;
	R_TFT_CTRL = reg;
		
	data = R_TFT_MEM_BUFF_RD;
	return data;
}
#endif //(defined _DRV_L1_TFT) && (_DRV_L1_TFT == 1)

