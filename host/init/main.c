/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <ssv_types.h>
#include <msgevt.h>
#include <pbuf.h>
#include <cmd_def.h>
#include <cli/cli.h>
#include <drv/ssv_drv.h>
#include <host_apis.h>

#include <netapp/net_app.h>

#include <cli/cmds/cli_cmd_wifi.h>



//


// #include <sim_cfg.h>

#if 0
struct ap_info_st sg_ap_info[20];
#endif // 0


/**
 *  Entry point of the firmware code. After system booting, this is the
 *  first function to be called from boot code. This function need to 
 *  initialize the chip register, software protoctol, RTOS and create
 *  tasks. Note that, all memory resources needed for each software
 *  modulle shall be pre-allocated in initialization time.
 */
/* return int to avoid from compiler warning */

#if CONFIG_PLATFORM_CHECK
//big   : 0x01 0x23 0x45 0x67
//little: 0x67 0x45 0x23 0x01
static void ssv6xxx_plform_chk()
{
    volatile u32 i=0x01234567;
    u8 *ptr;
    u16* ptr_u16;
    bool b_le = FALSE;
    // return 0 for big endian, 1 for little endian.
    PRINTF("============================================================\r\n");
    PRINTF("\t\t Platorm Check\r\n");
    PRINTF("************************************************************\r\n");

    if(((*((u8*)(&i))) == 0x67)){
        PRINTF("Little Endian\r\n");
        b_le=TRUE;
    }else{
        PRINTF("Big Endian\r\n");
    }
    ptr = (u8*)&i;
    PRINTF("Addr[%p] for i\r\n", ptr);
    
    ptr++;        
    ptr_u16 = (u16*)ptr;

    if(*ptr_u16 == 0x2345)
        PRINTF("Addr[%p] Support odd alignment\r\n", ptr_u16);
    else
        PRINTF("Addr[%p] NOT Support odd alignment\r\n", ptr_u16);
    
    ptr++;
    ptr_u16 = (u16*)ptr;
    
    if(b_le){
        if(*ptr_u16 == 0x0123)
            PRINTF("Addr[%p] Support 2 byte alignment\r\n", ptr_u16);
        else
            PRINTF("Addr[%p] NOT Support 2 byte alignment\r\n", ptr_u16);
            
    }else{
        
        if(*ptr_u16 == 0x4567)
            PRINTF("Addr[%p] Support 2 byte alignment\r\n", ptr_u16);
        else
            PRINTF("Addr[%p] NOT Support 2 byte alignment\r\n", ptr_u16);

    }

    PRINTF("============================================================\r\n");

}
#endif//CONFIG_PLATFORM_CHECK
extern int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode);

#ifdef __MTRACE_CHECK__
//Check memory leak tool
#include <mcheck.h>

#endif//#ifdef __MTRACE_CHECK__
OsTaskHandle dev_init_task_handle;

void dev_init_task(void *args)
{
        ssv6xxx_dev_init(SSV6XXX_HWM_STA);
#if CONFIG_PLATFORM_CHECK
        ssv6xxx_plform_chk();
#endif//CONFIG_PLATFORM_CHECK
        OS_TaskDelete(dev_init_task_handle);


}
#define DEV_INIT_PRIORITY   2

struct task_info_st dev_init_task_info[] =
{
    { "dev_init_task",  (OsMsgQ)0, 1, DEV_INIT_PRIORITY, DEV_INIT_STACK_SIZE, NULL, dev_init_task},
};

/************************************************************************************************************************************************************************
*
*   This main function just use in FREERTOS """"""SIM"""" platform
*   other platform need to call the function,ssv6xxx_dev_init to initialize wifi module
*   in the customer's task. 
*   Note: ssv6xxx_dev_init need to be call after os starting schedule(this function include some os commands inside)
*
************************************************************************************************************************************************************************/
s32 main(s32 argc, char *argv[])
{
#ifdef __MTRACE_CHECK__
	//log memroy heap status
	mtrace();
#endif//#ifdef __MTRACE_CHECK__

    OS_TaskCreate(dev_init_task_info[0].task_func,
              dev_init_task_info[0].task_name,
              dev_init_task_info[0].stack_size<<4,
              NULL,
              dev_init_task_info[0].prio,
              &dev_init_task_handle);

	/* Start the scheduler so our tasks start executing. */
    OS_StartScheduler();   

    return 0;
}

