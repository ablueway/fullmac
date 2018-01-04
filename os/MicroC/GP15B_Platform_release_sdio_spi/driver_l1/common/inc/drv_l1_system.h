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
#ifndef __drv_l1_SYSTEM_H__
#define __drv_l1_SYSTEM_H__

#include "driver_l1.h"
#include "drv_l1_isys.h"

/********************* Define R_SYSTEM_CTRL bit mask (0xD000000C, System Control Register) *****************/
#define MASK_SYSCTL_CSICLK_DIV					BIT14	/* Sensor Master Clock Division, 0:Sensor Master Clock / 1,1: Sensor Master Clock / 2 */
#define MASK_SYSCTL_SEL30K						BIT12	/* 0: disable, 1: enable */
#define MASK_SYSCTL_SEN_48M_SEL					BIT11	/* Sensor clock source, 0: system clock/2, 1: XCKGEN_48M/2 */
#define MASK_SYSCTL_CSICLK_EN					BIT8	/* CSI Clock Enable */
#define MASK_SYSCTL_HNRC						BIT1	/* 0: normal halt mode, 1: The next instruction is continued after the CPU is waked up from the Halt mode */
#define MASK_SYSCTL_SEL_REAL32K					BIT0	/* 0: from XCKGEN48M, 1: from RTC's 32768 */

/********************* Define R_SYSTEM_MISC_CTRL0 bit mask (0xD0000008, SYSTEM MISC Control Register 0) *****************/
#define MASK_SYSMISC_CTL0_SW_PHY_AFE_RESET		BIT15	/* Software Control USB PHY AFE_RESET CONTROL Singal, 1: reset */
#define MASK_SYSMISC_CTL0_SW_PHY_SIM_MODE		BIT14	/* Software Control USB PHY SIM_MODE CONTROL Singal, 1: go Sim Mode */
#define MASK_SYSMISC_CTL0_SW_PHY_SLUMBER		BIT13	/* Software Control USB PHY SLUMBER CONTROL Singal, 1: go SLUMBER Mode */
#define MASK_SYSMISC_CTL0_SW_PHY_PARTIAL		BIT12	/* Software Control USB PHY PARTIAL CONTROL Singal, 1: Go PARTIAL Mode */
#define MASK_SYSMISC_CTL0_SW_PHY_RESET			BIT11	/* Software Control USB PHY RESET CONTROL Singal, 1: reset */
#define MASK_SYSMISC_CTL0_SW_PHY_DPDM_PD		BIT10	/* Software Control USB PHY_DPDM_PD CONTROL Singal, 1: DPDM Pull Down */
#define MASK_SYSMISC_CTL0_HOST_SW				BIT8	/* PHY Connect Host or Device, 0: switch device, 1: switch host */
#define MASK_SYSMISC_CTL0_SEN2CDSP_CLKO_INV		BIT5	/* Invert CDSP Sensor Clock,1: invert */
#define MASK_SYSMISC_CTL0_SEN2CDSP_CLKO_EN		BIT4	/* Enable CDSP Sensor Clock, 0: disable, 1: enable */
#define MASK_SYSMISC_CTL0_CDSP_SOURCE_MIPI		BIT3	/* set CDSP source is from mipi interface */
#define MASK_SYSMISC_CTL0_CDSP_SOURCE_SENSOR	BIT2	/* set cdsp source is from parallel sensor */
#define MASK_SYSMISC_CTL0_CDSP_YUV_MODE			BIT1	/* CDSP Source format, 0: Bayer Raw, 1: YUV mode */
#define MASK_SYSMISC_CTL0_PPU_MIPI_SW			BIT0	/* PPU Sensor Source Swich, 0: from sensor, 1: from MIPI */

/********************* Define R_SYSTEM_MISC_CTRL4 bit mask (0xD0000084, SYSTEM MISC Control Register 4) *****************/
#define MASK_SYSMISC_CTL4_MAP_0_TO_SPIF			BIT15	/* In Internal boot ROM code and dis_bootremap is true, map address 0 to => 0: SD RAM, 1: SPI flash */
#define MASK_SYSMISC_CTL4_CDSP_SEN_SEL			BIT4	/* Internal data path selection, 0: cdsp_w->mux3m2m_cdsp_W, sen->x1_csi_mux, 1: cdsp_w->x1_csi_mux, sen->mux3m2m_cdsp_W */

typedef enum
{
	CLK_SYSTEM_BUS = 0,
	CLK_MEM_SDRAM,
	CLK_GPIO_INTERRUPT,
	CLK_HDMI,
	CLK_TIME_I2C_RTC,
	CLK_GTE,
	CLK_DAC,
	CLK_UART,
	CLK_MP3,
	CLK_SPI,
	CLK_ADC,
	CLK_422To420,
	CLK_SPU,
	CLK_TFT,
	CLK_CDSP,
	CLK_SDC,
	CLK_RSD,
	CLK_USBH,
	CLK_USBD,
	CLK_STN_WRAP,
	CLK_DMA,
	CLK_TV,
	CLK_PPU_SCALE1,
	CLK_IRAM,
	CLK_SPIFC,
	CLK_MIPI,
	CLK_JPEG,
	CLK_PPU_REG,
	CLK_SCALE0,
	CLK_NAND,
	CLK_SYSTEM_CTRL_REG,
	CLK_SYSTEM_CTRL_EFUSE,
	CLK_MAX
} MODULE_CLK;

extern INT32U  MCLK;
extern INT32U  MHZ;

extern INT32S sdram_calibrationed_flag(void);
extern INT8U mcu_version_get(void);
extern void system_set_pll(INT32U PLL_CLK);

extern void system_bus_arbiter_init(void);
extern void system_module_clk_enable(MODULE_CLK bit);
extern void system_module_clk_disable(MODULE_CLK bit);

extern void system_sdram_driving(void);
extern INT32S sys_sdram_self_refresh_thread_cycle(INT8U sref_cycle);
extern INT32S sys_sdram_CAS_latency_cycle(SDRAM_CL cl_cycle);
extern INT32S sys_sdram_wakeup_cycle(INT8U RC_cycle);
extern INT32S sys_sdram_refresh_period_ns(INT32U refresh_period_ns);
extern INT32S sys_sdram_init(SD_CLK_OUT_DELAY_CELL clk_out_dly_cell, SD_CLK_IN_DELAY_CELL clk_in_dly_cell, SD_CLK_PHASE clk_phase, SD_BUS_WIDTH bus_mode, SD_BANK_BOUND bank_bound, SD_SIZE sd_size, SD_SWITCH disable_enable);
extern INT32S sys_sdram_read_latch_at_CLK_Neg(INT8U ebable_disable);
extern INT32S sys_sdram_enable(SD_SWITCH sd_disable_enable);
extern INT32S sys_sdram_refresh_period_set(tREFI_ENUM tREFI);
extern INT32S sys_sdram_MHZ_set(SDRAM_CLK_MHZ sd_clk);
extern void sys_sdram_auto_refresh_set(INT8U cycle);
extern void sys_sdram_input_clock_dly_enable(INT8U ebable_disable);

extern void system_power_on_ctrl(void);

extern void system_enter_wait_mode(void);
extern void system_enter_halt_mode(void);
extern void system_enter_sleep_mode(void);
#endif		// __drv_l1_SYSTEM_H__
