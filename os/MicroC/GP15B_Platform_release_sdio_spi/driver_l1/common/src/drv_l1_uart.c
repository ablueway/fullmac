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
#include "drv_l1_uart.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_isys.h"
#if _OPERATING_SYSTEM != _OS_NONE
#include "os.h"
#endif

#if ((defined _DRV_L1_UART0) && (_DRV_L1_UART0 == 1)) || ((defined _DRV_L1_UART1) && (_DRV_L1_UART1 == 1))
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// UART RX error status register
#define	C_UART_ERR_FRAME			0x00000001
#define	C_UART_ERR_PARITY			0x00000002
#define	C_UART_ERR_BREAK			0x00000004
#define	C_UART_ERR_OVERRUN			0x00000008

// UART control register
#define	C_UART_CTRL_RX_INT			0x00008000
#define	C_UART_CTRL_TX_INT			0x00004000
#define	C_UART_CTRL_RXTO_INT		0x00002000
#define	C_UART_CTRL_UART_ENABLE		0x00001000
#define	C_UART_CTRL_MODEM_INT		0x00000800
#define	C_UART_CTRL_LOOPBACK		0x00000400
#define	C_UART_CTRL_WORD_8BIT		0x00000060
#define	C_UART_CTRL_WORD_7BIT		0x00000040
#define	C_UART_CTRL_WORD_6BIT		0x00000020
#define	C_UART_CTRL_WORD_5BIT		0x00000000
#define	C_UART_CTRL_FIFO_ENABLE		0x00000010
#define	C_UART_CTRL_2STOP_BIT		0x00000008
#define	C_UART_CTRL_EVEN_PARITY		0x00000004
#define	C_UART_CTRL_PARITY_EN		0x00000002
#define	C_UART_CTRL_SEND_BREAK		0x00000001

// UART interrupt pending bit and status register
#define	C_UART_STATUS_RX_INT		0x00008000
#define	C_UART_STATUS_TX_INT		0x00004000
#define	C_UART_STATUS_RXTO_INT		0x00002000
#define	C_UART_STATUS_TX_EMPTY		0x00000080
#define	C_UART_STATUS_RX_FULL		0x00000040
#define	C_UART_STATUS_TX_FULL		0x00000020
#define	C_UART_STATUS_RX_EMPTY		0x00000010
#define	C_UART_STATUS_BUSY			0x00000008

// UART FIFO register
#define	C_UART_FIFO_TX_LEVEL_0		0x00000000
#define	C_UART_FIFO_TX_LEVEL_1		0x00001000
#define	C_UART_FIFO_TX_LEVEL_2		0x00002000
#define	C_UART_FIFO_TX_LEVEL_3		0x00003000
#define	C_UART_FIFO_TX_LEVEL_4		0x00004000
#define	C_UART_FIFO_TX_LEVEL_5		0x00005000
#define	C_UART_FIFO_TX_LEVEL_6		0x00006000
#define	C_UART_FIFO_TX_LEVEL_7		0x00007000
#define	C_UART_FIFO_RX_LEVEL_0		0x00000000
#define	C_UART_FIFO_RX_LEVEL_1		0x00000010
#define	C_UART_FIFO_RX_LEVEL_2		0x00000020
#define	C_UART_FIFO_RX_LEVEL_3		0x00000030
#define	C_UART_FIFO_RX_LEVEL_4		0x00000040
#define	C_UART_FIFO_RX_LEVEL_5		0x00000050
#define	C_UART_FIFO_RX_LEVEL_6		0x00000060
#define	C_UART_FIFO_RX_LEVEL_7		0x00000070

#define UART_RX			0
#define UART_TX			1
#define UART_DISABLE	0
#define UART_ENABLE		1

#define UART_FIFO_SIZE  32

#define UART0_DEV		0
#define UART1_DEV		1
#define UART_DEV_NO		2

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct fifo {
	INT8U *p_start;
	INT8U *p_end;
	INT8U *p_write;
	INT8U *p_read;
} UART_FIFO;

typedef struct uartReg_s 
{
	volatile INT32U uart_data;			//0xC0060000
	volatile INT32U uart_rx_status;		//0xC0060004
	volatile INT32U uart_ctrl;			//0xC0060008
	volatile INT32U uart_baud_rate;		//0xC006000C
	volatile INT32U uart_irq_status;	//0xC0060010
	volatile INT32U uart_fifo;			//0xC0060014
	volatile INT32U uart_tx_delay;		//0xC0060018
	volatile INT32U irda_buad_rate;
	volatile INT32U irda_ctrl;
	volatile INT32U irda_low_power;
} uartReg_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
INT32S drv_l1_uart_sw_fifo_init(INT8U dev_idx);
INT32S drv_l1_uart_sw_fifo_get(INT8U dev_idx, INT8U *data);
INT32S drv_l1_uart_sw_fifo_put(INT8U dev_idx, INT8U data);
void drv_l1_uart_set_ctrl(INT8U dev_idx, INT32U val);
INT32U drv_l1_uart_get_ctrl(INT8U dev_idx);
void drv_l1_uart_set_to_int(INT8U dev_idx, INT8U status);
void drv_l1_uart_buad_rate_set(INT8U dev_idx, INT32U bps);
void drv_l1_uart_fifo_ctrl(INT8U dev_idx, INT8U tx_level, INT8U rx_level);
void drv_l1_uart_init(INT8U dev_idx);
void drv_l1_uart_uninit(INT8U dev_idx);
void drv_l1_uart_rx_tx_en(INT8U dev_idx, INT32U dir, INT32U enable);
void drv_l1_uart_fifo_en(INT8U dev_idx, INT32U enable);
void drv_l1_uart_data_send(INT8U dev_idx, INT8U data, INT8U wait);
INT32S drv_l1_uart_data_get(INT8U dev_idx, INT8U *data, INT8U wait);
INT32S drv_l1_uart_word_len_set(INT8U dev_idx, INT8U word_len);
INT32S drv_l1_uart_stop_bit_size_set(INT8U dev_idx, INT8U stop_size);
INT32S drv_l1_uart_parity_chk_set(INT8U dev_idx, INT8U status, INT8U parity);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
UART_FIFO uart_sw_fifo_head[UART_DEV_NO];
INT8U uart_sw_fifo_buf[UART_DEV_NO][UART_FIFO_SIZE];
INT8U uart_tx_on[UART_DEV_NO];

static uartReg_t* uart_select(INT8U dev_idx)
{
	if(dev_idx == UART0_DEV) {
		return (uartReg_t *)P_UART0_BASE;
	}
	
	if(dev_idx == UART1_DEV) {
		return (uartReg_t *)P_UART1_BASE;
	}
	
	return 0;
}

/**
 * @brief   uart software fifo init
 * @param   dev_idx: uart device
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_init(INT8U dev_idx)
{
	INT32U i;
	UART_FIFO *pFifoHead;
	
	for(i=0; i<UART_FIFO_SIZE; i++) {
		uart_sw_fifo_buf[dev_idx][i] = 0;	
	}
	
	pFifoHead = &uart_sw_fifo_head[dev_idx];	
	pFifoHead->p_start = &uart_sw_fifo_buf[dev_idx][0];
	pFifoHead->p_end = &uart_sw_fifo_buf[dev_idx][UART_FIFO_SIZE-1];
	pFifoHead->p_write = &uart_sw_fifo_buf[dev_idx][0];
	pFifoHead->p_read = &uart_sw_fifo_buf[dev_idx][0];
	return 0;
}

/**
 * @brief   get data from software fifo 
 * @param   dev_idx: uart device
 * @param   data: return data value
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_get(INT8U dev_idx, INT8U *data)
{
	INT8U *read_ptr;
	INT32S ret;
	UART_FIFO *pFifoHead;
	
	if(data == 0) {
		return -1;
	}

#if _OPERATING_SYSTEM != _OS_NONE				
	OSSchedLock();
#endif
	
	ret = 0;
	pFifoHead = &uart_sw_fifo_head[dev_idx];
	
	// Check FIFO is empty	
	if (pFifoHead->p_write == pFifoHead->p_read) {
	 	ret = -2;
		goto __exit;
	}
	
	// Only read p_read once
	read_ptr = pFifoHead->p_read;
	*data = *read_ptr++;
	if (read_ptr > pFifoHead->p_end) {
		read_ptr = pFifoHead->p_start;
	}
	
	pFifoHead->p_read = read_ptr;
__exit:
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif
	return ret;
}

/**
 * @brief   put data to software fifo 
 * @param   dev_idx: uart device
 * @param   data: data value 
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_uart_sw_fifo_put(INT8U dev_idx, INT8U data)
{
	INT8U *read_ptr, *write_ptr;
	UART_FIFO *pFifoHead;

	pFifoHead = &uart_sw_fifo_head[dev_idx];
	
	read_ptr = pFifoHead->p_read - 1;
	if (read_ptr < pFifoHead->p_start) {
		read_ptr = pFifoHead->p_end;
	}
	
	// check FIFO is full
	write_ptr = pFifoHead->p_write;
	if (write_ptr == read_ptr) {			
		return -2;
	}
	
	// only write p_write once
	*write_ptr++ = data;
	if (write_ptr > pFifoHead->p_end) {
		write_ptr = pFifoHead->p_start;
	}
	
	pFifoHead->p_write = write_ptr;
	return 0;
}

/**
 * @brief   uart interrupt interrupt service
 * @param   none
 * @return 	none
 */
static void uart_isr(void)
{
	INT32S i;
	INT32U status;
	INT32U enable;
	uartReg_t *pUart;
	
	for(i=0; i<UART_DEV_NO; i++) {
		pUart = uart_select(i); 
		status = pUart->uart_irq_status & (C_UART_STATUS_RX_INT | C_UART_CTRL_RXTO_INT);
		enable = pUart->uart_ctrl & C_UART_CTRL_RX_INT;
		if(enable && status) {
			while ((pUart->uart_irq_status & C_UART_STATUS_RX_EMPTY) == 0) {
				drv_l1_uart_sw_fifo_put(i, (INT8U)pUart->uart_data);
			}
		}
	}
}

/**
 * @brief   set uart control register
 * @param   dev_idx: uart device
 * @param   val: register value 
 * @return 	none
 */
void drv_l1_uart_set_ctrl(INT8U dev_idx, INT32U val)
{
	// Bit15: Receive interrupt enable
	// Bit14: Transmit interrupt enable
	// Bit13: Receive timeout interrupt enable
	// Bit12: UART enable
	// Bit11: Modem status interrupt enable
	// Bit10: Self loop test enable(used in IrDA mode only)
	// Bit9-7: Reservede
	// Bit6-5: Word length, 00=5 bits, 01=6 bits, 10=7 bits, 11=8 bits
	// Bit4: FIFO enable
	// Bit3: Stop bit size, 0=1 bit, 1=2 bits
	// Bit2: Parity, 0=odd parity, 1=even parity
	// Bit1: Parity enable, 0=disable, 1=enable
	// Bit0: Send break, 0=normal, 1=send break signal
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}
	
	pUart->uart_ctrl = val & 0xFC7F;
}

/**
 * @brief   get uart control register value 
 * @param   dev_idx: uart device
 * @return 	register value
 */
INT32U drv_l1_uart_get_ctrl(INT8U dev_idx)
{
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return 0;
	}
	
	return pUart->uart_ctrl;
}

/**
 * @brief   uart rx timeout irq enable 
 * @param   dev_idx: uart device
 * @param   status: enable bit 
 * @return 	none
 */
void drv_l1_uart_set_to_int(INT8U dev_idx, INT8U status)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}
	
	reg = pUart->uart_ctrl;
	if (status == UART_ENABLE) {
		reg |= C_UART_CTRL_RXTO_INT;
	} else {
		reg &= ~C_UART_CTRL_RXTO_INT;
	}

	pUart->uart_ctrl = reg;
}

/**
 * @brief   set uart buat rate 
 * @param   dev_idx: uart device
 * @param   bps: buad rate speed 
 * @return 	none
 */
void drv_l1_uart_buad_rate_set(INT8U dev_idx, INT32U bps)
{
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}
	
	pUart->uart_baud_rate = sys_clk_get() / bps;
}

/**
 * @brief   set uart fifo threshold
 * @param   dev_idx: uart device
 * @param   tx_level: tx fifo threshold 
 * @param   rx_level: rx fifo threshold 
 * @return 	none
 */
void drv_l1_uart_fifo_ctrl(INT8U dev_idx, INT8U tx_level, INT8U rx_level)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}
	
	reg = ((INT32U) tx_level & 0x7) << 12;
	reg |= ((INT32U) rx_level & 0x7) << 4;
	pUart->uart_fifo = reg;
}

/**
 * @brief   set uart initialize.
 * @param   dev_idx: uart device
 * @return 	none
 */
void drv_l1_uart_init(INT8U dev_idx)
{
	if(dev_idx >= UART_DEV_NO) {
		return;
	}
	
	// disable uart
	drv_l1_uart_set_ctrl(dev_idx, 0);
	
	uart_tx_on[dev_idx] = 0;
	drv_l1_uart_sw_fifo_init(dev_idx);
	
	// 8 bits, 1 stop bit, no parity check, RX disabled
	drv_l1_uart_set_ctrl(dev_idx, C_UART_CTRL_UART_ENABLE | C_UART_CTRL_WORD_8BIT | C_UART_CTRL_RX_INT);
		
	vic_irq_register(VIC_UART_01, uart_isr);	
	vic_irq_enable(VIC_UART_01);
	
	// enable rx timeout interrupt
    drv_l1_uart_set_to_int(dev_idx, UART_ENABLE);
}

/**
 * @brief   set uart uninitialize.
 * @param   dev_idx: uart device
 * @return 	none
 */
void drv_l1_uart_uninit(INT8U dev_idx)
{
	drv_l1_uart_set_ctrl(dev_idx, 0);
	//vic_irq_unregister(VIC_UART_01);	
	//vic_irq_disable(VIC_UART_01);
}

/**
 * @brief   set uart direction enable
 * @param   dev_idx: uart device
 * @param   dir: direction, UART_RX or UART_TX
 * @param   enable: enable bit
 * @return 	none
 */
void drv_l1_uart_rx_tx_en(INT8U dev_idx, INT32U dir, INT32U enable)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}

#if _OPERATING_SYSTEM != _OS_NONE	
	OSSchedLock();
#endif
	
	if (dir == UART_RX) {					// RX
		reg = pUart->uart_ctrl;
		if (enable == UART_ENABLE) {
			reg |= C_UART_CTRL_RX_INT;
		} else {
			reg &= ~C_UART_CTRL_RX_INT;
		} 
		
		pUart->uart_ctrl = reg;
	} else {								// TX
		if (enable == UART_ENABLE) {
			uart_tx_on[dev_idx] = 1;
		} else {
			uart_tx_on[dev_idx] = 0;
		}
	}
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif
}

/**
 * @brief   set uart fifo enable
 * @param   dev_idx: uart device
 * @param   enable: enable bit
 * @return 	none
 */
void drv_l1_uart_fifo_en(INT8U dev_idx, INT32U enable)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedLock();
#endif
  	
  	reg = pUart->uart_ctrl;
  	if (enable == UART_ENABLE) {
		reg |= C_UART_CTRL_FIFO_ENABLE;
	} else {
		reg &= ~C_UART_CTRL_FIFO_ENABLE;
	}
	
	pUart->uart_ctrl = reg;
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif
}

/**
 * @brief   send uart data
 * @param   dev_idx: uart device
 * @param   data: data value
 * @param   wait: wait data transfer
 * @return 	none
 */
void drv_l1_uart_data_send(INT8U dev_idx, INT8U data, INT8U wait)
{
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return;
	}

	if (uart_tx_on[dev_idx] == 0) {
		return;
	}
	
	// Wait until FIFO is not full
	if (wait) {
		while(pUart->uart_irq_status & C_UART_STATUS_TX_FULL);	
	}
		
	// Send our data now
	pUart->uart_data = data;
}

/**
 * @brief   get uart data
 * @param   dev_idx: uart device
 * @param   data: get data value
 * @param   wait: wait data get
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_data_get(INT8U dev_idx, INT8U *data, INT8U wait)
{
	if(dev_idx >= UART_DEV_NO) {
		return STATUS_FAIL;
	}

    while (drv_l1_uart_sw_fifo_get(dev_idx, data)) {
    	// Queue is empty and the caller doesn't want to wait
    	if (!wait) {						
    		return STATUS_FAIL;
    	}
    }
    
    return STATUS_OK;
}

/**
 * @brief   set uart word length
 * @param   dev_idx: uart device
 * @param   data: C_UART_CTRL_WORD_5BIT ~ C_UART_CTRL_WORD_8BIT
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_word_len_set(INT8U dev_idx, INT8U word_len)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return STATUS_FAIL;
	}
	
	reg = pUart->uart_ctrl;
	reg &= ~(3<<5);
	switch(word_len) {
		case WORD_LEN_5:
			reg |= C_UART_CTRL_WORD_5BIT;
			break;
		case WORD_LEN_6:
			reg |= C_UART_CTRL_WORD_6BIT;
			break;
		case WORD_LEN_7:
			reg |= C_UART_CTRL_WORD_7BIT;
			break;
		case WORD_LEN_8:
			reg |= C_UART_CTRL_WORD_8BIT;
			break;
		default :
			return STATUS_FAIL;
	}

	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

/**
 * @brief   set uart stop bit
 * @param   dev_idx: uart device
 * @param   stop_size: STOP_SIZE_1 or STOP_SIZE_2
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_stop_bit_size_set(INT8U dev_idx, INT8U stop_size)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return STATUS_FAIL;
	}

	reg = pUart->uart_ctrl;
	reg &= ~(1<<3);
	if (stop_size == STOP_SIZE_2) {
		reg |= C_UART_CTRL_2STOP_BIT;
	}
	
	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

/**
 * @brief   set uart parity
 * @param   dev_idx: uart device
 * @param   status: UART_DISABLE or UART_ENABLE
 * @param   parity: PARITY_EVEN or PARITY_ODD
 * @return 	result; >=0 is success. <0 is fail.
 */
INT32S drv_l1_uart_parity_chk_set(INT8U dev_idx, INT8U status, INT8U parity)
{
	INT32U reg;
	uartReg_t *pUart = uart_select(dev_idx);
	if(pUart == 0) {
		return STATUS_FAIL;
	}
	
	reg = pUart->uart_ctrl;
	if (status == UART_DISABLE) {
		reg &= ~(1<<1);
	} else {
		reg |= C_UART_CTRL_PARITY_EN;
		reg &= ~(1<<2);
		if (parity == PARITY_EVEN) {
			reg |= C_UART_CTRL_EVEN_PARITY;
		}
	}

	pUart->uart_ctrl = reg;
	return STATUS_OK;
}

#if (defined _DRV_L1_UART0) && (_DRV_L1_UART0 == 1)
void drv_l1_uart0_init(void)
{
	drv_l1_uart_init(UART0_DEV);
}

void drv_l1_uart0_buad_rate_set(INT32U bps)
{
	drv_l1_uart_buad_rate_set(UART0_DEV, bps);
}

void drv_l1_uart0_rx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_RX, UART_ENABLE);
}

void drv_l1_uart0_rx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_RX, UART_DISABLE);
}

void drv_l1_uart0_tx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_TX, UART_ENABLE);
}

void drv_l1_uart0_tx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART0_DEV, UART_TX, UART_DISABLE);
}

void drv_l1_uart0_fifo_enable(void)
{
	drv_l1_uart_fifo_en(UART0_DEV, UART_ENABLE);
}

void drv_l1_uart0_fifo_disable(void)
{
	drv_l1_uart_fifo_en(UART0_DEV, UART_DISABLE);
}

void drv_l1_uart0_data_send(INT8U data, INT8U wait)
{
	drv_l1_uart_data_send(UART0_DEV, data, wait);
}

INT32S drv_l1_uart0_data_get(INT8U *data, INT8U wait)
{
	return drv_l1_uart_data_get(UART0_DEV, data, wait);
}

INT32S drv_l1_uart0_word_len_set(INT8U word_len)
{
	return drv_l1_uart_word_len_set(UART0_DEV, word_len);
}

INT32S drv_l1_uart0_stop_bit_size_set(INT8U stop_size)
{
	return drv_l1_uart_stop_bit_size_set(UART0_DEV, stop_size);
}

INT32S drv_l1_uart0_parity_chk_set(INT8U status, INT8U parity)
{
	return drv_l1_uart_parity_chk_set(UART0_DEV, status, parity);
}
#endif

#if (defined _DRV_L1_UART1) && (_DRV_L1_UART1 == 1)
void drv_l1_uart1_init(void)
{
	drv_l1_uart_init(UART1_DEV);
}

void drv_l1_uart1_buad_rate_set(INT32U bps)
{
	drv_l1_uart_buad_rate_set(UART1_DEV, bps);
}

void drv_l1_uart1_rx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_RX, UART_ENABLE);
}

void drv_l1_uart1_rx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_RX, UART_DISABLE);
}

void drv_l1_uart1_tx_enable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_TX, UART_ENABLE);
}

void drv_l1_uart1_tx_disable(void)
{
	drv_l1_uart_rx_tx_en(UART1_DEV, UART_TX, UART_DISABLE);
}

void drv_l1_uart1_fifo_enable(void)
{
	drv_l1_uart_fifo_en(UART1_DEV, UART_ENABLE);
}

void drv_l1_uart1_fifo_disable(void)
{
	drv_l1_uart_fifo_en(UART1_DEV, UART_DISABLE);
}

void drv_l1_uart1_data_send(INT8U data, INT8U wait)
{
	drv_l1_uart_data_send(UART1_DEV, data, wait);
}

INT32S drv_l1_uart1_data_get(INT8U *data, INT8U wait)
{
	return drv_l1_uart_data_get(UART1_DEV, data, wait);
}

INT32S drv_l1_uart1_word_len_set(INT8U word_len)
{
	return drv_l1_uart_word_len_set(UART1_DEV, word_len);
}

INT32S drv_l1_uart1_stop_bit_size_set(INT8U stop_size)
{
	return drv_l1_uart_stop_bit_size_set(UART1_DEV, stop_size);
}

INT32S drv_l1_uart1_parity_chk_set(INT8U status, INT8U parity)
{
	return drv_l1_uart_parity_chk_set(UART1_DEV, status, parity);
}
#endif

#endif
