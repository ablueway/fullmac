/*
* Purpose: System option setting
*
* Author: dominantyang
*
* Date: 2009/12/8
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
* History :

     1 . system config in iRAM (internal ram)
*/

#include "drv_l1_isys.h"
//#include "drv_l1_tools.h"


#define R_MCU_VER_GET	   (*((volatile INT32U *) 0xF0001E00))
#define GPL32X_A    0x00000012
#define GPL32X_B    0xE2811002
#define GPL32X_C    0xE2811003
#define GPL326_A    0x00826959

#ifndef _SDRAM_DRV
#define _SDRAM_DRV
typedef enum {
    SDRAM_DRV_4mA=0x0000,
    SDRAM_DRV_8mA=0x5555,
    SDRAM_DRV_12mA=0xAAAA,
    SDRAM_DRV_16mA=0xFFFF
} SDRAM_DRV;
#endif

#ifndef _NEG_SAMPLE
#define _NEG_SAMPLE
typedef enum {
    NEG_OFF=0x0,
    NEG_ON=0x2
} NEG_SAMPLE;
#endif

#ifndef _PLL_REG_V
#define _PLL_REG_V
typedef enum {
    PLL_1o8V  = 0x00,
    PLL_1o65V = 0x10, 
    PLL_1o5V  = 0x20,
    PLL_1o35V = 0x30,
    PLL_1o95V = 0x40,
    PLL_2o1V  = 0x50
} PLL_REG_V;
#endif

INT32S sys_pll_set(INT32U PLL_CLK,INT8U sdram_out_dly,INT8U sdram_in_dly,SD_SIZE sdram_size,NEG_SAMPLE neg,SDRAM_DRV sdram_driving)
{
    INT32U data;
    INT32U sys_ctrl_reg_val=R_SYSTEM_CLK_CTRL;
 
    if ((sdram_out_dly+sdram_in_dly)!=0) 
    { 
        if (PLL_CLK<=48) {
        R_SYSTEM_LVR_CTRL=PLL_1o5V;        
        } else if (PLL_CLK<=72) {
            R_SYSTEM_LVR_CTRL=PLL_1o65V;
        } else {
            R_SYSTEM_LVR_CTRL=PLL_1o8V;
        }
        
        /*Memory Configuration Value Setting*/    
        R_MEM_SDRAM_MISC |= 0x4; // enable input delay (must be)
        R_MEM_SDRAM_MISC |= neg; // negtive
        if (PLL_CLK <= 100) {
            R_MEM_SDRAM_CTRL1=0x2000; /*default is CAS slow CL3 for compatible*/
        } else {
            R_MEM_SDRAM_CTRL1=0x3000;
        }

        R_MEM_SDRAM_CTRL0 = (((sdram_out_dly & 0xF))<<12 |((sdram_in_dly&0x7)<<8) |(1<<4) | ((sdram_size & 0x7)<<0));
    }

    R_MEM_DRV = sdram_driving;  
    /*PLL setting*/
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting stable

    if (PLL_CLK > 6) {
        /*2013Aug16*/
        //sys_ctrl_reg_val |=0x0400; // dominant add, 11/18/2008
        sys_ctrl_reg_val &= ~(0x8008); // dominant add, 11/18/2008
		
		sys_clk_set(PLL_CLK*1000000);
		
        sys_ctrl_reg_val |= 0x8008; // Fast PLL Enable
     #if (_DRV_L1_DAC ==1) || (SUPPORT_AD_KEY!=KEY_AD_NONE) || (SUPPORT_AUD_RECODER==CUSTOM_ON)
     	sys_ctrl_reg_val |= 0x10; /* DA/AD PLL Enable */
     #endif
        R_SYSTEM_CLK_CTRL = sys_ctrl_reg_val;
       // sys_sdram_MHZ_set(PLL_CLK);  // dominant add, 11/18/2008
        if (sdram_size<SD_256Mb) { /* according to general SDRAM Spec. */
            data = tREFI_15o6u;
        } else { 
            data = tREFI_7o8u;
        }
        R_MEM_SDRAM_CBRCYC=(data*PLL_CLK/10)&0xFFFF;
    } else if (PLL_CLK == 6) {
    	R_SYSTEM_CLK_CTRL = 0x400;
    } else {

    }
	/* If the 6MHz crystal is the only source used in the system, please don't enable this bit */
 	R_SYSTEM_PLLEN |= 0x100; /* system 32K clock from external 32768Hz crystal */
    
    data =(*((volatile INT32U *) 0xFF000000));//DUMMY OP
    data =(*((volatile INT32U *) 0xFF000001));//DUMMY OP
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting stable

    return R_SYSTEM_PLLEN;
}

#if 0 

INT32S sys_clk_set(INT32U PLL_CLK_HZ)
{
	INT32U data;
	INT32U pll;


    if (PLL_CLK_HZ==0 ) {
       MCLK = 48000000;
       return MCLK;
    }
    
    if(MCLK == PLL_CLK_HZ) {
    	return MCLK;
    }
    
    MCLK = PLL_CLK_HZ;
    
    /*PLL setting*/
    while (R_SYSTEM_POWER_STATE == SYS_POWER_STATE_CLKCHG) {;;} //waiting stable

    R_SYSTEM_CLK_CTRL &= ~(0x8008); // Fast PLL disable, Entry Slow Mode

    data =(*((volatile INT32U *) 0xFF000000));//DUMMY OP
    data =(*((volatile INT32U *) 0xFF000001));//DUMMY OP
    
    while (R_SYSTEM_POWER_STATE == SYS_POWER_STATE_CLKCHG) {;;} //waiting slow mode stable
   

	if(PLL_CLK_HZ >= 48000000){	
		pll = R_SYSTEM_PLLEN;
		pll &= ~0x3f;
		pll |= 2 + (PLL_CLK_HZ/1000000-48)/4;
		R_SYSTEM_PLLEN = pll;
		
		DRV_USEC_WAIT(300);// wait 3ms
		R_SYSTEM_CLK_CTRL |= 0x8008; // Fast PLL ENABLE
    }
    
    data =(*((volatile INT32U *) 0xFF000000));//DUMMY OP
    data =(*((volatile INT32U *) 0xFF000001));//DUMMY OP
    if(data){};//remove compiler warning
    while (R_SYSTEM_POWER_STATE == SYS_POWER_STATE_CLKCHG) {;;} //waiting fast mode stable
	
	return MCLK;
}

#endif
/**
 * @brief   get system pll clock
 * @param   none
 * @return 	result; system pll 
 */
INT32U sys_clk_get(void)
{
	INT32U pllValue = 0;
	INT32U sysClockHz = 0;
	
	if(R_SYSTEM_CLK_CTRL & SYS_CLK_SRC0_MASK)//check bit 14
	{// sysClk from C32K
		sysClockHz = 32000;
	}
	else 
	{
		if(R_SYSTEM_CLK_CTRL & SYS_CLK_SRC1_MASK)//check bit 15
		{// fast PLL
			pllValue = R_SYSTEM_PLLEN & SYS_FAST_PLL_MASK;
			if(pllValue <= 2)
			{
				sysClockHz = 48000000;//48MHz
			}
			else
			{
				sysClockHz = 48000000 + (pllValue - 2) * 4000000;
			}
		}
		else
		{// slow PLL 12MHz
			sysClockHz = 12000000;
		}
	}
	
	sysClockHz = sysClockHz >> (R_SYSTEM_CLK_CTRL & SYS_CLK_DIV_MASK);
	
	return sysClockHz;

}
