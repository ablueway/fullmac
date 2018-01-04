/******************************************************************** 
* Purpose: keychange driver/interface
* Author: 
* Date: 
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
* Version : 1.00
* History :
*********************************************************************/
#include "drv_l1_sfr.h"
#include "driver_l1_cfg.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_timer.h"
#include "drv_l1_keychange.h"
#include "drv_l1_gpio.h"


#if _OPERATING_SYSTEM != _OS_NONE
#include "os.h"
#endif

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined _DRV_L1_KEYCHANGE) && (_DRV_L1_KEYCHANGE == 1)					  //
//================================================================//
/****************************************************
*		constant 									*
****************************************************/

/****************************************************
*		marco										*
****************************************************/
#define CHECK_RETURN(x) \
{\
	ret = x;\
	if(ret < 0) {\
		goto __exit;\
	}\
}\

/****************************************************
*		data type 									*
*****************************************************/

/****************************************************
*		function
*****************************************************/
static void keychange_isr(void);

/****************************************************
*		varaible 									*
*****************************************************/
//void (*rtc_user_isr[RTC_INT_MAX])(void);
void (*keychange_user_isr[KEYCHG_INT_MAX])(void);


//GPIO_ENUM array

//#define VIC_RTC VIC_ALM_SCH_HMS
void drv_l1_keychange_init(void)
{
    INT8U i ;
    
    vic_irq_disable(VIC_KEY_CHANGE);
    R_KEYCH_INT = 0 ;       // clear interrupt enable bits
    R_KEYCH_INT |= 0xAAAA ;  // clear interrupt status bit
    //R_KEYCH_ENABLE = 1 ;  // disable all keychange function (not for interrupt)
    
    // reset int isr
    for (i=0; i<KEYCHG_INT_MAX; i++)
        keychange_user_isr[i] = 0 ;

    // init GPIO
	gpio_init_io(IO_B9,GPIO_INPUT);
	gpio_init_io(IO_C12,GPIO_INPUT);//share with ICE
	gpio_init_io(IO_C13,GPIO_INPUT);//share with ICE
	gpio_init_io(IO_C14,GPIO_INPUT);//share with ICE
	gpio_init_io(IO_C15,GPIO_INPUT);//share with ICE
	gpio_init_io(IO_D12,GPIO_INPUT);
    gpio_init_io(IO_D14,GPIO_INPUT);
    gpio_init_io(IO_D15,GPIO_INPUT);

	gpio_set_port_attribute(IO_B9 , 1);    
	gpio_set_port_attribute(IO_C12, 1); 
	gpio_set_port_attribute(IO_C13, 1); 
	gpio_set_port_attribute(IO_C14, 1); 
	gpio_set_port_attribute(IO_C15, 1); 
	gpio_set_port_attribute(IO_D12, 1); 
	gpio_set_port_attribute(IO_D14, 1); 
	gpio_set_port_attribute(IO_D15, 1); 

	gpio_write_io(IO_B9 , 0);    
	gpio_write_io(IO_C12, 0); 
	gpio_write_io(IO_C13, 0); 
	gpio_write_io(IO_C14, 0); 
	gpio_write_io(IO_C15, 0); 
	gpio_write_io(IO_D12, 0); 
	gpio_write_io(IO_D14, 0); 
	gpio_write_io(IO_D15, 0); 

    drv_l1_keychange_latch();
    R_SYSTEM_CLK_CTRL |= 1 << 9 ;
    R_KEYCH_INT |= 0xAAAA ;  // clear interrupt status bit

	vic_irq_register(VIC_KEY_CHANGE, keychange_isr);	// register ext rtc isr 
	vic_irq_enable(VIC_KEY_CHANGE);
}

static void keychange_isr(void)
{
    INT16U wStatusBit, wKeyChgNum ;
    INT16U wKeyChgRetValue ;
    
    wKeyChgRetValue = R_KEYCH_INT ;
    
    if (wKeyChgRetValue != 0) {
        DBG_PRINT("KeyChg=0x%08X\r\n", wKeyChgRetValue);
    }
    
    drv_l1_keychange_latch();
    for (wKeyChgNum=0; wKeyChgNum<KEYCHG_INT_MAX; wKeyChgNum++)
    {
        wStatusBit = 1<<((wKeyChgNum<<1)+1) ;
        if (wKeyChgRetValue& wStatusBit ) {
            if (keychange_user_isr[wKeyChgNum])
                //(*keychange_user_isr[wKeyChgNum])() ;
                keychange_user_isr[wKeyChgNum]() ;
        }
    }
    R_KEYCH_INT = wKeyChgRetValue ;  // clear all interrupt flag
}

void drv_l1_keychange_latch(void)
{
    INT8U temp;
    temp = R_KEYCH_LATCH ;
}



INT8S drv_l1_keychange_int_set(INT8U byKeyChgNum, INT8U bEnable, void (*keychange_isr)(void))
{

    INT16U wShift ;
    if (byKeyChgNum>=KEYCHG_INT_MAX)
        return STATUS_FAIL ;
        
#if _OPERATING_SYSTEM != _OS_NONE				// Soft Protect for critical section
	OSSchedLock();
#endif
    
    wShift = (byKeyChgNum<<1) ;
    R_KEYCH_INT |= (1<<(wShift+1)) ; // clear interrupt
    
    if (bEnable) {
        keychange_user_isr[byKeyChgNum] = keychange_isr ;
        R_KEYCH_INT |= (1<<wShift) ;  // enable interrupt
    } else {
        keychange_user_isr[byKeyChgNum] = NULL ;
        R_KEYCH_INT &= (0x5555 & ~(1<<wShift)) ; // disable interrupt
    }
#if _OPERATING_SYSTEM != _OS_NONE
	OSSchedUnlock();
#endif

    return STATUS_OK ;

}






#endif //(defined _DRV_L1_KEYCHANGE) && (_DRV_L1_KEYCHANGE == 1)

