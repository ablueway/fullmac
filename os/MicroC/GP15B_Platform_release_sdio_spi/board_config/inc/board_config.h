#ifndef __BOARD_CONFIG__
#define __BOARD_CONFIG__

#include "project.h"

/************************************************************/
/*						define								*/
/************************************************************/
/* analog power define */
#define VDAC_V30	0x0
#define VDAC_V31	0x1
#define VDAC_V32	0x2
#define VDAC_V33	0x3

/* GPIO PAD DRIVING */
#define IO_DRIVING_4mA				0
#define IO_DRIVING_8mA				1
#define IO_DRIVING_12mA				2
#define IO_DRIVING_16mA				3

/************************************************************/
/*						Pinmux	setting						*/
/************************************************************/
/* UART0 PINMUX */
#define UART0_TX_RX__IOB5_IOB4		0x0
#define UART0_TX_RX__IOC12_IOC13	0x1
#define UART0_TX_RX__IOD5_IOD4		0x2
#if(BOARD_TYPE == BOARD_DVP_GPB51PG)
#define UART0_POS					UART0_TX_RX__IOB5_IOB4
#elif (BOARD_TYPE == BOARD_EMU_GPB51PG)
#define UART0_POS					UART0_TX_RX__IOC12_IOC13
#endif

#define UART0_DRIVING				IO_DRIVING_4mA

/* UART1 PINMUX */
#define UART1_TX_RX__IOB7_IOB6		0x0
#define UART1_TX_RX__IOC15_IOC14	0x1
#define UART1_TX_RX__IOD9_IOD8		0x2
#define UART1_TX_RX__IOD15_IOD14	0x3
#define UART1_POS					UART1_TX_RX__IOB7_IOB6
#define UART1_DRIVING				IO_DRIVING_4mA

/* EXT IRQ PINMUX */
#define EXT_IRQ_A_B_C__IOB8_IOB9_IOB10			0x0
#define EXT_IRQ_A_B_C__IOC13_IOC14_IOC15		0x1
#define EXT_IRQ_A_B_C__IOD7_IOD8_IOD9			0x2
#define EXT_IRQ_A_B_C__IOD12_IOD14_IOD15		0x3
#define EXT_IRQ_POS								EXT_IRQ_A_B_C__IOB8_IOB9_IOB10
#define EXT_IRQ_DRIVING							IO_DRIVING_4mA

/* Timer A/B/C CCP PINMUX */
#define TIMER_A_B_C_CCP__IOB8_IOB9_IOB10		0x0
#define TIMER_A_B_C_CCP__IOC13_IOC14_IOC15		0x1
#define TIMER_A_B_C_CCP__IOD7_IOD8_IOD9			0x2
#define TIMER_A_B_C_CCP__IOD12_IOD14_IOD15		0x3
#define TIMER_ABC_CCP_POS						TIMER_A_B_C_CCP__IOB8_IOB9_IOB10
#define TIMER_ABC_CCP_DRIVING					IO_DRIVING_4mA	

/* Timer D CCP PINMUX */
#define TIMER_D_CCP__IOD6				0x0
#define TIMER_D_CCP__IOC12				0x1
#define TIMER_D_CCP_POS					TIMER_D_CCP__IOD6
#define TIMER_D_CCP_DRIVING				IO_DRIVING_4mA	

/* I2C PINMUX */
#define I2C_SCL_SDA__IOB4_IOB5			0x0
#define I2C_SCL_SDA__IOC14_IOC15		0x1
#define I2C_SCL_SDA__IOD4_IOD5			0x2
#define I2C_SCL_SDA__IOD8_IOD9			0x3
#define I2C_SCL_SDA__IOD14_IOD15		0x4
#define I2C_POS							I2C_SCL_SDA__IOB4_IOB5
#define I2C_DRIVING						IO_DRIVING_4mA

/* SPI PINMUX */
#define SPI_CS_CLK_TX_RX__IOD6_IOD7_IOD8_IOD9		0x0
#define SPI_CS_CLK_TX_RX__IOC8_IOC9_IOC10_IOC11		0x1
#define SPI_CS_CLK_TX_RX__IOD10_IOD11_IOD12_IOD13	0x2
#define SPI_POS										SPI_CS_CLK_TX_RX__IOD6_IOD7_IOD8_IOD9
#define SPI_DRIVING									IO_DRIVING_4mA

/* SPIFC PINMUX */
#define SPIFC_CS_CLK_RX0_3__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5		0x0
#define SPIFC_CS_CLK_RX0_3__IOD7_IOA14_IOA12_IOD8_IOD9_IOB3		0x1
#define SPIFC_CS_CLK_RX0_3__NONE								0xFF
#define SPIFC_POS												SPIFC_CS_CLK_RX0_3__IOD0_IOD1_IOD2_IOD3_IOD4_IOD5
#define SPIFC_CLK_DRIVING										IO_DRIVING_4mA
#define SPIFC_DRIVING											IO_DRIVING_4mA

/* SDC0 PINMUX */
#define SDC0_CMD_CLK_DATA0_3__IOA2_IOA3_IOA5_IOA6_IOA7_IOA4			0x0
#define SDC0_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10	0x1
#define SDC0_CMD_CLK_DATA0_3__IOC6_IOC7_IOC9_IOC10_IOC11_IOC8		0x2
#if(BOARD_TYPE == BOARD_DVP_GPB51PG)
#define SDC0_POS													SDC0_CMD_CLK_DATA0_3__IOC6_IOC7_IOC9_IOC10_IOC11_IOC8
#elif(BOARD_TYPE == BOARD_EMU_GPB51PG)													
#define SDC0_POS													SDC0_CMD_CLK_DATA0_3__IOB14_IOB15_IOB11_IOB12_IOB13_IOB10
#endif													
#define SDC0_CMD_CLK_DRIVING										IO_DRIVING_4mA

/* SDC1 PINMUX */
#define SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12	0x0
#define SDC1_CMD_CLK_DATA0_3__IODB0_IOB1_IOB3_IOB4_IOB5_IOB2		0x1
#define SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13	0x2
#define SDC1_CMD_CLK_DATA0_3__NONE									0xFF
#if(BOARD_TYPE == BOARD_DVP_GPB51PG)
#define SDC1_POS													SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOD14_IOD15_IOD12
#elif(BOARD_TYPE == BOARD_EMU_GPB51PG)	
#define SDC1_POS													SDC1_CMD_CLK_DATA0_3__IOD10_IOD11_IOD13_IOC14_IOC15_IOC13
#endif
#define SDC1_CMD_CLK_DRIVING										IO_DRIVING_4mA

/* I2S PINMUX */
#define I2S_MCLK_LR_BCK_TX__IOB4_IOB5_IOB6_IOB7			0x0
#define I2S_MCLK_LR_BCK_TX__IOB12_IOB13_IOB14_IOB15		0x1
#define I2S_MCLK_LR_BCK_TX__IOC12_IOC13_IOC14_IOC15		0x2
#define I2S_MCLK_LR_BCK_TX__IOD10_IOD11_IOD12_IOD13		0x3
#define I2S_NONE										0xFF
#if(BOARD_TYPE == BOARD_DVP_GPB51PG)
#define I2S_POS											I2S_NONE
#elif(BOARD_TYPE == BOARD_EMU_GPB51PG)
#define I2S_POS											I2S_MCLK_LR_BCK_TX__IOB12_IOB13_IOB14_IOB15
#endif
#define I2S_POS_DRIVING									IO_DRIVING_4mA

/* NAND DATA PINMUX */
#define NAND_DATA_0_7__IOB8_IOB15	0x0
#define NANN_DATA_0_7__IOC4_IOC11	0x1
#define NAND_DATA_POS				NAND_DATA_0_7__IOB8_IOB15
#define NAND_DATA_DRIVING			IO_DRIVING_4mA
#define NAND_CTRL_DRIVING			IO_DRIVING_4mA
#ifndef _NFIO_CFG
#define _NFIO_CFG
typedef enum {
    NAND_IO_IOB       =0x00,  
    NAND_IO_IOC		  =0x01,  
    NAND_IO_AUTO_SCAN =0xFF
} NFIO_CFG;
#endif
extern void nand_iopad_sel(NFIO_CFG nand_io_sel);


/* TFT PIN MUX	*/
#define TFT_DATA_0_7__IOA0_IOA7		0x0
#define TFT_DATA_0_7__IOE0_IOE7		0x1
#define TFT_DATA_0_7__NONE			0xFF

#define	TFT_DATA_8_15__IOA8_IOA15	0x0
#define TFT_DATA_8_15__IOE0_E7		0x1
#define TFT_DATA_8_15__NONE			0xFF

#define TFT_DATA_0_7_POS			TFT_DATA_0_7__IOA0_IOA7
#define TFT_DATA_8_15_POS			TFT_DATA_8_15__NONE
#define TFT_DATA_DIRVING			IO_DRIVING_4mA
#define TFT_CLK_DRIVING				IO_DRIVING_4mA

#define TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3	0x0
#define TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOC8_IOC9_IOC10_IOC11	0x1
#define TFT_CTRL_POS										TFT_CTRL_DE_HSYNC_VSYNC_CLK__IOB0_IOB1_IOB2_IOB3
#define TFT_CLK_DRIVING										IO_DRIVING_4mA

/*	CSI PIN	MUX	*/
#define CSI_DATA_2_9__IOC0_IOC7		0x0
#define CSI_DATA_2_9__IOB8_IOB15	0x1
#define CSI_DATA_2_9__IOE0_IOE7		0x2
#define CSI_DATA_2_9_POS			CSI_DATA_2_9__IOC0_IOC7
#define CSI_DATA_2_9_DRIVING		IO_DRIVING_4mA

#define CSI_DATA_0_1__IOB6_IOB7		0x0
#define CSI_DATA_0_1__IOB4_IOB5		0x1
#define CSI_DATA_0_1__IOC14_IOC15	0x2
#define CSI_DATA_0_1_POS			CSI_DATA_0_1__IOB6_IOB7
#define CSI_DATA_0_1_DRIVING		IO_DRIVING_4mA

#define CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOC8_IOC9_IOC10_IOC11	0x0
#define CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOD6_IOD7_IOD8_IOD9		0x1
#define CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOD6_IOD12_IOD8_IOD9	0x2
#define CSI_CTRL_POS											CSI_CTRL_CLKI_CLKO_HSYNC_VSYNC__IOC8_IOC9_IOC10_IOC11
#define CSI_CLKO_DRIVING										IO_DRIVING_4mA		

/************************************************************/
/*					function 								*/
/************************************************************/
extern void board_init(void);

#endif
