#ifndef __drv_l1_SPI_H__
#define __drv_l1_SPI_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

#define SPI_NUM				1

/* SPI error message */
#define SPI_TIMEOUT        (-1)
#define SPI_BUSY           (-2)

typedef enum
{
    SPI_0,
#if SPI_NUM > 1
	SPI_1,
#endif    
    SPI_MAX_NUMS
} SPI_CHANNEL;

typedef enum
{
    SPI_LBM_NORMAL, /* no loop back */
    SPI_LBM_LOOP_BACK
} SPI_LOOP_BACK_MODE;

typedef enum
{
    PHA0_POL0,
    PHA0_POL1,
    PHA1_POL0,
    PHA1_POL1
} SPI_CLK_PHA_POL;

extern INT32S drv_l1_spi_init(INT8U spi_num);
extern INT32S drv_l1_spi_uninit(INT8U spi_num);
extern INT32S drv_l1_spi_disable(INT8U spi_num);
extern INT32S drv_l1_spi_txrx_level_set(INT8U spi_num, INT8U tx_level, INT8U rx_level);
extern INT32S drv_l1_spi_clk_set(INT8U spi_num, INT32U spi_clk);
extern INT32S drv_l1_spi_pha_pol_set(INT8U spi_num, INT8U pha_pol);
extern INT32S drv_l1_spi_lbm_set(INT8U spi_num, INT8S status);
extern INT32S drv_l1_spi_cs_by_gpio_set(INT8U spi_num, INT8U enable, INT8U pin_num, INT8U active);
extern INT32S drv_l1_spi_transceive(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern INT32S drv_l1_spi_transceive_cpu(INT8U spi_num, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
extern INT32S drv_l1_spi_slave_mode_trans(INT8U spi_num, INT8U dma_en, INT8U *tx_data, INT32U tx_len, INT8U *rx_data, INT32U rx_len);
#endif 		// __drv_l1_SPI_H__