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
#include "drv_l1_i2s.h"
#include "drv_l1_sfr.h"
#include "drv_l1_dma.h"
#include "drv_l1_timer.h"
#include "gp_stdlib.h"

#if ((defined _DRV_L1_I2S) && (_DRV_L1_I2S == 1))
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Tx Control Register */
#define I2S_TX_EN					(1 << 0)
#define I2S_TX_FIRST_FRAME_R		(1 << 1)
#define I2S_TX_FRAME_POLARITY_L		(1 << 2)
#define I2S_TX_RISING_MODE			(1 << 3)
#define I2S_TX_LSB_MODE				(1 << 4)
#define I2S_TX_LEFT_ALIGN			(1 << 5)

#define I2S_TX_VALID_DATA_16BIT		(0x00 << 6)
#define I2S_TX_VALID_DATA_18BIT		(0x01 << 6)
#define I2S_TX_VALID_DATA_20BIT		(0x02 << 6)
#define I2S_TX_VALID_DATA_22BIT		(0x03 << 6)
#define I2S_TX_VALID_DATA_24BIT		(0x04 << 6)
#define I2S_TX_VALID_DATA_32BIT		(0x05 << 6)
#define I2S_TX_VALID_DATA_08BIT		(0x06 << 6)
#define I2S_TX_VALID_DATA_MASK		(0x07 << 6)

#define I2S_TX_MODE_I2S				(0x00 << 11)
#define I2S_TX_MODE_NORMAL			(0x01 << 11)
#define I2S_TX_MODE_DSP				(0x02 << 11)
#define I2S_TX_MODE_MASK			(0x03 << 11)

#define I2S_TX_SLVMODE				(1 << 13)
#define I2S_TX_IRQ_POLARITY_HI		(1 << 14)
#define I2S_TX_OVWR_DISABLE			(1 << 15)
#define I2S_TX_INT_EN				(1 << 16)

#define I2S_TX_IRQ_FLAG				(1 << 17)
#define I2S_TX_FIFO_CLEAR			(1 << 18)
#define I2S_TX_UNDERFLOW			(1 << 19)
#define I2S_TX_MERGE				(1 << 20)
#define I2S_TX_MERGE_R_LSB			(1 << 21)
#define I2S_TX_MONO_MODE			(1 << 22)

#define I2S_TX_MCLK_DIV_2			(0x02 << 24)
#define I2S_TX_MCLK_DIV_3			(0x03 << 24)
#define I2S_TX_MCLK_DIV_4			(0x04 << 24)
#define I2S_TX_MCLK_DIV_6			(0x06 << 24)
#define I2S_TX_MCLK_DIV_8			(0x07 << 24)
#define I2S_TX_MCLK_DIV_MASK		(0x07 << 24)

#define I2S_TX_FRAMESIZE_16BIT		(0x00 << 28)
#define I2S_TX_FRAMESIZE_24BIT		(0x01 << 28)
#define I2S_TX_FRAMESIZE_32BIT		(0x02 << 28)
#define I2S_TX_FRAMESIZE_48BIT		(0x03 << 28)
#define I2S_TX_FRAMESIZE_64BIT		(0x04 << 28)
#define I2S_TX_FRAMESIZE_96BIT		(0x05 << 28)
#define I2S_TX_FRAMESIZE_128BIT		(0x06 << 28)
#define I2S_TX_FRAMESIZE_176BIT		(0x07 << 28)
#define I2S_TX_FRAMESIZE_192BIT		((INT32U)0x08 << 28)
#define I2S_TX_FRAMESIZE_MASK		((INT32U)0x0F << 28)

/* Rx Control Register */
#define I2S_RX_EN					(1 << 0)
#define I2S_RX_FIRST_FRAME_R		(1 << 1)
#define I2S_RX_FRAME_POLARITY_L		(1 << 2)
#define I2S_RX_RISING_MODE			(1 << 3)
#define I2S_RX_LSB_MODE				(1 << 4)
#define I2S_RX_LEFT_ALIGN			(1 << 5)

#define I2S_RX_VALIDDATA_16BIT		(0x00 << 6)
#define I2S_RX_VALIDDATA_18BIT		(0x01 << 6)
#define I2S_RX_VALIDDATA_20BIT		(0x02 << 6)
#define I2S_RX_VALIDDATA_22BIT		(0x03 << 6)
#define I2S_RX_VALIDDATA_24BIT		(0x04 << 6)
#define I2S_RX_VALIDDATA_32BIT		(0x05 << 6)
#define I2S_RX_VALIDDATA_08BIT		(0x06 << 6)
#define I2S_RX_VALIDDATA_MASK		(0x07 << 6)

#define I2S_RX_FRAMESIZE_16BIT		(0x00 << 9)
#define I2S_RX_FRAMESIZE_24BIT		(0x01 << 9)
#define I2S_RX_FRAMESIZE_32BIT		(0x02 << 9)
#define I2S_RX_FRAMESIZE_AUTO		(0x03 << 9)
#define I2S_RX_FRAMESIZE_MASK		(0x03 << 9)

#define I2S_RX_MODE_I2S				(0x00 << 11)
#define I2S_RX_MODE_NORMAL			(0x01 << 11)
#define I2S_RX_MODE_DSP				(0x02 << 11)
#define I2S_RX_MODE_MASK			(0x03 << 11)

#define I2S_RX_MASTER_MODE			(1 << 13)
#define I2S_RX_IRQ_POLARITY_HI		(1 << 14)
#define I2S_RX_OVWR_CLEAR			(1 << 15)
#define I2S_RX_INT_EN				(1 << 16)
#define I2S_RX_IRQ_FLAG				(1 << 17)
#define I2S_RX_FIFO_CLEAR			(1 << 18)
#define I2S_RX_MERGE				(1 << 19)
#define I2S_RX_MERGE_R_LSB			(1 << 20)
#define I2S_RX_MONO_MODE			(1 << 21)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		gp_memset((INT8S *)ptr, 0x00, cblen)

#define APPL_ENABLE				(R_SYSTEM_CKGEN_CTRL |= 0x10)
#define APPL_DISABLE			(R_SYSTEM_CKGEN_CTRL &= ~(0x10))

#define MCLK_PLL_67MHz 			(R_SYSTEM_CKGEN_CTRL |= 0x80)
#define MCLK_PLL_73MHz 			(R_SYSTEM_CKGEN_CTRL &= (~0x80))

#define MCLK_PLL_DIV_MASK 		(R_SYSTEM_MISC_CTRL2 &= (~0xF000))
#define MCLK_PLL_DIV4 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0x3000); }
#define MCLK_PLL_DIV6 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0x5000); }
#define MCLK_PLL_DIV8 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0x7000); }
#define MCLK_PLL_DIV9 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0x8000); }
#define MCLK_PLL_DIV12 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0xB000); }
#define MCLK_PLL_DIV16 			{ MCLK_PLL_DIV_MASK; (R_SYSTEM_MISC_CTRL2 |= 0xF000); }

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static DMA_STRUCT i2s_tx_dma_dbf;
static DMA_STRUCT i2s_rx_dma_dbf;

/**************************************************************************
 *                         T X     A P I    	                          *
 **************************************************************************/
/**
 * @brief   i2s tx initialize
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_tx_init(void)
{
	R_I2STX_CTRL = I2S_TX_UNDERFLOW | I2S_TX_IRQ_FLAG;
	R_I2STX_CTRL = 0x14004A20; // default value	
	APPL_ENABLE;
}	

/**
 * @brief   i2s tx exit
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_tx_exit(void)
{
	R_I2STX_CTRL = I2S_TX_UNDERFLOW | I2S_TX_IRQ_FLAG;
	R_I2STX_CTRL = 0x14004A20; // default value
	APPL_DISABLE;
}	

/**
 * @brief   i2s tx set first frame 
 * @param   mode[in]: I2S_FIRST_FRAME_L or I2S_FIRST_FRAME_R
 * @return 	none
 */
void drv_l1_i2s_tx_set_firstframe(I2S_FIRST_FRAME_ENUM mode)
{	
	if(mode == I2S_FIRST_FRAME_R) {	
		R_I2STX_CTRL |= I2S_TX_FIRST_FRAME_R;	// FirstFrame to be RIGHT
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_FIRST_FRAME_R; // FirstFrame to be LEFT
	}
}

/**
 * @brief   i2s tx set frame polarity 
 * @param   mode[in]: I2S_FRAME_POLARITY_L or I2S_FRAME_POLARITY_R
 * @return 	none
 */
void drv_l1_i2s_tx_set_framepolarity(I2S_FRAME_POLARITY_ENUM mode)
{
	if(mode == I2S_FRAME_POLARITY_L) {	
		R_I2STX_CTRL |= I2S_TX_FRAME_POLARITY_L;	// LRCK=0 is left frame
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_FRAME_POLARITY_L;	// LRCK=0 is right frame
	}
}

/**
 * @brief   i2s tx set edge mode 
 * @param   mode[in]: I2S_FALLING_EDGE or I2S_RISING_EDGE
 * @return 	none
 */
void drv_l1_i2s_tx_set_edgemode(I2S_EDGE_MODE_ENUM mode)
{
	if(mode == I2S_RISING_EDGE) {	
		R_I2STX_CTRL |= I2S_TX_RISING_MODE;		// SCLK is rising edge
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_RISING_MODE;	// SCLK is falling edge
	}	
}

/**
 * @brief   i2s tx set data sequence 
 * @param   mode[in]: I2S_MSB_MODE or I2S_LSB_MODE
 * @return 	none
 */
void drv_l1_i2s_tx_set_sendmode(I2S_SEND_MODE_ENUM mode)
{
	if(mode == I2S_LSB_MODE) {	
		R_I2STX_CTRL |= I2S_TX_LSB_MODE;	// LSB first
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_LSB_MODE;	// MSB first
	}
}

/**
 * @brief   i2s tx set data alignemt 
 * @param   mode[in]: I2S_ALIGN_LEFT or I2S_ALIGN_RIGHT
 * @return 	none
 */
void drv_l1_i2s_tx_set_align(I2S_ALIGN_ENUM mode)
{
	if(mode == I2S_ALIGN_LEFT) {	
		R_I2STX_CTRL |= I2S_TX_LEFT_ALIGN;	// Left Align
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_LEFT_ALIGN;	// Right Align
	}
}

/**
 * @brief   i2s tx set valid data size
 * @param   mode[in]: I2S_TX_VALID_DATA_16BIT ~ I2S_TX_VALID_DATA_24BIT
 * @return 	none
 */
void drv_l1_i2s_tx_set_validdata(I2S_VALID_DATA_ENUM data)
{
	INT32U reg;
	
	reg = R_I2STX_CTRL;
	reg &= ~I2S_TX_VALID_DATA_MASK;
	switch(data)
	{
	case I2S_VALID_16:
		reg |= I2S_TX_VALID_DATA_16BIT;
		break;
		
	case I2S_VALID_18:
		reg |= I2S_TX_VALID_DATA_18BIT;
		break;
		
	case I2S_VALID_20:	
		reg |= I2S_TX_VALID_DATA_20BIT;	
		break;
		
	case I2S_VALID_22:
		reg |= I2S_TX_VALID_DATA_22BIT;	
		break;
	
	case I2S_VALID_24:
		reg |= I2S_TX_VALID_DATA_24BIT;
		break;
	
	case I2S_VALID_32:
		reg |= I2S_TX_VALID_DATA_32BIT; 
		break;
		
	case I2S_VALID_8:
		reg |= I2S_TX_VALID_DATA_08BIT;
		break;
		
	default:
		reg |= I2S_TX_VALID_DATA_16BIT;
	}
	R_I2STX_CTRL = reg;	
}

/**
 * @brief   i2s tx set spu input speed
 * @param   sample_rate[in]: sample rate
 * @return 	none
 */
void drv_l1_i2s_tx_spu_fs(INT32U sample_rate)
{
	INT32U reg = R_I2STX_CTRL;
	
	reg &= ~(0x03 << 9);
	switch(sample_rate)
	{
	case 44100:
		reg |= 0 << 9;	//output speed=I2S_MCLK/(256/3)
		break;
		
	case 48000:
		reg |= 1 << 9;	//output speed=I2S_MCLK/(384/3)
		break;
	
	case 22050:
		reg |= 2 << 9;	//output speed=I2S_MCLK/(512/3)
		break;
	
	case 24000:
		reg |= 3 << 9;	//output speed=I2S_MCLK/(768/3)
		break;
	}

	R_I2STX_CTRL = reg;
}

/**
 * @brief   i2s tx set i2s mode
 * @param   mode[in]: I2S_MODE_I2S / I2S_MODE_NORMAL or I2S_MODE_DSP
 * @return 	none
 */
void drv_l1_i2s_tx_set_i2smode(I2S_MODE_ENUM mode)
{
	INT32U reg;
	
	reg = R_I2STX_CTRL;
	reg &= ~I2S_TX_MODE_MASK;
	switch(mode)
	{
	case I2S_MODE_I2S:
		reg |= I2S_TX_MODE_I2S;
		break;
	
	case I2S_MODE_NORMAL:
		reg |= I2S_TX_MODE_NORMAL;
		break;
		
	case I2S_MODE_DSP:
		reg |= I2S_TX_MODE_DSP;
		break;
		
	default:
		reg |= I2S_TX_MODE_I2S;
	}
	
	R_I2STX_CTRL = reg;
}

/**
 * @brief   i2s tx set master mode, when master will driving LRCK out. 
 * @param   mode[in]: I2S_MASTER or I2S_SLAVE
 * @return 	none
 */
void drv_l1_i2s_tx_set_master(I2S_MASTER_MODE_ENUM mode)
{
	if(mode == I2S_SLAVE) {	
		R_I2STX_CTRL |= I2S_TX_SLVMODE;		// acts as Slave
	} else {
		R_I2STX_CTRL &= ~I2S_TX_SLVMODE;	// acts as Master and driving LRCK out
	}
}

/**
 * @brief   i2s tx set interrupt polarity 
 * @param   mode[in]: I2S_IRT_POLARITY_L or I2S_IRT_POLARITY_H
 * @return 	none
 */
void drv_l1_i2s_tx_set_intpolarity(I2S_IRT_POLARITY_ENUM mode)
{
	if(mode == I2S_IRT_POLARITY_H) {	
		R_I2STX_CTRL |= I2S_TX_IRQ_POLARITY_HI;	// interrupt is high active
	} else {
		R_I2STX_CTRL &= ~I2S_TX_IRQ_POLARITY_HI;	// interrupt is low active
	}
}

/**
 * @brief   i2s tx set fifo over write interrupt enable 
 * @param   mode[in]: I2S_OVERWRITE_ENABLE or I2S_OVERWRITE_DISABLE
 * @return 	none
 */
void drv_l1_i2s_tx_set_over_write(I2S_OVWR_ENUM mode)
{
	if(mode == I2S_OVERWRITE_DISABLE) {	
		R_I2STX_CTRL |= I2S_TX_OVWR_DISABLE;	// Disable overwritten
	} else {
		R_I2STX_CTRL &= ~I2S_TX_OVWR_DISABLE;	// enable over written
	}
}

/**
 * @brief   i2s tx set interrupt enable 
 * @param   mode[in]: I2S_INT_DISABLE or I2S_INT_ENABLE
 * @return 	none
 */
void drv_l1_i2s_tx_set_int(I2S_INT_ENUM mode)
{
	if(mode == I2S_INT_ENABLE) {	
		R_I2STX_CTRL |= I2S_TX_INT_EN;	// enable interrupt
	} else {
		R_I2STX_CTRL &= ~I2S_TX_INT_EN;// disable interrupt
	}
}

/**
 * @brief   i2s tx fifo clear 
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_tx_fifo_clear(void)
{
	R_I2STX_CTRL |= I2S_TX_FIFO_CLEAR;	//auto clear to 0	
}

/**
 * @brief   i2s tx set data merge mode only valid when use 16bit mode 
 * @param   mode[in]: I2S_MERGE_DISABLE or I2S_MERGE_ENABLE
 * @return 	none
 */
void drv_l1_i2s_tx_set_merge(I2S_MERGE_ENUM mode)
{
	/* This bit only valid when I2S valid word length is 16-bit wide */
	if(mode == I2S_MERGE_ENABLE) {
		R_I2STX_CTRL |= I2S_TX_MERGE;
	} else {
		R_I2STX_CTRL &= ~I2S_TX_MERGE;
	}	
}

/**
 * @brief   i2s tx set mono or stereo channel 
 * @param   mode[in]: I2S_MODE_STEREO or I2S_MODE_MONO
 * @return 	none
 */
void drv_l1_i2s_tx_set_mono(I2S_MONO_MODE_ENUM mode)
{
	if(mode == I2S_MODE_MONO) {	
		R_I2STX_CTRL |= I2S_TX_MONO_MODE;	// set as mono
	} else {		
		R_I2STX_CTRL &= ~I2S_TX_MONO_MODE;	// set as stereo
	}	
}

/**
 * @brief   i2s tx set frame size 
 * @param   mode[in]: I2S_FRAMESIZE_16 ~ I2S_FRAMESIZE_192
 * @return 	none
 */
void drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_ENUM mode)
{
	INT32U reg;

	reg = R_I2STX_CTRL;
	reg &= ~I2S_TX_FRAMESIZE_MASK;
	switch(mode)
	{
		case I2S_FRAMESIZE_16:
			reg |= I2S_TX_FRAMESIZE_16BIT;
			break;

		case I2S_FRAMESIZE_24:
			reg |= I2S_TX_FRAMESIZE_24BIT;
			break;

		case I2S_FRAMESIZE_32:
			reg |= I2S_TX_FRAMESIZE_32BIT;
			break;

		case I2S_FRAMESIZE_48:
			reg |= I2S_TX_FRAMESIZE_48BIT;
			break;

		case I2S_FRAMESIZE_64:
			reg |= I2S_TX_FRAMESIZE_64BIT;
			break;

		case I2S_FRAMESIZE_96:
			reg |= I2S_TX_FRAMESIZE_96BIT;
			break;

		case I2S_FRAMESIZE_128:
			reg |= I2S_TX_FRAMESIZE_128BIT;
			break;

		case I2S_FRAMESIZE_176:
			reg |= I2S_TX_FRAMESIZE_176BIT;
			break;

		case I2S_FRAMESIZE_192:
			reg |= I2S_TX_FRAMESIZE_192BIT;
			break;

		default:
			reg |= I2S_TX_FRAMESIZE_32BIT;
	}

	R_I2STX_CTRL = reg;
}

/**
 * @brief   i2s tx set MCLK divider 
 * @param   mode[in]: I2S_MCLK_DIV_2 ~ I2S_MCLK_DIV_8
 * @return 	none
 */
void drv_l1_i2s_tx_set_mclk_div(I2S_MCLK_DIV_ENUM mode)
{
	INT32U reg;

	reg = R_I2STX_CTRL;
	reg &= ~I2S_TX_MCLK_DIV_MASK;
	switch(mode)
	{
	case I2S_MCLK_DIV_2:
		reg |= I2S_TX_MCLK_DIV_2;
		break;

	case I2S_MCLK_DIV_3:
		reg |= I2S_TX_MCLK_DIV_3;
		break;

	case I2S_MCLK_DIV_4:
		reg |= I2S_TX_MCLK_DIV_4;
		break;

	case I2S_MCLK_DIV_6:
		reg |= I2S_TX_MCLK_DIV_6;
		break;

	case I2S_MCLK_DIV_8:
		reg |= I2S_TX_MCLK_DIV_8;
		break;

	default:
		reg |= I2S_TX_MCLK_DIV_4;
	}

	R_I2STX_CTRL = reg;
}

/**
 * @brief   i2s tx set sample rate 
 * @param   hz[in]: sample rate
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_sample_rate_set(INT32U hz)
{
	drv_l1_i2s_tx_set_mclk_div(I2S_MCLK_DIV_4);

	switch(hz)
	{
	case 8000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_192);
		break;

	case 12000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_128);
		break;

	case 16000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_96);
		break;

	case 24000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_64);
		break;

	case 32000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_48);
		break;

	case 48000:
		MCLK_PLL_73MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_32);
		break;

	case 11025:
		MCLK_PLL_67MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_128);
		break;

	case 22050:
		MCLK_PLL_67MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_64);
		break;

	case 44100:
		MCLK_PLL_67MHz;
		drv_l1_i2s_tx_set_framesize(I2S_FRAMESIZE_32);
		break;
		
	default:
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

/**
 * @brief   i2s tx set start transfer 
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_start(void)
{
	R_I2STX_CTRL |= I2S_TX_FIFO_CLEAR;	// clear FIFO

	R_I2STX_CTRL |= I2S_TX_EN; 		// enable I2S_TX

	return STATUS_OK;
}

/**
 * @brief   i2s tx set stop transfer 
 * @param   none
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_stop(void)
{
	R_I2STX_CTRL &= (~I2S_TX_EN); 	// Disable I2S_TX

	R_I2STX_CTRL |= I2S_TX_FIFO_CLEAR;	// clear FIFO	

	return STATUS_OK;
}

/**
 * @brief   i2s tx dma double buffer put 
 * @param   data[in]: buffer 
 * @param   cwlen[in]: length in short unit
 * @param   os_q[in]: os queue
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_dbf_put(INT16S *data, INT32U cwlen, OS_EVENT *os_q)
{
	INT32S status;
	
	CLEAR(&i2s_tx_dma_dbf, sizeof(DMA_STRUCT));
	i2s_tx_dma_dbf.s_addr = (INT32U) data;
	i2s_tx_dma_dbf.t_addr = (INT32U) P_I2STX_DATA;
	i2s_tx_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;		
	i2s_tx_dma_dbf.count = (INT32U) cwlen/2;
	i2s_tx_dma_dbf.notify = NULL;
	i2s_tx_dma_dbf.timeout = 0;
	status = drv_l1_dma_transfer_with_double_buf(&i2s_tx_dma_dbf, os_q);
	if(status < 0) {
		return status;
	}
	
	return STATUS_OK;
}

/**
 * @brief   i2s tx dma double buffer set 
 * @param   data[in]: buffer 
 * @param   cwlen[in]: length in short unit
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_tx_dbf_set(INT16S *data, INT32U cwlen)
{
	INT32S status;

	i2s_tx_dma_dbf.s_addr = (INT32U) data;
	i2s_tx_dma_dbf.t_addr = (INT32U) P_I2STX_DATA;
	i2s_tx_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;		
	i2s_tx_dma_dbf.count = (INT32U) cwlen/2;
	i2s_tx_dma_dbf.notify = NULL;
	i2s_tx_dma_dbf.timeout = 0;
	status = drv_l1_dma_transfer_double_buf_set(&i2s_tx_dma_dbf);
	if(status < 0) {
		return status;
	}
	
	return STATUS_OK;
}

/**
 * @brief   i2s get tx dma double buffer status  
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_tx_dbf_status_get(void)
{
	if (i2s_tx_dma_dbf.channel == 0xff) {
		return -1;
	}
	
	return drv_l1_dma_dbf_status_get(i2s_tx_dma_dbf.channel);	
}

/**
 * @brief   i2s get tx dma status  
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_tx_dma_status_get(void)
{
	if (i2s_tx_dma_dbf.channel == 0xff) {
		return -1;
	}
	
	return drv_l1_dma_status_get(i2s_tx_dma_dbf.channel);	
}

/**
 * @brief   i2s free tx dma channel  
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_tx_dbf_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&i2s_tx_dma_dbf);
	i2s_tx_dma_dbf.channel = 0xff;
}

/**************************************************************************
 *                         R X     A P I    	                          *
 **************************************************************************/ 
/**
 * @brief   i2s rx form mic or line in LR path 
 * @param   path[in]: MIC_INPUT or LINE_IN_LR
 * @return 	none
 */  
void drv_l1_i2s_rx_set_input_path(I2S_RX_INPUT_PATH path)
{
	// check CODEC LDO enable
	if((R_SYSTEM_POWER_CTRL0 & 0x100) == 0) {
		R_SYSTEM_POWER_CTRL0 |= 0x700;
	}
	
	// R_MIC_SETUP
	//LNINGR: 	[15:11], (值愈小，音量愈大)
	//LNINGL:	[10:6]	 (值愈小，音量愈大)
	//ADHP: 	[5]
	//ENMICBIAS:[4]
	//ENLNIN: 	[3]
	//MICEN: 	[2] 
	//ENADR: 	[1]
	//ENADL: 	[0]
	
	//R_MIC_BOOST 
	//MIC_PGA :	[4:0](default 0), MIC PGA  (值愈小，音量愈大)
	//MIC_BOOST:[8]  (default 1), MIC boost	（1:ON, 0:OFF)
	//USE_SIGN: [11] (default 0)
	//USE_LR: 	[10] (default 0)
	
	// turn off ADC
	R_MIC_SETUP &= (~0x1F);
	
	if (path == MIC_INPUT) {		
		R_MIC_SETUP |= 0x17;  	// MIC In, enable I2S RX clock, enable Vref
		R_MIC_BOOST |= 0x118;  	// turn on boost and set gain
	} else {
		R_MIC_SETUP |= 0x1B;  	// LineIn
		R_MIC_SETUP |= 0x8400;	// adjust volume
	}
	
	// wait vref ready
	drv_msec_wait(200);
}
  
/**
 * @brief   i2s rx initalize 
 * @param   none
 * @return 	none
 */ 
void drv_l1_i2s_rx_init(void)
{	
	// APLL EN  (audio PLL)
	APPL_ENABLE;
	
	// set i2s for mic or line in 
	R_I2SRX_CTRL |= I2S_RX_IRQ_FLAG | I2S_RX_OVWR_CLEAR;
	R_I2SRX_CTRL = 0x4820;					// Reset Value
	R_I2SRX_CTRL &= ~I2S_RX_MODE_MASK;		// I2S Mode
	R_I2SRX_CTRL |= I2S_RX_MODE_NORMAL;		// must Normal Mode because of front end design.
	R_I2SRX_CTRL |= I2S_RX_MERGE;  			// MERGE Mode
}

/**
 * @brief   i2s rx exit 
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_exit(void)
{	
	R_I2SRX_CTRL = 0x4820;		// Reset Value
	
	// APLL disable	
	APPL_DISABLE;
	
	// turn off MIC
	R_MIC_SETUP &= (~0x1F);
	R_MIC_BOOST &= (~0x11F);
}	

/**
 * @brief   i2s rx clear fifo 
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_fifo_clear(void)
{
	R_I2SRX_CTRL |= I2S_RX_FIFO_CLEAR;	// clear FIFO
	R_I2SRX_CTRL &= ~I2S_RX_FIFO_CLEAR;
}

/**
 * @brief   i2s rx set sample rate 
 * @param   hz[in]: smaple rate
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_sample_rate_set(INT32U hz)
{
	switch(hz)
	{
	case 48000:
		MCLK_PLL_73MHz;
		MCLK_PLL_DIV6;
		break;
		
	case 44100:
		MCLK_PLL_67MHz;
		MCLK_PLL_DIV6;
		break;
		
	case 32000:
		MCLK_PLL_73MHz;
		MCLK_PLL_DIV9;
		break;
		
	case 24000:
		MCLK_PLL_73MHz;
		MCLK_PLL_DIV12;
		break;
	
	case 22050:
		MCLK_PLL_67MHz;
		MCLK_PLL_DIV12;	
		break;
		
	case 16000: //Inaccurate when use 16K 
		MCLK_PLL_67MHz;
		MCLK_PLL_DIV16;	
		break;
		
	default:
		MCLK_PLL_67MHz;
		MCLK_PLL_DIV12;	
		return STATUS_FAIL;		
	}

	return STATUS_OK;
}

/**
 * @brief   i2s rx set mono channel 
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_mono_ch_set(void)
{
	R_I2SRX_CTRL |= I2S_RX_MONO_MODE;	// MONO mode
	return STATUS_OK;
}

/**
 * @brief   i2s rx set start 
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_start(void)
{
	R_I2SRX_CTRL |= I2S_RX_FIFO_CLEAR;	// clear FIFO
	R_I2SRX_CTRL &= ~I2S_RX_FIFO_CLEAR;

	R_I2SRX_CTRL |= I2S_RX_EN; 			// enable I2S_RX
	return STATUS_OK;
}

/**
 * @brief   i2s rx set stop
 * @param   none
 * @return 	STATUS_OK or STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_stop(void)
{
	R_I2SRX_CTRL &= ~I2S_RX_EN; 		// disable I2S_RX
	R_I2SRX_CTRL |= I2S_RX_FIFO_CLEAR;	// clear FIFO
	
	R_I2SRX_CTRL &= ~I2S_RX_FIFO_CLEAR;
	return STATUS_OK;
}

/**
 * @brief   i2s rx dma double buffer put 
 * @param   data[in]: buffer 
 * @param   cwlen[in]: length in short unit
 * @param   os_q[in]: os queue
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_dbf_put(INT16S *data, INT32U cwlen, OS_EVENT *os_q)
{
	INT32S status;
	
	CLEAR(&i2s_rx_dma_dbf, sizeof(DMA_STRUCT));
	i2s_rx_dma_dbf.s_addr = (INT32U) P_I2SRX_DATA;
	i2s_rx_dma_dbf.t_addr = (INT32U) data;
	i2s_rx_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;		
	i2s_rx_dma_dbf.count = (INT32U) cwlen/2;
	i2s_rx_dma_dbf.notify = NULL;
	i2s_rx_dma_dbf.timeout = 0;
	status = drv_l1_dma_transfer_with_double_buf(&i2s_rx_dma_dbf, os_q);
	if(status < 0) {
		return status;
	}
	
	return STATUS_OK;
}

/**
 * @brief   i2s rx dma double buffer sut 
 * @param   data[in]: buffer 
 * @param   cwlen[in]: length in short unit
 * @return 	STATUS_OK / STATUS_FAIL
 */
INT32S drv_l1_i2s_rx_dbf_set(INT16S *data, INT32U cwlen)
{
	INT32S status;

	i2s_rx_dma_dbf.s_addr = (INT32U) P_I2SRX_DATA;
	i2s_rx_dma_dbf.t_addr = (INT32U) data;
	i2s_rx_dma_dbf.width = DMA_DATA_WIDTH_4BYTE;		
	i2s_rx_dma_dbf.count = (INT32U) cwlen/2;
	i2s_rx_dma_dbf.notify = NULL;
	i2s_rx_dma_dbf.timeout = 0;
	status = drv_l1_dma_transfer_double_buf_set(&i2s_rx_dma_dbf);
	if(status < 0) {
		return status;
	}
	
	return STATUS_OK;
}

/**
 * @brief   i2s get rx dma double buffer status  
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_dbf_status_get(void)
{
	if (i2s_rx_dma_dbf.channel == 0xff) {
		return -1;
	}
	
	return drv_l1_dma_dbf_status_get(i2s_rx_dma_dbf.channel);	
}

/**
 * @brief   i2s get rx dma status  
 * @param   none
 * @return 	0: idle, 1: busy, -1: fail
 */
INT32S drv_l1_i2s_rx_dma_status_get(void)
{
	if (i2s_rx_dma_dbf.channel == 0xff) {
		return -1;
	}
	
	return drv_l1_dma_status_get(i2s_rx_dma_dbf.channel);	
}

/**
 * @brief   i2s free rx dma channel  
 * @param   none
 * @return 	none
 */
void drv_l1_i2s_rx_dbf_free(void)
{
	drv_l1_dma_transfer_double_buf_free(&i2s_rx_dma_dbf);
	i2s_rx_dma_dbf.channel = 0xff;
}
#endif //(defined _DRV_L1_I2S_TX) && (_DRV_L1_I2S_TX == 1)       


