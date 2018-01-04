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
/******************************************************************** 
* Purpose:  I2C driver/interface
* Author: 
* Date: 	2014/09/01
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
#include "drv_l1_sfr.h"
#include "drv_l1_i2c.h"
#include "drv_l1_timer.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
//#if (defined _DRV_L1_I2C) && (_DRV_L1_I2C == 1)	
/****************************************************
*		Definition 									*
****************************************************/
#define DBG_I2C_ENABLE		0

#if DBG_I2C_ENABLE == 1
#define DRV_I2C_DBG		DBG_PRINT
#else
#define DRV_I2C_DBG(...)
#endif

/****************************************************
*		varaible and data declaration				*
****************************************************/
extern INT32U MCLK;
#if _OPERATING_SYSTEM == _OS_UCOS2
static OS_EVENT *I2C_Sema;
#endif

/*****************************************
*			Local functions				 *
*****************************************/
static void _tiny_while(INT8U arg1, INT8U arg2)
{
	INT32U i;
	INT32U loopCnt = 0xf; // [200k]0x4FF;  // [100k]0x3FF;
	for(i=0;i<loopCnt;i++);
}

static INT32S _i2c_busy_waiting(INT32S ms)
{
	INT32S timeout = ms * 75;
	
	while((R_I2C_ICSR & MASK_I2C_SR_BUSYSTA))
	{
		timeout--;
		if(timeout < 0)
		{
			return -1;
		}
		_tiny_while(0,5);
//		drv_msec_wait(1);
	}
	
	return 0;
}

INT32S _i2c_intpend_waiting(INT32S ms)
{
	INT32S timeout = ms * 75;
	
	while((R_I2C_ICCR & MASK_I2C_CR_INTRPEND) == 0)
	{
		timeout--;
		if(timeout < 0)
		{
			return -1;
		}
		_tiny_while(0,5);
//		drv_msec_wait(1);
	}

	return 0;
}

static void _i2cSetClkRate(INT16U clkRate)
{
	//==================================================================================
	// I2C_Clock = SYSCLK /( { ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) + 1 } * 2)
	// ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) means [4 + 8 = 12] bits combined as following
	//
	// -------------------------------------------------
	// | TXCLKPRE[3:0] |         TXCLKLSB[7:0]         |
	// -------------------------------------------------
	// | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	// -------------------------------------------------
	//
	// ( TXCLKPRE[3:0], TXCLKLSB[7:0] ) = ( SYSCLK / 2 / I2C_Clock ) - 1
	//==================================================================================
	INT32U tmp=0;
	
	tmp = MCLK / (2*clkRate*1000);
	if (tmp > 0xFFF) {
		tmp = 0xFFF;
	} else if (tmp > 0) {
		tmp--;
	}
	
	// Setup TXCLKLSB[7:0]
	R_I2C_TXCLKLSB = (tmp & 0xFF);
	
	// Setup TXCLKPRE[3:0] in ICCR
	R_I2C_ICCR &= ~0xF;
	R_I2C_ICCR |= ((tmp & 0xF00) >> 8);
}

INT32S _i2cStartTran(INT16U slaveAddr, INT16U clkRate, INT8U cmd, INT8U aAck)
{
	INT32S ret = 1;
	INT32U iccr = 0, ctrl = 0;
	INT32S timeout;

	/* check i2c bus is idle or not */
	ret = _i2c_busy_waiting(I2C_TIME_OUT);
	if (ret < 0) {
		DRV_I2C_DBG("[%s]-- I2C bus is busy\r\n", __func__);
		return -1;
	}

	_i2cSetClkRate(clkRate);
	
	iccr = R_I2C_ICCR;
	//iccr = (iccr & I2C_ICCR_TXCLKMSB_MASK) | MASK_I2C_CR_INTREN;
	// [TODO]
	// TXCLKMSB_MASK was set in _i2cSetClkRate() and I2C does not use interrupt.
	// The following line should be un-necessary.
	//												by Craig 2012.6.28
	iccr &= I2C_ICCR_TXCLKMSB_MASK | MASK_I2C_CR_INTREN;

	switch (cmd)
	{
		case I2C_BUS_WRITE:
			iccr |= I2C_ICCR_INIT;
			ctrl = I2C_ICSR_MASTER_TX | MASK_I2C_SR_DOEN;
			break;

		case I2C_BUS_READ:
			iccr |= MASK_I2C_CR_ACKEN;
			ctrl = I2C_ICSR_MASTER_RX | MASK_I2C_SR_DOEN;
			break;

		default:
			return -1;
			break;
	}
	
	R_I2C_ICCR = iccr;
	R_I2C_ICSR = ctrl;
	
	if (cmd == I2C_BUS_READ)
	{
		R_I2C_IDSR = (slaveAddr & 0xFF) | 0x01;
	}
	else
	{
		R_I2C_IDSR = slaveAddr & 0xFE;
	}
	
	DRV_I2C_DBG("[%s]-- I2C Start to Send Start Signal\r\n", __func__);
	R_I2C_ICSR |= MASK_I2C_SR_BUSYSTA;
	
	ret = _i2c_intpend_waiting(I2C_TIME_OUT);
	if (ret < 0)
	{
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}
	
	timeout = NO_ACK_TIMEOUT * 75;
	while ((R_I2C_ICSR & MASK_I2C_SR_LRBIT) && aAck)
	{
		DRV_I2C_DBG("[%s]-- Waiting for ACK\r\n", __func__);
		timeout--;
		if(timeout < 0)
		{
			DRV_I2C_DBG("[%s]-- Waiting ACK timeout\r\n", __func__);
			ret = -1;
			break;
		}
		_tiny_while(0,5);
	}
	return ret;
}

static void _i2cStopTran(INT8U cmd)
{
	INT32S ret=0;
	INT32U ctrl = 0;
	
	if (cmd == I2C_BUS_WRITE) {
		ctrl = I2C_ICSR_MASTER_TX | MASK_I2C_SR_DOEN;
	} else {
		ctrl = I2C_ICSR_MASTER_RX | MASK_I2C_SR_DOEN;
	}
		
	R_I2C_ICSR = ctrl;

	DRV_I2C_DBG("[%s]-- Stop transmition\r\n", __func__);
	ret = _i2c_busy_waiting(I2C_TIME_OUT);
	if (ret < 0)
	{
		DRV_I2C_DBG("[%s]-- I2C bus is busy\r\n", __func__);
		return;
	}
}

INT32S _i2cMiddleTran(INT8U *data, INT8U cmd, INT8U aAck)
{
	INT32S ret = 1;
	INT32U iccr = 0;
	INT32S timeout;
	
	iccr = R_I2C_ICCR;
	iccr &= I2C_ICCR_TXCLKMSB_MASK;
	
	switch (cmd)
	{
		case I2C_BUS_WRITE:
			R_I2C_IDSR = *data & 0xFF;
			iccr |= MASK_I2C_CR_INTRPEND;
			break;
		
		case I2C_BUS_READ:
			if (aAck == 1)
				iccr |= MASK_I2C_CR_ACKEN | MASK_I2C_CR_INTRPEND;
			else
				iccr |= MASK_I2C_CR_INTRPEND;
			break;
		
		default:
			return -1;
			break;
	}
	
	/* clear irq */
	R_I2C_ICCR = iccr;

	// Interrupt Pending
	ret = _i2c_intpend_waiting(I2C_TIME_OUT);
	if (ret < 0)
	{
		DRV_I2C_DBG("[%s]-- Interrupt is not received\r\n", __func__);
		return -1;
	}

	timeout = NO_ACK_TIMEOUT * 75;
	// Ack Received ?
	while ( (R_I2C_ICSR & MASK_I2C_SR_LRBIT ) && aAck )
	{
		DRV_I2C_DBG("[%s]-- Waiting for ACK\r\n", __func__);
		timeout--;
		if(timeout < 0)
		{
			DRV_I2C_DBG("[%s]-- Waiting ACK timeout\r\n", __func__);
			ret = -1;
			break;
		}
		_tiny_while(0,5);
	}

	*data = (R_I2C_IDSR & 0xFF);
	return ret;
}

static INT32S _i2c_bus_xfer(drv_l1_i2c_bus_handle_t *hd, INT8U* data, INT8U len, INT8U cmd)
{
	INT32S i=0, ret = -1;
	
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT8U err;
	
	OSSemPend(I2C_Sema, 0, &err); // lock
#endif
	
	if (hd->clkRate == 0)
	{
		DRV_I2C_DBG("[%s]-- Error: i2c clock rate must to be more than zero\r\n", __func__);
		goto __exit;
	}
	
	if (cmd == I2C_BUS_WRITE)
	{
		ret = _i2cStartTran(hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 0);
		// [TODO] OID sensor should wait for ACK. Why the code is no ACK?
		//ret = _i2cStartTran(hd->slaveAddr, hd->clkRate, I2C_BUS_WRITE, 1);
		if (ret < 0)
		{
			DRV_I2C_DBG("WRITE-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}
		
		for (i = 0; i < len; i++)
		{
			// [TODO] OID sensor should wait for ACK. Why the code is no ACK for last byte?
			if (i==(len-1)) {
				ret = _i2cMiddleTran((data+i), I2C_BUS_WRITE, 0);
			} else {
				ret = _i2cMiddleTran((data+i), I2C_BUS_WRITE, 1);
			}
			
			if (ret < 0)
			{
				DRV_I2C_DBG("WRITE-Error: write data fail\r\n", __func__);
				goto __exit;
			}
		}
		
		_i2cStopTran(I2C_BUS_WRITE);
	}
	else if (cmd == I2C_BUS_READ)
	{
		ret = _i2cStartTran(hd->slaveAddr, hd->clkRate, I2C_BUS_READ, 1);
		if (ret < 0)
		{
			DRV_I2C_DBG("READ-Error: write slave device address fail\r\n", __func__);
			goto __exit;
		}
		
		for (i = 0; i < len; i++)
		{
			if (i == (len-1)) {
				ret = _i2cMiddleTran((data+i), I2C_BUS_READ, 0);
			} else {
				ret = _i2cMiddleTran((data+i), I2C_BUS_READ, 1);
			}
				
			if (ret < 0)
			{
				DRV_I2C_DBG("READ-Error: read data fail\r\n", __func__);
				goto __exit;
			}
		}
		
		_i2cStopTran(I2C_BUS_READ);
	}
	
	ret = i;
__exit:
#if _OPERATING_SYSTEM == _OS_UCOS2
	OSSemPost(I2C_Sema); // unlock
#endif	
	return ret;
}

/*****************************************
*			I2C APIs    				 *
*****************************************/
/**
 * @brief   driver layer 1 I2C initilization
 * @param   none
 * @return 	none
 */
void drv_l1_i2c_init(void)
{
#if _OPERATING_SYSTEM == _OS_UCOS2	
	if(I2C_Sema == 0) {
		I2C_Sema = OSSemCreate(1);
	}
#endif

	R_I2C_MISC = MASK_I2C_MISC_GPIOEN;
	R_I2C_ICCR = I2C_ICCR_INIT;
	R_I2C_IDEBCLK = I2C_IDEBCLK_INIT;
}

/**
 * @brief   driver layer 1 I2C un-initilization
 * @param   none
 * @return 	none
 */
void drv_l1_i2c_uninit(void)
{
	R_I2C_MISC &= ~MASK_I2C_MISC_GPIOEN;
}

/**
 * @brief   I2C bus write function
 * @param   handle: hanle for I2C clock and slave address
 * @param   data: data want to write
 * @param   len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_write(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len)
{
	return _i2c_bus_xfer(handle, data, len, (INT8U)I2C_BUS_WRITE);
}

/**
 * @brief   I2C bus read function
 * @param   handle: hanle for I2C clock and slave address
 * @param   data: the buffer where I2C write the read data into
 * @param   len: data length
 * @return 	0(successful) or -1(failed)
 */
INT32S drv_l1_i2c_bus_read(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len)
{
	return _i2c_bus_xfer(handle, data, len, (INT8U)I2C_BUS_READ);
}

/**
 * @brief   I2C bus write 1byte then write 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: the second data wnat to write
 * @return 	0(successful) or -1(failed)
 */

INT32U drv_l1_reg_1byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U value)
{
	INT8U data[2]={0};
	
	data[0] = reg & 0xFF;
	data[1] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%02X\r\n", __func__, reg, value);
	
	return drv_l1_i2c_bus_write(handle, data, 2);
}

/**
 * @brief   I2C bus write 1byte then read 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the data want to write
 * @param   value: the buffer where I2C write the read data into
 * @return 	0(successful) or -1(failed)
 */
INT32U drv_l1_reg_1byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U *value)
{
	INT32U ret=0;
	INT8U addr[1]={0}, data[1]={0};
	
	addr[0] = reg & 0xFF;
	
	ret = drv_l1_i2c_bus_write(handle, addr, 1);
	ret = drv_l1_i2c_bus_read(handle, data, 1);
	*value = data[0];
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%02X\r\n", __func__, reg, data[0]);
	
	return ret;
}

/**
 * @brief   I2C bus write 1byte then write 2 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: contains 2 bytes data want to write
 * @return 	0(successful) or -1(failed)
 */
INT32U drv_l1_reg_1byte_data_2byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U value)
{
	INT8U data[3]={0};
	
	data[0] = reg & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%04X\r\n", __func__, reg, value);
	
	return drv_l1_i2c_bus_write(handle, data, 3);
}

/**
 * @brief   I2C bus write 1byte then read 2 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: the first data want to write
 * @param   value: the buffer where I2C write the read 2 bytes data into
 * @return 	0(successful) or -1(failed)
 */
INT32U drv_l1_reg_1byte_data_2byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U *value)
{
	INT32U ret=0;
	INT8U addr[1]={0}, data[2]={0};
	
	addr[0] = reg & 0xFF;
	
	ret = drv_l1_i2c_bus_write(handle, addr, 1);
	ret = drv_l1_i2c_bus_read(handle, data, 2);
	*value = (((INT16U)data[0]) << 8) | (data[1]);
	DRV_I2C_DBG("[%s]-- addr=0x%02X, data=0x%04X\r\n", __func__, reg, *value);
	
	return ret;
}

/**
 * @brief   I2C bus write 2byte then write 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: contains 2 bytes data want to write
 * @param   value: the third byte want to write
 * @return 	0(successful) or -1(failed)
 */
INT32U drv_l1_reg_2byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT16U reg, INT8U value)
{
	INT8U data[3]={0};
	
	data[0] = (reg>>8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = value & 0xFF;
	DRV_I2C_DBG("[%s]-- addr=0x%04X, data=0x%02X\r\n", __func__, reg, value);
	
	return drv_l1_i2c_bus_write(handle, data, 3);
}

/**
 * @brief   I2C bus write 2byte then read 1 bytes
 * @param   handle: hanle for I2C clock and slave address
 * @param   reg: contains 2 bytes data want to write
 * @param   value: the buffer where I2C write the read data into
 * @return 	0(successful) or -1(failed)
 */
INT32U drv_l1_reg_2byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT16U reg, INT8U *value)
{
	INT32U ret=0;
	INT8U addr[2]={0}, data[1]={0};
	
	addr[0] = (reg>>8) & 0xFF;
	addr[1] = reg & 0xFF;
	
	ret = drv_l1_i2c_bus_write(handle, addr, 2);
	ret = drv_l1_i2c_bus_read(handle, data, 1);
	*value = data[0];
	DRV_I2C_DBG("[%s]-- addr=0x%04X, data=0x%02X\r\n", __func__, reg, *value);
	
	return ret;
}

//#endif //(defined _DRV_L1_I2C) && (_DRV_L1_I2C == 1)
