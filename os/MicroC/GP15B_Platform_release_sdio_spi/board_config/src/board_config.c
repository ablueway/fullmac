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
#include "board_config.h"
#include "driver_l1.h"
#include "drv_l1_gpio.h"
#include "gplib_cfg.h"

static void uart0_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;

	funpos0 &= ~0x3;
#if UART0_POS == UART0_TX_RX__IOB5_IOB4
	funpos0 &= ~(3<<0);	
	gpio_drving_init_io(IO_B5,(IO_DRV_LEVEL)UART0_DRIVING); 
	gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)UART0_DRIVING); 	
#elif UART0_POS == UART0_TX_RX__IOC12_IOC13
	funpos0 |= (1<<0);
	gpio_drving_init_io(IO_C12,(IO_DRV_LEVEL)UART0_DRIVING); 
	gpio_drving_init_io(IO_C13,(IO_DRV_LEVEL)UART0_DRIVING); 
#elif UART0_POS == UART0_TX_RX__IOD5_IOD4
	funpos0 |= (2<<0);
	gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)UART0_DRIVING); 
	gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)UART0_DRIVING); 			
#endif
	R_FUNPOS0 = funpos0;
}

static void uart1_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;

	funpos0 &= ~0xc;
#if UART1_POS == UART1_TX_RX__IOB7_IOB6
	funpos0 &= ~(3<<2);	
	gpio_drving_init_io(IO_B6,(IO_DRV_LEVEL)UART1_DRIVING); 
	gpio_drving_init_io(IO_B7,(IO_DRV_LEVEL)UART1_DRIVING);			
#elif UART1_POS == UART1_TX_RX__IOC15_IOC14
	funpos0 |= (1<<2);
	gpio_drving_init_io(IO_C15,(IO_DRV_LEVEL)UART1_DRIVING); 
	gpio_drving_init_io(IO_C14,(IO_DRV_LEVEL)UART1_DRIVING);
#elif UART1_POS == UART1_TX_RX__IOD9_IOD8
	funpos0 |= (2<<2);
	gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)UART1_DRIVING); 
	gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)UART1_DRIVING);
#elif UART1_POS == UART1_TX_RX__IOD15_IOD14
	funpos0 |= (3<<2);
	gpio_drving_init_io(IO_D14,(IO_DRV_LEVEL)UART1_DRIVING); 
	gpio_drving_init_io(IO_D15,(IO_DRV_LEVEL)UART1_DRIVING);
#endif
	R_FUNPOS0 = funpos0;
}

static void ext_irq_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;

	funpos0 &= ~0x0600;
#if EXT_IRQ_POS == EXT_IRQ_A_B_C__IOB8_IOB9_IOB10
	funpos0 &= ~(3<<9);
#elif EXT_IRQ_POS == EXT_IRQ_A_B_C__IOC13_IOC14_IOC15
	funpos0 |= (1<<9);
#elif EXT_IRQ_POS == EXT_IRQ_A_B_C__IOD7_IOD8_IOD9
	funpos0 |= (2<<9);
#elif EXT_IRQ_POS == EXT_IRQ_A_B_C__IOD12_IOD14_IOD15
	funpos0 |= (3<<9);
#endif
	R_FUNPOS0 = funpos0;		
}

static void i2c_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	INT32U scl_io, sda_io;
	
	funpos0 &= ~(7<<25);
#if I2C_POS == I2C_SCL_SDA__IOB4_IOB5
	funpos0 &= ~(7<<25);
	scl_io = IO_B4;
	sda_io = IO_B5;
#elif I2C_POS == I2C_SCL_SDA__IOC14_IOC15
	funpos0 |= (1<<25);
	scl_io = IO_C14;
	sda_io = IO_C15;
#elif I2C_POS == I2C_SCL_SDA__IOD4_IOD5
	funpos0 |= (2<<25);
	scl_io = IO_D4;
	sda_io = IO_D5;
#elif I2C_POS == I2C_SCL_SDA__IOD8_IOD9
	funpos0 |= (3<<25);
	scl_io = IO_D8;
	sda_io = IO_D9;
#elif I2C_POS == I2C_SCL_SDA__IOD14_IOD15
	funpos0 |= (4<<25);
	scl_io = IO_D14;
	sda_io = IO_D15;
#endif 

	gpio_init_io(scl_io, GPIO_OUTPUT);
	gpio_init_io(sda_io, GPIO_OUTPUT);
	gpio_set_port_attribute(scl_io, ATTRIBUTE_HIGH);
	gpio_set_port_attribute(sda_io, ATTRIBUTE_HIGH);
	gpio_write_io(scl_io, DATA_HIGH);
	gpio_write_io(sda_io, DATA_HIGH);
	gpio_drving_init_io(scl_io,(IO_DRV_LEVEL)I2C_DRIVING); 
	gpio_drving_init_io(sda_io,(IO_DRV_LEVEL)I2C_DRIVING);
	R_FUNPOS0 = funpos0;
}	

static void spi_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	
	funpos0 &= ~(3<<14);
#if SPI_POS == SPI_CS_CLK_TX_RX__IOD6_IOD7_IOD8_IOD9
	funpos0 &= ~(3<<14);
	gpio_drving_init_io(IO_D6,(IO_DRV_LEVEL)SPI_DRIVING); 
	gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)SPI_DRIVING);
	gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)SPI_DRIVING); 
#elif SPI_POS == SPI_CS_CLK_TX_RX__IOC8_IOC9_IOC10_IOC11
	funpos0 |= (1<<14);
	gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)SPI_DRIVING); 
	gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)SPI_DRIVING);
	gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)SPI_DRIVING); 
#elif SPI_POS == SPI_CS_CLK_TX_RX__IOD10_IOD11_IOD12_IOD13
	funpos0 |= (2<<14);
	gpio_drving_init_io(IO_D10,(IO_DRV_LEVEL)SPI_DRIVING); 
	gpio_drving_init_io(IO_D11,(IO_DRV_LEVEL)SPI_DRIVING);
	gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)SPI_DRIVING); 
#endif	
	R_FUNPOS0 = funpos0;
}

static void spifc_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	
#if SPIFC_POS == SPIFC_CS_CLK_RX0_3__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5
	funpos0 &= ~(1<<30);
	gpio_drving_init_io(IO_D0,(IO_DRV_LEVEL)SPIFC_DRIVING); 
	gpio_drving_init_io(IO_D1,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
	gpio_drving_init_io(IO_D2,(IO_DRV_LEVEL)SPIFC_DRIVING);
	gpio_drving_init_io(IO_D3,(IO_DRV_LEVEL)SPIFC_DRIVING);
	gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)SPIFC_DRIVING);
	gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)SPIFC_DRIVING);    
#elif SPIFC_POS == SPIFC_CS_CLK_RX0_3__IOD7_IOA14_IOA12_IOD8_IOD9_IOB3
	funpos0 |= (1<<30);
	gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)SPIFC_DRIVING); 
	gpio_drving_init_io(IO_A14,(IO_DRV_LEVEL)SPIFC_CLK_DRIVING);
	gpio_drving_init_io(IO_A12,(IO_DRV_LEVEL)SPIFC_DRIVING); 
	gpio_drving_init_io(IO_D8,(IO_DRV_LEVEL)SPIFC_DRIVING);
	gpio_drving_init_io(IO_D9,(IO_DRV_LEVEL)SPIFC_DRIVING);
	gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)SPIFC_DRIVING);
#endif
	R_FUNPOS0 = funpos0;
}	

static void sdc0_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	
	funpos0 &= ~(3<<21);
#if SDC0_POS == SDC0_CMD_CLK_DATA0_3__IOA2_IOA3_IOA5_IOA6_IOA7_IOA4
	funpos0 &= ~(3<<21);
       R_IOA_DIR &= ~0x00F4;
	R_IOA_ATT &= ~0x00F4;
	R_IOA_O_DATA |= 0x00F4;
#elif SDC0_POS == SDC0_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10
	funpos0 |= (1<<21);
	R_IOB_DIR &= ~0x7C00;
	R_IOB_ATT &= ~0x7C00;
	R_IOB_O_DATA |= 0x7C00;
#elif SDC0_POS == SDC0_CMD_CLK_DATA0_3__IOC6_IOC7_IOC9_IOC10_IOC11_IOC8
	funpos0 |= (2<<21);
	R_IOC_DIR &= ~0x0F40;
	R_IOC_ATT &= ~0x0F40;
	R_IOC_O_DATA |= 0x0F40;	
#endif	
	R_FUNPOS0 = funpos0;
}

static void sdc1_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;

	funpos0 &= ~(3<<23);
#if SDC1_POS == SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12
	funpos0 &= ~(3<<23);
	R_IOD_DIR &= ~0xF400;
	R_IOD_ATT &= ~0xF400;
	R_IOD_O_DATA |= 0xF400;
#elif SDC1_POS == SDC1_CMD_CLK_DATA0_3__IODB0_IOB1_IOB3_IOB4_IOB5_IOB2
	funpos0 |= (1<<23);
	R_IOB_DIR &= ~0x003D;
	R_IOB_ATT &= ~0x003D;
	R_IOB_O_DATA |= 0x003D;
#elif SDC1_POS == SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13
	funpos0 |= (2<<23);
	R_IOD_DIR &= ~0x2400;
	R_IOD_ATT &= ~0x2400;
	R_IOD_O_DATA |= 0x2C00;
	R_IOC_DIR &= ~0xE000;
	R_IOC_ATT &= ~0xE000;
	R_IOC_O_DATA |= 0xE000;
#endif
	R_FUNPOS0 = funpos0;
}

static void i2s_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	
	funpos0 &= ~(3<<28);
#if I2S_POS == I2S_MCLK_LR_BCK_TX__IOB4_IOB5_IOB6_IOB7
	funpos0 &= ~(3<<28);
	gpio_drving_init_io(IO_B4,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_B5,(IO_DRV_LEVEL)I2S_POS_DRIVING);
	gpio_drving_init_io(IO_B6,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_B7,(IO_DRV_LEVEL)I2S_POS_DRIVING);
#elif I2S_POS == I2S_MCLK_LR_BCK_TX__IOB12_IOB13_IOB14_IOB15
	funpos0 |= (1<<28);
	gpio_drving_init_io(IO_B12,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_B13,(IO_DRV_LEVEL)I2S_POS_DRIVING);
	gpio_drving_init_io(IO_B14,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_B15,(IO_DRV_LEVEL)I2S_POS_DRIVING);
#elif I2S_POS == I2S_MCLK_LR_BCK_TX__IOC12_IOC13_IOC14_IOC15
	funpos0 |= (2<<28);
	gpio_drving_init_io(IO_C12,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_C13,(IO_DRV_LEVEL)I2S_POS_DRIVING);
	gpio_drving_init_io(IO_C14,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_C15,(IO_DRV_LEVEL)I2S_POS_DRIVING);
#elif I2S_POS == I2S_MCLK_LR_BCK_TX__IOD10_IOD11_IOD12_IOD13
	funpos0 |= (3<<28);
	gpio_drving_init_io(IO_D10,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_D11,(IO_DRV_LEVEL)I2S_POS_DRIVING);
	gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)I2S_POS_DRIVING); 
	gpio_drving_init_io(IO_D13,(IO_DRV_LEVEL)I2S_POS_DRIVING);
#endif 
	R_FUNPOS0 = funpos0;
}

static void tft_pinmux_set(void)
{
	INT32U funpos0 = R_FUNPOS0;
	
	funpos0 &= ~(7<<11);
#if TFT_DATA_0_7_POS == TFT_DATA_0_7__IOA0_IOA7
	funpos0 &= ~(1<<11);
#elif TFT_DATA_0_7_POS == TFT_DATA_0_7__IOE0_IOE7
	funpos0 |= (1<<11);
#endif

#if TFT_DATA_8_15_POS == TFT_DATA_8_15__IOA8_IOA15
	funpos0 &= ~(1<<12);
#elif TFT_DATA_8_15_POS == TFT_DATA_8_15__IOE0_E7
	funpos0 |= (1<<12);
#endif
	
#if TFT_CTRL_POS == TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
	funpos0 &= ~(1<<13);
	gpio_drving_init_io(IO_B3,(IO_DRV_LEVEL)TFT_CLK_DRIVING); 
#elif TFT_CTRL_POS == TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11
	funpos0 |= (1<<13);
	gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)TFT_CLK_DRIVING); 
#endif
	R_FUNPOS0 = funpos0;
}
 
static void csi_pinmux_set(void)
{
	INT32U funpos1 = R_FUNPOS1;
	
	funpos1 &= ~(0x1F<<0);
#if CSI_CTRL_POS == CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOC8_IOC9_IOC10_IOC11
	funpos1 &= ~(7<<0);
	gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)CSI_CLKO_DRIVING); 
#elif CSI_CTRL_POS == CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOD6_IOD7_IOD8_IOD9  
	funpos1 |= (1<<2) | (1<<0);
	gpio_drving_init_io(IO_D7,(IO_DRV_LEVEL)CSI_CLKO_DRIVING); 
#elif CSI_CTRL_POS == CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOD6_IOD12_IOD8_IOD9 
	funpos1 |= (1<<2) | (2<<0);
	gpio_drving_init_io(IO_D12,(IO_DRV_LEVEL)CSI_CLKO_DRIVING); 
#endif
	
#if CSI_DATA_2_9_POS == CSI_DATA_2_9__IOC0_IOC7
	funpos1 &= ~(3<<3);
#elif CSI_DATA_2_9_POS == CSI_DATA_2_9__IOB8_IOB15
	funpos1 |= (1<<3);
#elif CSI_DATA_2_9_POS == CSI_DATA_2_9__IOE0_IOE7
	funpos1 |= (2<<3);
#endif
	R_FUNPOS1 = funpos1;
}

static void nand_pinmux_set(void)
{
	NFIO_CFG nand_io_sel = NAND_IO_AUTO_SCAN;
	INT32U funpos0 = R_FUNPOS0;
	
#if NAND_DATA_POS == NAND_DATA_0_7__IOB8_IOB15	
	funpos0 &= ~(1<<6);
	nand_io_sel = NAND_IO_IOB;
	gpio_drving_init_io(IO_B8,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA0
	gpio_drving_init_io(IO_B9,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA1
	gpio_drving_init_io(IO_B10,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA2
	gpio_drving_init_io(IO_B11,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA3
	gpio_drving_init_io(IO_B12,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA4
	gpio_drving_init_io(IO_B13,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA5
	gpio_drving_init_io(IO_B14,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA6
	gpio_drving_init_io(IO_B15,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA7
#elif NAND_DATA_POS == NANN_DATA_0_7__IOC4_IOC11
	funpos0 |= (1<<6);
	nand_io_sel = NAND_IO_IOC;
	gpio_drving_init_io(IO_C4,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA0_#1
	gpio_drving_init_io(IO_C5,(IO_DRV_LEVEL)NAND_DATA_DRIVING);	 // NF_DATA1_#1
	gpio_drving_init_io(IO_C6,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA2_#1
	gpio_drving_init_io(IO_C7,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA3_#1
	gpio_drving_init_io(IO_C8,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA4_#1
	gpio_drving_init_io(IO_C9,(IO_DRV_LEVEL)NAND_DATA_DRIVING);  // NF_DATA5_#1
	gpio_drving_init_io(IO_C10,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA6_#1
	gpio_drving_init_io(IO_C11,(IO_DRV_LEVEL)NAND_DATA_DRIVING); // NF_DATA7_#1
#endif
	R_FUNPOS0 = funpos0;
	nand_iopad_sel(nand_io_sel);
	
	gpio_drving_init_io(IO_D0,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_ALE
	gpio_drving_init_io(IO_D1,(IO_DRV_LEVEL)NAND_CTRL_DRIVING);	// NF_CLE
	gpio_drving_init_io(IO_D2,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_REB
	gpio_drving_init_io(IO_D3,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_WEB
	gpio_drving_init_io(IO_D4,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_RDY
	gpio_drving_init_io(IO_D5,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_WP
	gpio_drving_init_io(IO_D6,(IO_DRV_LEVEL)NAND_CTRL_DRIVING); // NF_CS
}	

void board_init(void)
{	
	gpio_init();
	
	tft_pinmux_set();
	
	csi_pinmux_set();

	uart0_pinmux_set();
	
	uart1_pinmux_set();
	
	ext_irq_pinmux_set();

	i2s_pinmux_set();

	i2c_pinmux_set();

	spi_pinmux_set();

	spifc_pinmux_set();

	sdc0_pinmux_set();
	
	sdc1_pinmux_set();
	
	#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
		(defined(NAND2_EN) && (NAND2_EN == 1)) || \
		(defined(NAND3_EN) && (NAND3_EN == 1)) || \
		((defined NAND_APP_EN) && (NAND_APP_EN == 1))
	//nand_pinmux_set();
	#endif
}