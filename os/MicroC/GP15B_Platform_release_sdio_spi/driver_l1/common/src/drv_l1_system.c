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
#include "drv_l1_system.h"
#include "drv_l1_sfr.h"

#if (defined _DRV_L1_SYSTEM) && (_DRV_L1_SYSTEM == 1)             
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define GPB51PG_A    0x5F313030
#define GPB51PG_B    0x5F323030

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define R_MCU_VER_GET	   (*((volatile INT32U *) 0xF0005FEC))

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static tREFI_ENUM refresh_period = tREFI_15o6u;
INT32U MCLK = INIT_MCLK;
INT32U MHZ  = INIT_MHZ;


INT32S sdram_calibrationed_flag(void)
{
    if ((R_MEM_SDRAM_CTRL0==0x13)   &&
        (R_MEM_SDRAM_CTRL1==0x3000) &&
        (R_MEM_SDRAM_TIMING==0xF8A) &&
        (R_MEM_SDRAM_CBRCYC==0x100) &&
        (R_MEM_SDRAM_MISC==0x6)) {
        return 0;
    }

    return 1;
}

INT8U mcu_version_get(void)
{
	INT32U value;

	value = R_MCU_VER_GET;
	if (value == GPB51PG_A) {			// GPB51PG Version A
		return 1;
	} else if (value == GPB51PG_B) {	// GPB51PG Version B
		return 2;
	}
	return 255;
}

void system_set_pll(INT32U PLL_CLK)
{
    sys_pll_set(PLL_CLK,0,0,0,0,R_MEM_DRV);
}

void system_bus_arbiter_init(void)
{
	R_MEM_M2_BUS_PRIORITY =0;	/* usb20 */
	R_MEM_M3_BUS_PRIORITY =0;	/* ppu */
	R_MEM_M4_BUS_PRIORITY =0;	/* dma */
	R_MEM_M5_BUS_PRIORITY =0; 	/* jpeg */
	R_MEM_M6_BUS_PRIORITY =0;	/* scale */
	R_MEM_M7_BUS_PRIORITY =0; 	/* spu */
	R_MEM_M8_BUS_PRIORITY =0;	/* nfc */
	R_MEM_M9_BUS_PRIORITY =0;	/* cpu */
	R_MEM_M10_BUS_PRIORITY =0;	/* mp3 */
	R_MEM_M11_BUS_PRIORITY =0;	/* mp4 */
}

void system_module_clk_enable(MODULE_CLK bit)
{
	if(bit < 16) {
		R_SYSTEM_CLK_EN0 |= 1 << bit;
	} else {
		bit -= 16;
		R_SYSTEM_CLK_EN1 |= 1 << bit;
	}
}

void system_module_clk_disable(MODULE_CLK bit)
{
	if(bit < 16) {
		R_SYSTEM_CLK_EN0 &= ~(1 << bit);
	} else {
		bit -= 16;
		R_SYSTEM_CLK_EN1 &= ~(1 << bit);
	}
}

void system_sdram_driving(void)
{
	R_MEM_DRV = 0; /* 4mA */
}

/* if the sref_cycle less, more easy(more slow) to entry self refresh latch mode */
INT32S sys_sdram_self_refresh_thread_cycle(INT8U sref_cycle)
{
    INT32U sdram_ctrl1_reg_val;
    sdram_ctrl1_reg_val = R_MEM_SDRAM_CTRL1;
    sdram_ctrl1_reg_val &= ~(0x00FF); /*clear [7:0]*/
    sdram_ctrl1_reg_val |= sref_cycle;
    if (sref_cycle!=0 && sref_cycle < 64) {
    	return STATUS_FAIL;
    }
    
    R_MEM_SDRAM_CTRL1 = sdram_ctrl1_reg_val;
    return STATUS_OK;
}

/* under 96MHz, SDRAM can support CL2 always. */
INT32S sys_sdram_CAS_latency_cycle(SDRAM_CL cl_cycle)
{
    INT32U sdram_ctrl1_reg_val;
    INT8U  cl_val;

    cl_val=(INT8U) cl_cycle;
    sdram_ctrl1_reg_val = R_MEM_SDRAM_CTRL1;
    sdram_ctrl1_reg_val &= ~(0x3000); /*clear [13:12]*/
    sdram_ctrl1_reg_val |= ((cl_val&0x3)<<12);
    if (cl_val<2) {
    	return STATUS_FAIL;
    }

    R_MEM_SDRAM_CTRL1 = sdram_ctrl1_reg_val;
    return STATUS_OK;
}

/* when entry the latch mode, need RC cycle times to wake up */
INT32S sys_sdram_wakeup_cycle(INT8U RC_cycle)
{
    INT32U sdram_timing_reg_val;

    sdram_timing_reg_val = R_MEM_SDRAM_TIMING;
    RC_cycle &= 0xF;  /* fore bits effect */
    sdram_timing_reg_val &= ~(0xF);
    sdram_timing_reg_val |= RC_cycle;
    R_MEM_SDRAM_TIMING = sdram_timing_reg_val;

    return STATUS_OK;
}

/* large period time get more effect,default is 15600 ns (15.6us), don't less then it */
INT32S sys_sdram_refresh_period_ns(INT32U refresh_period_ns)
{
    INT32U cbrcyc_reg_val;

    //cbrcyc_reg_val = R_MEM_SDRAM_CBRCYC;
    
    if (refresh_period_ns < 15600) {
    	return STATUS_FAIL;
    }

    cbrcyc_reg_val=(refresh_period_ns*MHZ/1000)&0xFFFF;
    R_MEM_SDRAM_CBRCYC = cbrcyc_reg_val;

    return STATUS_OK;
}


#ifndef __CS_COMPILER__
	#pragma arm section rwdata="sdram_ctrl", code="sdram_ctrl"
#else
	INT32S sys_sdram_init(SD_CLK_OUT_DELAY_CELL clk_out_dly_cell, 
						SD_CLK_IN_DELAY_CELL clk_in_dly_cell,
	                    SD_CLK_PHASE clk_phase, 
	                    SD_BUS_WIDTH bus_mode, 
	                    SD_BANK_BOUND bank_bound,
	                    SD_SIZE sd_size, 
	                    SD_SWITCH disable_enable) __attribute__ ((section(".sdram_ctrl")));
	INT32S sys_sdram_read_latch_at_CLK_Neg(INT8U ebable_disable) __attribute__ ((section(".sdram_ctrl")));
#endif

INT32S sys_sdram_init(SD_CLK_OUT_DELAY_CELL clk_out_dly_cell, SD_CLK_IN_DELAY_CELL clk_in_dly_cell,
                    SD_CLK_PHASE clk_phase, SD_BUS_WIDTH bus_mode, SD_BANK_BOUND bank_bound, 
                    SD_SIZE sd_size, SD_SWITCH disable_enable)
{
    INT32U sdram_ctrl0_val;

    bank_bound = SD_BANK_1M;

    if(clk_in_dly_cell!=SDRAM_IN_DLY_DISABLE)
    {
        sys_sdram_input_clock_dly_enable(1);
    } else {
        sys_sdram_input_clock_dly_enable(0);
        clk_in_dly_cell=SDRAM_IN_DLY_LV0;
    }

    sdram_ctrl0_val = R_MEM_SDRAM_CTRL0;
    sdram_ctrl0_val &= ~(0xFFEF);  /* keep sdram enable and disable status */

    sdram_ctrl0_val |= (((clk_out_dly_cell & 0xF))<<12 | ((clk_phase & 1)<<11) | ((clk_in_dly_cell&0x7)<<8)| ((bank_bound&0x1)<<6) | ((bus_mode&0x1)<<5) |((disable_enable&0x1)<<4) | ((sd_size & 0x7)<<0));
    R_MEM_SDRAM_CTRL0 = sdram_ctrl0_val;

    return STATUS_OK;
}

INT32S sys_sdram_read_latch_at_CLK_Neg(INT8U ebable_disable)
{
    INT32U sdram_misc_val;

    sdram_misc_val = R_MEM_SDRAM_MISC;

    if (ebable_disable==0) {
        sdram_misc_val &= ~(0x2);
    } else {
        sdram_misc_val |= 0x2;
    }
    sdram_misc_val =0x6;
    R_MEM_SDRAM_MISC = sdram_misc_val;
    return STATUS_OK;
}

#ifndef __CS_COMPILER__
#pragma arm section rwdata, code
#endif

INT32S sys_sdram_enable(SD_SWITCH sd_disable_enable)
{
    INT32U sdram_ctrl0_val;
    sdram_ctrl0_val = R_MEM_SDRAM_CTRL0;
    sdram_ctrl0_val &= ~(0x0010); /* clear bit4 */
    sdram_ctrl0_val |= (sd_disable_enable&0x1)<<4;
    R_MEM_SDRAM_CTRL0 = sdram_ctrl0_val;

    return STATUS_OK;
}

INT32S sys_sdram_refresh_period_set(tREFI_ENUM tREFI) 
{
    refresh_period = tREFI;
    return STATUS_OK;
}

INT32S sys_sdram_MHZ_set(SDRAM_CLK_MHZ sdram_clk)
{

    INT32U cbrcyc_reg_val;
    INT32U oc=1; /* Internal test for overing clock */

    //cbrcyc_reg_val = R_MEM_SDRAM_CBRCYC;

    cbrcyc_reg_val=(refresh_period*sdram_clk/10)&0xFFFF * oc;

    R_MEM_SDRAM_CBRCYC = cbrcyc_reg_val;

    return STATUS_OK;
}

void sys_sdram_auto_refresh_set(INT8U cycle)
{
	R_MEM_SDRAM_CTRL1 &= ~0xFF;
	R_MEM_SDRAM_CTRL1 |= cycle;
}

void sys_sdram_input_clock_dly_enable(INT8U ebable_disable)
{
    INT32U sdram_misc_reg_val;

    sdram_misc_reg_val = R_MEM_SDRAM_MISC;

    if (ebable_disable==0) {
        sdram_misc_reg_val &= ~(0x4);
    } else {
        sdram_misc_reg_val |= (0x4);
    }
    R_MEM_SDRAM_MISC = sdram_misc_reg_val;
}
  
#ifndef __CS_COMPILER__
	#pragma arm section rwdata="pwr_ctrl", code="pwr_ctrl"
	#pragma O2
	void system_power_on_ctrl(void);
#else
	void system_power_on_ctrl(void) __attribute__ ((section(".pwr_ctrl")));
#endif
    
void system_power_on_ctrl(void)
{
	
}

// End POWER ON Functions 
#ifndef __CS_COMPILER__
#pragma O0
#else
#pragma GCC optimize("O0")
#endif
void system_enter_wait_mode(void)
{
	INT32U data;
	
	R_SYSTEM_WAIT = 0x5005;
    data = R_CACHE_CTRL;
    if(data){};//removing compiling warnings

    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
}

void system_enter_halt_mode(void)
{
	INT32U data;

#if 1	
	R_SYSTEM_CTRL &= ~0x2; 	/* CPU reset when wakeup */
#else
	R_SYSTEM_CTRL |= 0x2; 	/* CPU next instruction when wakeup */
#endif

    R_SYSTEM_HALT = 0x500A;
    data = R_CACHE_CTRL;
    if(data){};//removing compiling warnings

    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
}

void system_enter_sleep_mode(void)
{
	INT32U data;
	
    R_SYSTEM_SLEEP = 0xA00A;
   	data = R_CACHE_CTRL;
   	if(data){};//removing compiling warnings

    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
    ASM(NOP);
}

#endif //(defined _DRV_L1_SYSTEM) && (_DRV_L1_SYSTEM == 1)
