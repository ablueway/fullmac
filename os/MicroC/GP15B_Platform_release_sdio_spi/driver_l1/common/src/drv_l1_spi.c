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
#include "drv_l1_spi.h"
#include "drv_l1_dma.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_gpio.h"
#include "gp_stdlib.h"
#if _OPERATING_SYSTEM != _OS_NONE 
#include "os.h"
#endif

#if (defined _DRV_L1_SPI) && (_DRV_L1_SPI == 1)					  
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SPI_CTRL_ENA		(1 << 15)	/* SPI enable */
#define SPI_CTRL_LBM		(1 << 13)	/* loop back mode selection: SPIRX = SPITX */
#define SPI_CTRL_RST		(1 << 11)	/* SPI software reset */
#define SPI_CTRL_SLVE		(1 <<  8)	/* SPI Slave Mode */
#define SPI_CTRL_CPHA		(1 <<  5)	/* SPI Clock Phase */
#define SPI_CTRL_CPOL		(1 <<  4)	/* SPI Clock Polarity */
#define SPI_CTRL_CLK		(7 <<  0)	/* SPI master mode clock selection */

#define SPI_TX_STATUS_IF	(1 << 15)	/* SPI transmit interrupt flag */
#define SPI_RX_STATUS_IF	(1 << 15)	/* SPI receive interrupt flag */
#define SPI_TX_STATUS_IEN	(1 << 14)	/* SPI transmit interrupt enable */
#define SPI_RX_STATUS_IEN	(1 << 14)	/* SPI receive interrupt enable */

#define SPI_TX_STATUS_LEVEL	(0xF << 4)  /* SPI transmit FIFO interrupt level */
#define SPI_RX_STATUS_LEVEL	(0xF << 4)  /* SPI receiver FIFO interrupt level */
#define SPI_RX_STATUS_RXFLAG (7 << 0)

#define SPI_MISC_SMART		(1 << 8)  	/* SPI FIFO smart mode */
#define SPI_MISC_RFF		(1 <<  3)  	/* Rcecive FIFO is not empty */
#define SPI_MISC_RNE		(1 <<  2)  	/* Rcecive FIFO is not empty */
#define SPI_MISC_TNF		(1 <<  1)  	/* Transmit FIFO is not full */

#define SPI_TIMEOUT_VALUE	10000
#define SPI_TRANS_INT		0
#define SPI_QUEUE_MAX		1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define CLEAR(ptr, cblen)		gp_memset((INT8S *)ptr, 0x00, cblen)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef enum
{
    SPI_DISABLED,                 
    SPI_ENABLED                     
} SPI_FLAG;

typedef struct
{
	INT8U 	spi_usage;
	INT8U   cs_by_external;
	INT8U 	cs_gpio_pin;
	INT8U   cs_active;
	
	INT8U   tx_int_level;
	INT8U   rx_int_level;
	INT8S	slave_dma_en;
	INT8S	reserved;
	
	INT8U 	*tx_buf;
	INT8U 	*rx_buf;
	
	INT32U  len;
	INT32U  rcv_len;
	INT32U  tx_count;
	INT32U  rx_count;
#if _OPERATING_SYSTEM != _OS_NONE	
	OS_EVENT *spi_dma_txq;
	OS_EVENT *spi_dma_rxq;
	void 	 *spi_dma_txq_buf[SPI_QUEUE_MAX];
	void 	 *spi_dma_rxq_buf[SPI_QUEUE_MAX];
#endif	
} SPI_MASTER;

typedef struct spiReg_s 
{
	volatile INT32U	CTRL;		//0xC0080000
	volatile INT32U	TX_STATUS;	//0xC0080004
	volatile INT32U	TX_DATA;	//0xC0080008
	volatile INT32U	RX_STATUS;	//0xC008000C
	volatile INT32U	RX_DATA;	//0xC0080010
	volatile INT32U	MISC;		//0xC0080014
} spiReg_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static spiReg_t* spi_select(INT8U spi_num);
static SPI_MASTER* spi_workmem_get(INT8U spi_num);
static INT32S spi_usage_get(SPI_MASTER *pSpi);
static void spi_usage_free(SPI_MASTER *pSpi);
static void spi_reg_clear(spiReg_t *pSpiReg);
static void spi_tx_int_set(spiReg_t *pSpiReg, INT8S tx_int);
static void spi_rx_int_set(spiReg_t *pSpiReg, INT8S tx_int);
static INT32S spi_transceive_dma(INT8U spi_num);
static INT32S spi_transceive_polling(INT8U spi_num);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern INT32U MCLK;
SPI_MASTER spi_master[SPI_MAX_NUMS];


static spiReg_t* spi_select(INT8U spi_num)
{
	if(spi_num == SPI_0) {
		return (spiReg_t*)P_SPI_CTRL;
	}
	
#if SPI_NUM > 1	
	if(spi_num == SPI_1) {
	 	return (spiReg_t*)P_SPI1_CTRL;
	}
#endif
	return 0;
}

static SPI_MASTER* spi_workmem_get(INT8U spi_num)
{
	if(spi_num == SPI_0) {
		return (SPI_MASTER*)&spi_master[SPI_0];
	}
	
#if SPI_NUM > 1	
	if(spi_num == SPI_1) {
		return (SPI_MASTER*)&spi_master[SPI_1];
	}
#endif
	return 0;
}

static INT32S spi_usage_get(SPI_MASTER *pSpi)
{
	INT32S ret;
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedLock();
#endif
	
	if(pSpi->spi_usage == 1) {
		ret = SPI_BUSY;
	} else {
		pSpi->spi_usage = 1;
		ret = STATUS_OK;
	}
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif
	return ret;	
}

static void spi_usage_free(SPI_MASTER *pSpi)
{
#if _OPERATING_SYSTEM != _OS_NONE 
	OSSchedLock();
#endif	

	pSpi->spi_usage = 0;

#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif		
}

static void spi_reg_clear(spiReg_t *pSpiReg)
{
	pSpiReg->CTRL = 0;
	pSpiReg->TX_STATUS = 0;
	pSpiReg->RX_STATUS = 0;
	pSpiReg->MISC = 0;
	
	/* reset the SPI */
	pSpiReg->CTRL |= SPI_CTRL_RST;
	
	/* clear interrupt flag */	
	pSpiReg->TX_STATUS |= SPI_TX_STATUS_IF; 
	pSpiReg->RX_STATUS |= SPI_RX_STATUS_IF;
}

static void spi_tx_int_set(spiReg_t *pSpiReg, INT8S tx_int)
{
	if(tx_int == SPI_ENABLED) {
		pSpiReg->TX_STATUS |= SPI_TX_STATUS_IEN;
	} else {
		pSpiReg->TX_STATUS &= ~SPI_TX_STATUS_IEN;
	}
} 

static void spi_rx_int_set(spiReg_t *pSpiReg, INT8S rx_int)
{
	if(rx_int == SPI_ENABLED) {
		pSpiReg->RX_STATUS |= SPI_RX_STATUS_IEN;
	} else {
		pSpiReg->RX_STATUS &= ~SPI_RX_STATUS_IEN;
	}
} 

static INT32S handle_spi_isr(spiReg_t *pSpiReg, SPI_MASTER *pSpi)
{
	INT8U  dummy;
    INT32U tx_status = 0;
    INT32U rx_status = 0;
    INT32U i = 0;
    
 	tx_status = pSpiReg->TX_STATUS;
    rx_status = pSpiReg->RX_STATUS;
    
	if (rx_status & SPI_RX_STATUS_IF) {
		pSpiReg->RX_STATUS |= SPI_RX_STATUS_IF; /* clear interrupt flag */
		while(pSpiReg->MISC & SPI_MISC_RNE){
			/* receive */
			if (pSpi->rx_count < pSpi->len) {
				if (pSpi->rx_count < pSpi->rcv_len) {
					pSpi->rx_buf[pSpi->rx_count++] = (INT8U)pSpiReg->RX_DATA;
				} else {
					dummy = (INT8U)pSpiReg->RX_DATA;
					if(dummy){};	//remove compiler warning
					pSpi->rx_count++;
				}
			} else {
				dummy = (INT8U)pSpiReg->RX_DATA;
			} 
		}
	}	

    if (tx_status & SPI_TX_STATUS_IF) {	
    	pSpiReg->TX_STATUS |= SPI_TX_STATUS_IF; /* clear interrupt flag */
    	
    	/* start transmition */
    	for (i=0;(i<8-pSpi->tx_int_level)&&(pSpi->len > pSpi->tx_count);i++) {	
			if ((pSpiReg->MISC & SPI_MISC_TNF)) {		
				pSpiReg->TX_DATA = (INT32U)pSpi->tx_buf[pSpi->tx_count++];
			}
		}
		
		if (pSpi->tx_count >= pSpi->len) {
			spi_tx_int_set(pSpiReg, SPI_DISABLED);
		}	
	}
	
	return 0;
}

static void spi0_isr(void)
{
	SPI_MASTER *pSpi = spi_workmem_get(SPI_0);
	spiReg_t *pSpiReg = spi_select(SPI_0);
	
	handle_spi_isr(pSpiReg, pSpi);
}

#if SPI_NUM > 1
static void spi1_isr(void)
{
	SPI_MASTER *pSpi = spi_workmem_get(SPI_1);
	spiReg_t *pSpiReg = spi_select(SPI_1);
	
	handle_spi_isr(pSpiReg, pSpi);
}
#endif

static INT32S spi_transceive_dma(INT8U spi_num)
{
	INT8U err;	
	volatile INT8S done_rx, done_tx;
	INT32S status;
	DMA_STRUCT dma_rx, dma_tx;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}
	
	/* reset spi control */	
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0, 0);
	
	// rx setting
	done_rx = C_DMA_STATUS_WAITING;
	dma_rx.s_addr = (INT32U) &pSpiReg->RX_DATA;
	if(pSpi->rcv_len == 0) {
		dma_rx.t_addr = 0x60000000;	/* reserved memory */
	} else {
		dma_rx.t_addr = (INT32U) pSpi->rx_buf;	
	}

	dma_rx.count = pSpi->len;
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	dma_rx.notify = &done_rx;
	dma_rx.timeout = 255;
	dma_rx.aes = 0;
	dma_rx.trigger = 0;
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSQFlush(pSpi->spi_dma_rxq);
	status = drv_l1_dma_transfer_with_queue(&dma_rx, pSpi->spi_dma_rxq);
#else	
	status = drv_l1_dma_transfer(&dma_rx);
#endif	
	if(status != 0) {
		return status;
	}
	
	// tx setting
	done_tx = C_DMA_STATUS_WAITING;
	dma_tx.s_addr = (INT32U) pSpi->tx_buf;
	dma_tx.t_addr = (INT32U) &pSpiReg->TX_DATA;
	dma_tx.count = pSpi->len;
	dma_tx.width = DMA_DATA_WIDTH_1BYTE;
	dma_tx.notify = &done_tx;
	dma_tx.timeout = 255;
	dma_tx.aes = 0;
	dma_tx.trigger = 0;
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSQFlush(pSpi->spi_dma_txq);
	status = drv_l1_dma_transfer_with_queue(&dma_tx, pSpi->spi_dma_txq);
#else
	status = drv_l1_dma_transfer(&dma_tx);
#endif	
	if (status != 0) {
		return status;
	}

#if _OPERATING_SYSTEM != _OS_NONE
	status = (INT32U) OSQPend(pSpi->spi_dma_rxq, SPI_TIMEOUT_VALUE/10, &err);
	if(status != C_DMA_STATUS_DONE || err != OS_NO_ERR) {
		return STATUS_FAIL;
	}

	status = (INT32U) OSQPend(pSpi->spi_dma_txq, SPI_TIMEOUT_VALUE/10, &err);
	if(status != C_DMA_STATUS_DONE || err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
#else
	while (done_rx == C_DMA_STATUS_WAITING);
	while (done_tx == C_DMA_STATUS_WAITING);
	
	if (done_rx != C_DMA_STATUS_DONE || done_tx != C_DMA_STATUS_DONE) {
		return SPI_TIMEOUT;
	}
#endif
	
	return STATUS_OK;
}

static INT32S spi_slave_transceive_dma(INT8U spi_num)
{
	INT8U err;
	INT8S done_rx;
	INT32S status;
	DMA_STRUCT dma_rx;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}
	
	done_rx = C_DMA_STATUS_WAITING;
	dma_rx.s_addr = (INT32U) &pSpiReg->RX_DATA;
	dma_rx.t_addr = (INT32U) pSpi->rx_buf;
	dma_rx.count = pSpi->rcv_len;
	dma_rx.width = DMA_DATA_WIDTH_1BYTE;
	dma_rx.notify = &done_rx;
	dma_rx.timeout = 0;
	dma_rx.aes = 0;
	dma_rx.trigger = 0;
	
#if _OPERATING_SYSTEM != _OS_NONE
	OSQFlush(pSpi->spi_dma_rxq);
	status = drv_l1_dma_transfer_with_queue(&dma_rx, pSpi->spi_dma_rxq);
#else
	status = drv_l1_dma_transfer(&dma_rx);
#endif	
	if(status != 0) {
		return status;
	}

#if _OPERATING_SYSTEM != _OS_NONE
	status = (INT32U) OSQPend(pSpi->spi_dma_rxq, 0, &err);
	if(status != C_DMA_STATUS_DONE || err != OS_NO_ERR) {
		return STATUS_FAIL;
	}
#else
	while (done_rx == C_DMA_STATUS_WAITING);
	
	if (done_tx != C_DMA_STATUS_DONE) {
		return SPI_TIMEOUT;
	}
#endif
	
	return STATUS_OK;
}

static INT32S spi_transceive_polling(INT8U spi_num)
{
	INT8U dummy;
	INT32U timeout;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	/* reset spi control */	
	pSpiReg->CTRL |= SPI_CTRL_RST;
	spi_tx_int_set(pSpiReg, SPI_DISABLED);
	spi_rx_int_set(pSpiReg, SPI_DISABLED);
	drv_l1_spi_txrx_level_set(spi_num, 0, 0);
	
	/* polling status */
	while (pSpi->len > pSpi->tx_count) { 
		pSpiReg->TX_DATA = (INT32U)pSpi->tx_buf[pSpi->tx_count];
		timeout = 0;
		 while((pSpiReg->RX_STATUS & SPI_RX_STATUS_RXFLAG) == 0) {
			if(++timeout == SPI_TIMEOUT_VALUE) {
	        	return SPI_TIMEOUT;
	    	}	
	    }

		if(pSpi->tx_count < pSpi->rcv_len) { 
			pSpi->rx_buf[pSpi->tx_count] = (INT8U)pSpiReg->RX_DATA;
		} else {
			dummy = (INT8U)pSpiReg->RX_DATA;
			if(dummy){};	//for remove compiler warning
		}
		pSpi->tx_count++;
	}

	return STATUS_OK;
}

/**
 * @brief   Initialize SPI interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_init(INT8U spi_num)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}
	
	/* clear spi */
	drv_l1_spi_uninit(spi_num);
	
	/* create queue */
#if _OPERATING_SYSTEM != _OS_NONE	
	if(pSpi->spi_dma_txq == 0) {
		pSpi->spi_dma_txq = OSQCreate(&pSpi->spi_dma_txq_buf[0], SPI_QUEUE_MAX);
		if(pSpi->spi_dma_txq == 0) {
			return STATUS_FAIL;
		}
	}
	
	if(pSpi->spi_dma_rxq == 0) {
		pSpi->spi_dma_rxq = OSQCreate(&pSpi->spi_dma_rxq_buf[0], SPI_QUEUE_MAX);
		if(pSpi->spi_dma_rxq == 0) {
			INT8U err;
			OSQDel(pSpi->spi_dma_txq, OS_DEL_ALWAYS, &err); 
			return STATUS_FAIL;
		}
	}
#endif	
		
	/* enable the SPI */
	pSpiReg->CTRL |= SPI_CTRL_ENA;
	
	/* enable the SPI smart mode */
	pSpiReg->MISC |= SPI_MISC_SMART;
		
	drv_l1_spi_txrx_level_set(spi_num, 4, 4); /* set interrupt trigger level */
	if(spi_num == SPI_0) {
		vic_irq_register(VIC_SPI, spi0_isr);	/* register SPI isr */
		vic_irq_enable(VIC_SPI);
	}
	 
#if SPI_NUM > 1	
	if(spi_num == SPI_1) {
		vic_irq_register(VIC_SPI1, spi1_isr);	/* register SPI isr */
		vic_irq_enable(VIC_SPI1);
	}
#endif

	return STATUS_OK; 	
}

/**
 * @brief   Un-initialize SPI interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_uninit(INT8U spi_num)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}
	
	/* clear register */
	spi_reg_clear(pSpiReg);
	
#if _OPERATING_SYSTEM != _OS_NONE
	if(pSpi->spi_dma_txq) {
		INT8U err;
		
		OSQFlush(pSpi->spi_dma_txq);
		OSQDel(pSpi->spi_dma_txq, OS_DEL_ALWAYS, &err);
		pSpi->spi_dma_txq = 0;
	}
	
	if(pSpi->spi_dma_rxq) {
		INT8U err;
		
		OSQFlush(pSpi->spi_dma_rxq);
		OSQDel(pSpi->spi_dma_rxq, OS_DEL_ALWAYS, &err);
		pSpi->spi_dma_rxq = 0;
	}
#endif

	/* clear memory */
	CLEAR(pSpi, sizeof(SPI_MASTER));
	
	if(spi_num == SPI_0) {
		vic_irq_unregister(VIC_SPI);
		vic_irq_disable(VIC_SPI);
	}
	
#if SPI_NUM > 1	
	if(spi_num == SPI_1) {
		vic_irq_unregister(VIC_SPI1);
		vic_irq_disable(VIC_SPI1);
	}
#endif	
	return STATUS_OK;	
}

/**
 * @brief   disable spi interface
 * @param   spi_num: spi device number
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_disable(INT8U spi_num)
{
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}
	
	pSpiReg->CTRL &= ~SPI_CTRL_ENA;  
	return STATUS_OK;
}

/**
 * @brief   set spi fifo threshold
 * @param   spi_num: spi device number
 * @param   tx_level: tx fifo threshold, 0 ~ 7
 * @param   rx_level: rx fifo threshold, 0 ~ 7 
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_txrx_level_set(INT8U spi_num, INT8U tx_level, INT8U rx_level)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}

	if(rx_level > 7) {
		rx_level = 7;
	}
	
	if(tx_level > 7) {
		tx_level = 7;
	} 

	pSpiReg->TX_STATUS &= ~SPI_TX_STATUS_LEVEL;
	pSpiReg->TX_STATUS |= tx_level << 4;
	
	pSpiReg->RX_STATUS &= ~SPI_RX_STATUS_LEVEL;
	pSpiReg->RX_STATUS |= rx_level << 4;
	
	pSpi->tx_int_level = tx_level;
	pSpi->rx_int_level = rx_level;
	
	return STATUS_OK;
}

/**
 * @brief   set spi clock speed for master mode.
 * @param   spi_num: spi device number
 * @param   spi_clk: spi clock speed
 * @return 	result; >=0 is success, <0 is fail
 */
INT32S drv_l1_spi_clk_set(INT8U spi_num, INT32U spi_clk)
{
	INT8U div, N;
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}

	div = MCLK / spi_clk;
	for(N=0; N<7; N++) {
		div >>= 1;
		if(div == 1) {
			break;
		}
	}
	
	if(N == 7) {
		N--;
	}
	
	/* set the SPI clock */
	pSpiReg->CTRL &= ~SPI_CTRL_CLK; 
	pSpiReg->CTRL |= N;
	return STATUS_OK;
}

/**
 * @brief   Set the spi clock phase and polarity for master mode.
 * @param   spi_num: spi device number
 * @param   pha_pol: PHA0_POL0/PHA0_POL1/PHA1_POL0/PHA1_POL1
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_pha_pol_set(INT8U spi_num, INT8U pha_pol)
{
	INT32U reg;
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}
	
	// spi disable first
	pSpiReg->CTRL &= ~SPI_CTRL_ENA; 
	
	reg = pSpiReg->CTRL;
	switch(pha_pol) 
	{
		case PHA0_POL0:
			reg &= ~(SPI_CTRL_CPHA | SPI_CTRL_CPOL);
			break;
			
		case PHA0_POL1:
			reg &= ~SPI_CTRL_CPHA;
			reg |= SPI_CTRL_CPOL;
			break; 
			
		case PHA1_POL0:
			reg |= SPI_CTRL_CPHA;
			reg &= ~SPI_CTRL_CPOL;
			break; 
			
		case PHA1_POL1:
			reg |= SPI_CTRL_CPHA;
			reg |= SPI_CTRL_CPOL;
			break; 
			
		default:
			break;
	}
	
	reg |= SPI_CTRL_ENA;
	pSpiReg->CTRL = reg;
	return STATUS_OK;
}

/**
 * @brief   Set the spi loop back mode
 * @param   spi_num: spi device number
 * @param   status: SPI_LBM_NORMAL / SPI_LBM_LOOP_BACK 
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_lbm_set(INT8U spi_num, INT8S status)
{
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if(pSpiReg == 0) {
		return STATUS_FAIL;
	}
	
	if(status == SPI_LBM_LOOP_BACK) {
		pSpiReg->CTRL |= SPI_CTRL_LBM;
	} else {
		pSpiReg->CTRL &= ~SPI_CTRL_LBM; 
	}
	
	return STATUS_OK;
}

/**
 * @brief   Set the spi cs pin by external gpio
 * @param   spi_num: spi device number
 * @param   enable: enable / disable
 * @param   pin_num: gpio pin mumber
 * @param   active: active level   
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_cs_by_gpio_set(INT8U spi_num, INT8U enable, INT8U pin_num, INT8U active)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}
	
	if(enable) {
		pSpi->cs_by_external = TRUE;
		pSpi->cs_gpio_pin = pin_num;
		pSpi->cs_active = active;
		
		gpio_init_io(pin_num, GPIO_OUTPUT);
		gpio_set_port_attribute(pin_num, 1);
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	} else {
		pSpi->cs_by_external = FALSE;
	}
	
	return STATUS_OK;	
}

/**
 * @brief   master mode transmit and receive data by dma 
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size  
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_transceive(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}
	
	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}	
	
	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;

	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
	}

	result = spi_transceive_dma(spi_num);
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	}
	
	spi_usage_free(pSpi);
	return result;
}

/**
 * @brief   master mode transmit and receive data by cpu
 * @param   spi_num: spi device number
 * @param   tx_data: tx data buffer
 * @param   tx_len: tx data size
 * @param   rx_data: rx data buffer
 * @param   rx_len: rx data size  
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_transceive_cpu(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	INT32S result;
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	if(pSpi == 0) {
		return STATUS_FAIL;
	}
	
	result = spi_usage_get(pSpi);
	if (result == SPI_BUSY) {
		return SPI_BUSY;
	}	
	
	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;

	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, pSpi->cs_active);
	}

	result = spi_transceive_polling(spi_num);
	if (pSpi->cs_by_external == TRUE) { /* chip select by internal */
		gpio_write_io(pSpi->cs_gpio_pin, (!pSpi->cs_active)&0x1);
	}
	
	spi_usage_free(pSpi);
	return result;
}

/**
 * @brief   spi slave mode start receive data 
 * @param   spi_num: spi device number
 * @param   dma_en: dma mode use.
 * @param   rx_data: receive data buffer. 
 * @param   rx_len: receive data size.  
 * @return 	result; STATUS_FAIL/STATUS_OK
 */
INT32S drv_l1_spi_slave_mode_trans(INT8U spi_num, INT8U dma_en, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len)
{
	SPI_MASTER *pSpi = spi_workmem_get(spi_num);
	spiReg_t *pSpiReg = spi_select(spi_num); 	
	if((pSpi == 0) || (pSpiReg == 0)) {
		return STATUS_FAIL;
	}
	
	pSpi->tx_buf = tx_data;
	pSpi->rx_buf = rx_data;
	pSpi->len = tx_len;
	pSpi->rcv_len = rx_len;
	pSpi->tx_count = 0;
	pSpi->rx_count = 0;
	pSpi->slave_dma_en = dma_en;
	
	/* slave mode enable */
	pSpiReg->CTRL |= SPI_CTRL_SLVE;
	pSpiReg->CTRL |= SPI_CTRL_RST;
		
	if(dma_en) {
		if((pSpi->tx_buf == 0) || (pSpi->len == 0)) {
			// only slave rx mode
			if(spi_slave_transceive_dma(spi_num) < 0) {
				return STATUS_FAIL;
			}
		} else {
			// slave rx and tx mode
			if(spi_transceive_dma(spi_num) < 0) {
				return STATUS_FAIL;
			}
		}
	} else {
	#if SPI_TRANS_INT == 1
		if((rx_len % 7) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 6);
		} else if((rx_len % 6) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 5);
		} else if((rx_len % 5) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 4);
		} else if((rx_len % 4) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 3);
		} else if((rx_len % 3) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 2);
		} else if((rx_len % 2) == 0) {
			drv_l1_spi_txrx_level_set(spi_num, 0, 1);
		} else {
			drv_l1_spi_txrx_level_set(spi_num, 0, 0);
		}
		
		/* Enable SPI receive interrupt */
		spi_rx_int_set(pSpiReg, SPI_ENABLED); 
	
		/* wait receive done */
		while(pSpi->rx_count < pSpi->rcv_len) {
		#if _OPERATING_SYSTEM != _OS_NONE
			OSTimeDly(1);
		#else
			drv_msec_wait(1);
		#endif
		}
		
		/* Disable SPI receive interrupt */
		spi_rx_int_set(pSpiReg, SPI_DISABLED);
	
	#else
		drv_l1_spi_txrx_level_set(spi_num, 0, 0);
		
		/* polling receice fifo */
		if(pSpi->tx_buf && pSpi->len) {
			pSpiReg->TX_DATA = pSpi->tx_buf[pSpi->tx_count++];
		}
		
		while(pSpi->rx_count < pSpi->rcv_len) {
			if(pSpiReg->RX_STATUS & 0x0F) {
				pSpi->rx_buf[pSpi->rx_count++] = (INT8U)pSpiReg->RX_DATA;
				
				if(pSpi->tx_buf && pSpi->len && (pSpi->tx_count < pSpi->len)) {
					pSpiReg->TX_DATA = (INT8U)pSpi->tx_buf[pSpi->tx_count++]; 
				}
			} else {
			#if _OPERATING_SYSTEM != _OS_NONE
				OSTimeDly(1);
			#else
				drv_msec_wait(1);
			#endif
			}
		}
	#endif
	}
	
	/* slave mode disable */
	pSpiReg->CTRL &= ~SPI_CTRL_SLVE;
	
	return STATUS_OK;
}
#endif //(defined _DRV_L1_SPI) && (_DRV_L1_SPI == 1)
