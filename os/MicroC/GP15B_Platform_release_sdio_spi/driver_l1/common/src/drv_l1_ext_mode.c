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
#include "drv_l1_tools.h"

static INT32S iRAM_Delay(INT32U cnt)
{
	INT32U i;
	
	for (i=0; i<cnt;++i)
	{
		ASM(NOP);
	}
	return 0;
}

void drv_l1_system_clk_set(INT32U SysClk, INT32U SDramPara)
{
	SysClk |= (R_SYSTEM_PLLEN & (~0x1F));
	R_MEM_SDRAM_CTRL0 = SDramPara;	
	R_SYSTEM_PLLEN =  SysClk;
	
	iRAM_Delay(16);
}


void system_clk_ext_XLAT_12M(void)
{
	R_SYSTEM_MISC_CTRL1 |= 0x01;
	R_SYSTEM_CKGEN_CTRL |= 0x14C;

	{
		INT32U i;
		for (i=0;i<0xF000;++i) {
			R_RANDOM0 = i;		// delay for XLAT clock stable
		}
		
	}
	
	R_SYSTEM_CLK_CTRL &= 0x3fff;		//enter SLOW mode

	while ( (R_SYSTEM_POWER_STATE&0xF)!=1 ) {
		//DBG_PRINT("wait goint to SLOW mode\r\n");
		;		
	}
	
	R_SYSTEM_CKGEN_CTRL |= 0x21;		
	R_SYSTEM_CLK_CTRL |= 0x8000;		//enter FAST mode again

	
	while ( (R_SYSTEM_POWER_STATE&0xF)!=2 ) {
		//DBG_PRINT("wait coming back to FAST mode\r\n");
		;		
	}

	R_SYSTEM_CTRL |= 0x00000902;  // josephhsieh@140519 sensor «eºÝ©T©w¬°48MHz
	R_SYSTEM_CKGEN_CTRL |= 0x1F;
}

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

