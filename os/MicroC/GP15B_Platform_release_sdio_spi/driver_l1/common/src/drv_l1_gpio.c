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

/******************************************************************** 
* Purpose:  GPIO driver/interface
* Author: 
* Date:		2014/07/15
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
 
/******************************************************************************
 * Paresent Header Include
 ******************************************************************************/
#include "drv_l1_gpio.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined _DRV_L1_GPIO) && (_DRV_L1_GPIO == 1)                 //
//================================================================//

/******************************************************************************
 * Static Variables
 ******************************************************************************/ 
/*internal ram*/
static INT16U bit_mask_array[16];
static INT16U bit_mask_inv_array[16];
static INT32U gpio_data_out_addr[5];
static INT32U gpio_data_in_addr[5];
static INT8U  gpio_set_array[80];
static INT8U  gpio_setbit_array[80];
/*internal ram*/

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
void gpio_init(void);
//void gpio_pad_init(void);
BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction);
BOOLEAN gpio_read_io(INT32U port);
BOOLEAN gpio_write_io(INT32U port, BOOLEAN data);
BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute);
BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data);
BOOLEAN gpio_get_dir(INT32U port);
INT32U gpio_read_all_by_set(INT8U gpio_set_num);
BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level);
void gpio_monitor(GPIO_MONITOR *gpio_monitor);
void gpio_drving_uninit(void);
void gpio_basic_init(void);

/******************************************************************************
 * Function Body
 ******************************************************************************/

/*
Function Name	gpio_init
Description	Initialize GPIO
Header File	drv_l1_gpio.h
Syntax	void gpio_init(void)
Parameters	None
Return Values	None
*/
void gpio_init(void)
{
	/* initial all gpio */
	gpio_basic_init();

}

/*
Function Name	gpio_basic_init
Description	Initialize GPIO
Header File	drv_l1_gpio.h
Syntax	void gpio_init(void)
Parameters	None
Return Values	None
*/
void gpio_basic_init(void)
{
    INT16U i;
    for (i=0; i<5;i++)
    {
      //   DRV_Reg32(IOA_ATTRIB_ADDR+(0x20*i)) = 0xffff;  /*Set all gpio attribute to 1 */
         gpio_data_out_addr[i] = (0xC0000004 + (0x20*i));
         gpio_data_in_addr[i] = (0xC0000000 + (0x20*i));
    }
        
    for (i=0; i<16; i++)
    {
        bit_mask_array[i]=1<<i;
        bit_mask_inv_array[i]=~(bit_mask_array[i]);
    }

    for (i=0; i<80; i++)
    {
        gpio_set_array[i]=i/16;
        gpio_setbit_array[i]=i%16;
    };
}

/* This interface is for the application layer to initail the GPIO direction*/
/* init_io will not only modyfy the direction but also control the attribute value */
/*
Function Name	gpio_init_io
Description	General IO setting
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			direction: Value filled in direction and attribute bit, 1: set as 1, 0: set as 1
Return Values	1: Success
			0: Failed. The assigned port is not existed.
*/
BOOLEAN gpio_init_io(INT32U port, BOOLEAN direction)
{
    INT16U gpio_set;
    INT16U gpio_set_num;
    //INT32U trace;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    direction &= LOWEST_BIT_MASK;
    if (direction == GPIO_OUTPUT) {
        DRV_Reg32(IOA_ATTRIB_ADDR+(EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);  /*Set attribute to 0 for input */
        DRV_Reg32((IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (direction == GPIO_INPUT) {
        DRV_Reg32(IOA_ATTRIB_ADDR+(EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);  /*Set attribute to 1 for output */
        DRV_Reg32((IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else { return GPIO_FAIL; }
    
    return GPIO_OK;
}

/*
Function Name	gpio_read_io
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_read_io(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Input status of the whole IO port which contains this IO
*/
BOOLEAN gpio_read_io(INT32U port)
{
#if 0
    if (DRV_Reg32(gpio_data_in_addr[gpio_set_array[port]])&(bit_mask_array[gpio_setbit_array[port]]))
    {return 1;}
    else 
    {return 0;}
#else
    INT16U gpio_set; 
    INT16U gpio_set_num;
    /*debug k*/
    //INT32U k;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    //k = DRV_Reg32(IOA_DATA_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set) ;
    return ((DRV_Reg32(IOA_DATA_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);  
#endif

}
/*
Function Name	gpio_write_io
Description	Set IO port output data
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_io(INT32U port, BOOLEAN data)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
BOOLEAN gpio_write_io(INT32U port, BOOLEAN data)
{
#if 0
    if ((data&LOWEST_BIT_MASK))
    {
        DRV_Reg32(gpio_data_out_addr[gpio_set_array[port]]) |= bit_mask_array[gpio_setbit_array[port]];
    }
    else
    {
        DRV_Reg32(gpio_data_out_addr[gpio_set_array[port]]) &= bit_mask_inv_array[gpio_setbit_array[port]];
    }
    return GPIO_OK;
#else   

    INT16U gpio_set; 
    INT16U gpio_set_num;
    //INT32U trace;
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;  // gpio_set_array
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS; // gpio_setbit_array
    
    data &= LOWEST_BIT_MASK;
    if (data == DATA_HIGH){    
        DRV_Reg32((IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (data == DATA_LOW){
        DRV_Reg32((IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else  {return GPIO_FAIL; }

    return GPIO_OK;

#endif
    return GPIO_OK;
}


/*
Function Name	gpio_drving_init_io
Description	Set GPIO driving capability
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level)
Parameters	port: IO pin number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			gpio_driving_level: IO drivering capability
Return Values	1: Success
			0: Failed
*/
BOOLEAN gpio_drving_init_io(GPIO_ENUM port, IO_DRV_LEVEL gpio_driving_level)
{

    INT16U gpio_set;
    INT16U gpio_set_num;
    INT32U drv_level;

    //INT32U trace;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    drv_level = (INT32U) gpio_driving_level;


    if (port < IO_E8)  // IOA/B/C/D and E(8-bit) 
    {
        if (drv_level == 0)
        {
            DRV_Reg32(IOA_DRV+(EACH_DIR_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);  
        }
        else //if (drv_level == 1)
        {
            DRV_Reg32(IOA_DRV+(EACH_DIR_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);  
        }
    }
    else 
    {
          return GPIO_FAIL;
    }

	
    return GPIO_OK;
}

/*
Function Name	gpio_drving_uninit
Description	Set GPIO driving capability to the lowest level
Header File	drv_l1_gpio.h
Syntax	void gpio_drving_uninit(void)
Parameters	None
Return Values	None
*/
void gpio_drving_uninit(void)
{
    R_IOA_DRV = 0;
    R_IOB_DRV = 0;
    R_IOC_DRV = 0;
    R_IOD_DRV = 0;
    R_IOE_DRV = 0;
}


/*
Function Name	gpio_set_port_attribute
Description	Set IO port attribute
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			attribute: 1: set as 1, 0: set as 0
Return Values	1: Success
			0: Failed
*/
BOOLEAN gpio_set_port_attribute(INT32U port, BOOLEAN attribute)
{
    INT16U gpio_set;
    INT16U gpio_set_num;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    attribute &= LOWEST_BIT_MASK;
    if (attribute == ATTRIBUTE_HIGH) {
        DRV_Reg32((IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set)) |= (1 << gpio_set_num);
    }
    else if (attribute == ATTRIBUTE_LOW) {
        DRV_Reg32((IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set)) &= ~(1 << gpio_set_num);
    }
    else { return GPIO_FAIL; }

    return GPIO_OK; 
}

/*INPUT : 
         gpio_set_num: GPIO_SET_A, GPIO_SET_B, GPIO_SET_C, GPIO_SET_D
         write_data: 0x00001234h 
*/
/*
Function Name	gpio_write_all_by_set
Description	Set IO port output value
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
			write_data: 1: output high, 0: output low
			Output status of whole IO port which contains this IO
Return Values	1: Success
			0: Failed
*/
BOOLEAN gpio_write_all_by_set(INT32U gpio_set_num, INT32U write_data)
{
    if (gpio_set_num < GPIO_SET_MAX) {
        DRV_WriteReg32(IOA_BUFFER_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set_num,(write_data&LOWEST_BIT_MASK));
        return GPIO_OK;   
    }
    else {
        return GPIO_FAIL; 
    }
}
/*
Function Name	gpio_read_all_by_set
Description	Read IO port data
Header File	drv_l1_gpio.h
Syntax	INT32U gpio_read_all_by_set(INT8U gpio_set_num)
Parameters	gpio_set_num: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Not 0xFFFFFFFF: Input status of this IO
			0xFFFFFFFF: gpio_set_num is out of range
*/
INT32U gpio_read_all_by_set(INT8U gpio_set_num)
{
    if (gpio_set_num < GPIO_SET_MAX) {
        return DRV_Reg32(IOA_DATA_ADDR+EACH_GPIO_DATA_REG_OFFSET*gpio_set_num);   
    }
        return 0xFFFFFFFF;
      
}

/*
Function Name	gpio_get_dir
Description	Get IO port direction
Header File	drv_l1_gpio.h
Syntax	BOOLEAN gpio_get_dir(INT32U port)
Parameters	port: IO number(ex. IOA [0] is 0, IOB[0] is 16 and IOC[3] is 35.)
Return Values	Direction setting of the specified IO
*/
BOOLEAN gpio_get_dir(INT32U port)
{
    INT16U gpio_set; 
    INT16U gpio_set_num;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    
    return ((DRV_Reg32(IOA_DIR_ADDR+EACH_DIR_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);  
}


/*
Function Name	gpio_set_ice_en
Description	Set IOH[5:2] as JTAG pins or GPIOs
Header File	drv_l1_gpio.h
Syntax	void gpio_set_ice_en(BOOLEAN status)
Parameters	status: 1: BKUEB, 0: GPIOs
Return Values	None
*/
void gpio_set_ice_en(BOOLEAN status)
{
	if (status == TRUE) {
	  R_GPIOCTRL |= 1; /* enable */
	}
	else  {
	  R_GPIOCTRL &= ~1; /* disable */
	}	
}



#if 0
static BOOLEAN gpio_get_attrib(INT32U port)
{
    INT16U gpio_set; 
    INT16U gpio_set_num;
    
    gpio_set = port / EACH_REGISTER_GPIO_NUMS;
    gpio_set_num = port % EACH_REGISTER_GPIO_NUMS;
    
    return ((DRV_Reg32(IOA_ATTRIB_ADDR+EACH_ATTRIB_REG_OFFSET*gpio_set) >> gpio_set_num) & LOWEST_BIT_MASK);  
}
#endif

#ifndef _GPIO_MONITOR_DEF_
#define _GPIO_MONITOR_DEF_
typedef struct {
    INT8U KS_OUT74_EN   ;
    INT8U KS_OUT31_EN   ;
    INT8U KS_OUT0_EN    ;
    INT8U KS_EN         ;
    INT8U TS_all_en     ;
    INT8U SEN_EN        ;
    INT8U KEYN_IN2_EN   ;
    INT8U KSH_EN        ;
    INT8U ND_SHARE_EN   ;
    INT8U NAND_EN       ;
    INT8U UART_EN       ;
    INT8U TFTEN         ;
    INT8U TFT_MODE1     ;
    INT8U TFT_MODE0     ;
    INT8U EN_CF         ;
    INT8U EN_MS         ;
    INT8U EN_SD123      ;
    INT8U EN_SD         ;
    INT8U USEEXTB       ;
    INT8U USEEXTA       ;
    INT8U SPI1_EN       ;
    INT8U SPI0_EN       ;
    INT8U CCP_EN2       ;
    INT8U CCP_EN1       ;
    INT8U CCP_EN0       ;
    INT8U KEYC_EN       ;
    INT8U CS3_0_EN_i3   ;
    INT8U CS3_0_EN_i2   ;
    INT8U CS3_0_EN_i1   ;
    INT8U CS3_0_EN_i0   ;
    INT8U XD31_16_EN    ;
    INT8U BKOEB_EN      ;
    INT8U BKUBEB_EN     ;
    INT8U XA24_19EN3    ;
    INT8U XA24_19EN2    ;
    INT8U XA24_19EN1    ;
    INT8U XA24_19EN0    ;
    INT8U SDRAM_CKE_EN  ;
    INT8U SDRAM_CSB_EN  ;
    INT8U XA24_19EN5    ;
    INT8U XA24_19EN4    ;
    INT8U XA14_12_EN2   ;
    INT8U XA14_12_EN1   ;
    INT8U XA14_12_EN0   ;
    INT8U EPPEN         ;
    INT8U MONI_EN       ;
} GPIO_MONITOR;

#endif  //_GPIO_MONITOR_DEF_
/*
Function Name	gpio_monitor
Description	Read monitor statues
Header File	drv_l1_gpio.h
Syntax	void gpio_monitor(GPIO_MONITOR *gpio_monitor)
Parameters	Pointer of structure which is used to store the monitor status
Return Values	None
*/
void gpio_monitor(GPIO_MONITOR *gpio_monitor)
{
    INT8U smoni0=R_SMONI0;
    INT8U smoni1=R_SMONI1;
    
    gpio_monitor->KS_OUT74_EN   = (smoni0>>3  ) & 0x1 ;        
    gpio_monitor->KS_OUT31_EN   = (smoni0>>2  ) & 0x1 ;        
    gpio_monitor->KS_OUT0_EN    = (smoni0>>1  ) & 0x1 ;        
    gpio_monitor->KS_EN         = (smoni0>>0  ) & 0x1 ;        
    gpio_monitor->TS_all_en     = (smoni0>>7  ) & 0x1 ;        
    gpio_monitor->SEN_EN        = (smoni0>>6  ) & 0x1 ;        
    gpio_monitor->KEYN_IN2_EN   = (smoni0>>5  ) & 0x1 ;        
    gpio_monitor->KSH_EN        = (smoni0>>4  ) & 0x1 ;        
    gpio_monitor->ND_SHARE_EN   = (smoni0>>10  ) & 0x1 ;        
    gpio_monitor->NAND_EN       = (smoni0>>9  ) & 0x1 ;        
    gpio_monitor->UART_EN       = (smoni0>>8  ) & 0x1 ;        
    gpio_monitor->TFTEN         = (smoni0>>14  ) & 0x1 ;        
    gpio_monitor->TFT_MODE1     = (smoni0>>13  ) & 0x1 ;        
    gpio_monitor->TFT_MODE0     = (smoni0>>12  ) & 0x1 ;        
    gpio_monitor->EN_CF         = (smoni0>>19  ) & 0x1 ;        
    gpio_monitor->EN_MS         = (smoni0>>18  ) & 0x1 ;        
    gpio_monitor->EN_SD123      = (smoni0>>17  ) & 0x1 ;        
    gpio_monitor->EN_SD         = (smoni0>>16  ) & 0x1 ;        
    gpio_monitor->USEEXTB       = (smoni0>>23  ) & 0x1 ;        
    gpio_monitor->USEEXTA       = (smoni0>>22  ) & 0x1 ;        
    gpio_monitor->SPI1_EN       = (smoni0>>21  ) & 0x1 ;        
    gpio_monitor->SPI0_EN       = (smoni0>>20  ) & 0x1 ;        
    gpio_monitor->CCP_EN2       = (smoni0>>27  ) & 0x1 ;        
    gpio_monitor->CCP_EN1       = (smoni0>>26  ) & 0x1 ;        
    gpio_monitor->CCP_EN0       = (smoni0>>25  ) & 0x1 ;        
    gpio_monitor->KEYC_EN       = (smoni0>>24  ) & 0x1 ;        
    
    gpio_monitor->CS3_0_EN_i3   = (smoni1>>3  ) & 0x1 ;
    gpio_monitor->CS3_0_EN_i2   = (smoni1>>2  ) & 0x1 ;
    gpio_monitor->CS3_0_EN_i1   = (smoni1>>1  ) & 0x1 ;
    gpio_monitor->CS3_0_EN_i0   = (smoni1>>0  ) & 0x1 ; 
    gpio_monitor->XD31_16_EN    = (smoni1>>6  ) & 0x1 ; 
    gpio_monitor->BKOEB_EN      = (smoni1>>5  ) & 0x1 ; 
    gpio_monitor->BKUBEB_EN     = (smoni1>>4  ) & 0x1 ; 
    gpio_monitor->XA24_19EN3    = (smoni1>>11  ) & 0x1 ;  
    gpio_monitor->XA24_19EN2    = (smoni1>>10  ) & 0x1 ;  
    gpio_monitor->XA24_19EN1    = (smoni1>>9  ) & 0x1 ;  
    gpio_monitor->XA24_19EN0    = (smoni1>>8  ) & 0x1 ; 
    gpio_monitor->SDRAM_CKE_EN  = (smoni1>>15  ) & 0x1 ;  
    gpio_monitor->SDRAM_CSB_EN  = (smoni1>>14  ) & 0x1 ;  
    gpio_monitor->XA24_19EN5    = (smoni1>>13  ) & 0x1 ;  
    gpio_monitor->XA24_19EN4    = (smoni1>>12  ) & 0x1 ; 
    gpio_monitor->XA14_12_EN2   = (smoni1>>18  ) & 0x1 ;
    gpio_monitor->XA14_12_EN1   = (smoni1>>17  ) & 0x1 ;
    gpio_monitor->XA14_12_EN0   = (smoni1>>16  ) & 0x1 ; 
    gpio_monitor->EPPEN         = (smoni1>>20  ) & 0x1 ; 
    gpio_monitor->MONI_EN       = (smoni1>>24  ) & 0x1 ;     
}


void gpio_IOE_switch_config_from_HDMI_to_GPIO(void)
{
	INT32U HDMI_CTRL = R_SYSTEM_HDMI_CTRL;
	INT32U CLK_EN0 = R_SYSTEM_CLK_EN0;
	R_SYSTEM_HDMI_CTRL &= (~0x8000);	// must
	R_SYSTEM_CLK_EN0  |= 0x8;			// must

	R_SYSTEM_CKGEN_CTRL |= 0x4;
	R_SYSTEM_PLLEN |= 0x80;
	R_HDMITXPHYCONFIG1 |= 0x1;
	R_SYSTEM_CKGEN_CTRL &= (~0x4);
	R_SYSTEM_PLLEN &= (~0x80);

	R_SYSTEM_HDMI_CTRL = HDMI_CTRL;
	R_SYSTEM_CLK_EN0 = CLK_EN0;
}

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(defined _DRV_L1_GPIO) && (_DRV_L1_GPIO == 1)            //
//================================================================//
