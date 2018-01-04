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
#ifndef __drv_l1_I2C_H__
#define __drv_l1_I2C_H__

/****************************************************
*		include file								*
****************************************************/
#include "driver_l1_cfg.h"
#include "project.h"

/****************************************************
*	Definition	 									*
****************************************************/
#define I2C_TIME_OUT		10	//ms
#define	NO_ACK_TIMEOUT		10	//ms

#define I2C_BUS_WRITE	0
#define I2C_BUS_READ	1

#define I2C_ICCR_INIT				0x00
#define I2C_IDEBCLK_INIT			0x04
#define I2C_ICCR_TXCLKMSB_MASK		0x0F

#define I2C_ICSR_MASTER_TX			0xC0
#define I2C_ICSR_MASTER_RX			0x80
#define I2C_ICSR_SLAVE_TX			0x40
#define I2C_ICSR_SLAVE_RX			0x00

#define I2C_MISC_PINMUX_EN			0x01
#define I2C_MISC_ACK_DONTCARE		0x02

/****************************************************
*		Register bitwise definition 				*
****************************************************/
/********************* Define R_I2C_ICCR bit mask (0xC00B0000, I2C control register) *****************/
#define MASK_I2C_CR_ACKEN		BIT7		/* I2C-bus acknowledge enable */
#define MASK_I2C_CR_CLKSELPRE	BIT6		/* Source clock of I2C-bus transmit clock prescaler selection bit */
#define MASK_I2C_CR_INTREN		BIT5		/* I2C-bus Tx/Rx interrupt enable */
#define MASK_I2C_CR_INTRPEND	BIT4		/* I2C-bus Tx/Rx interrupt pending flag */

/********************* Define R_I2C_ICSR bit mask (0xC00B0004, I2C control/status register) *****************/
#define MASK_I2C_SR_BUSYSTA		BIT5		/* I2C-bus busy signal status bit, when write 1: START signal generation, 0: STOP signal generation */
#define MASK_I2C_SR_DOEN		BIT4		/* I2C-bus data output enable/disable bit */
#define MASK_I2C_SR_APBIT		BIT3		/* I2C-bus arbitration procedure status flag bit */
#define MASK_I2C_SR_ASBIT		BIT2		/* I2C-bus address as slave status flag bit */
#define MASK_I2C_SR_AZBIT		BIT1		/* I2C-bus address zero status flag bit */
#define MASK_I2C_SR_LRBIT		BIT0		/* I2C-bus last received bit status flag bit. 0: ACK received, 1: ACK not received */

/********************* Define R_I2C_MISC bit mask (0xC00B0018, I2C miscellaneous control register) *****************/
#define MASK_I2C_MISC_IGNOREACK		BIT1		/* I2C-bus ignore ACK control bit */
#define MASK_I2C_MISC_GPIOEN		BIT0		/* I2C-bus output via GPIO enable */


/****************************************************
*		Data structure 								*
*****************************************************/
typedef struct drv_l1_i2c_bus_handle_s
{
	INT16U slaveAddr;			/* slave device address */
	INT16U clkRate;				/* i2c bus clock rate */
}drv_l1_i2c_bus_handle_t;

/****************************************************
*		external function declarations				*
****************************************************/
extern void drv_l1_i2c_init(void);
extern void drv_l1_i2c_uninit(void);
extern INT32S drv_l1_i2c_bus_write(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len);
extern INT32S drv_l1_i2c_bus_read(drv_l1_i2c_bus_handle_t *handle, INT8U* data, INT8U len);
extern INT32U drv_l1_reg_1byte_data_1byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U value);
extern INT32U drv_l1_reg_1byte_data_1byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT8U *value);
extern INT32U drv_l1_reg_1byte_data_2byte_write(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U value);
extern INT32U drv_l1_reg_1byte_data_2byte_read(drv_l1_i2c_bus_handle_t *handle, INT8U reg, INT16U *value);
#endif	/*__drv_l1_I2C_H__*/