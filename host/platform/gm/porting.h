/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/



#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_
#include <ssv_types.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

#include <lib_uart.h>

#define GM8136_PLATFORM   0x11
#define HOST_PLATFORM_SEL GM8136_PLATFORM

/*============Task Priority===================*/

#define RX_ISR_PRIORITY         0 //Non-used
#define CMD_ENG_PRIORITY        8
#define TMR_TASK_PRIORITY       8
#define MLME_TASK_PRIORITY      8
#define WIFI_RX_PRIORITY        8
#define WIFI_TX_PRIORITY        8
#define TCPIP_PRIORITY          7
#define NETAPP_PRIORITY         7
#define NETAPP_PRIORITY_1       7
#define NETAPP_PRIORITY_2       7
#define DHCPD_PRIORITY          7
#define NETMGR_PRIORITY         7
#define TASK_END_PRIO           7

#define MEMP_NUM_NETCONN        9
#define MEMP_NUM_TCP_PCB        9
#define MEMP_NUM_TCP_PCB_LISTEN 9
#define MEMP_NUM_UDP_PCB        9

/*============Console setting===================*/
#define PRINTF LOG_PRINTF
#define FFLUSH(x) fflush(x)
#define hal_getchar()   gm_uart_getc(0)
#define hal_print printf
#define hal_putchar(a)  gm_uart_putc(a, 0)


/*============Compiler setting===================*/
#define ARM_ADS
#undef STRUCT_PACKED
#define STRUCT_PACKED __attribute__ ((packed))
#define UNION_PACKED __attribute__ ((packed))
//LWIP PACK Definition
#define PACK_STRUCT_BEGIN   //#pragma pack(1)
#define PACK_STRUCT_END     //#pragma pack()
#define PACK_STRUCT_STRUCT     __attribute__ ((packed))
#define inline __inline
#define PACK(__Declaration__) __Declaration__ PACK_STRUCT_STRUCT;
#define PACK_STRUCT_FIELD(x)    x
#define ALIGN_ARRAY(a) 


/*============SSV-DRV setting===================*/
#define INTERFACE "sdio"
#define	CONFIG_RX_POLL      0
#define SDRV_INCLUDE_SDIO    1

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  64
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128
#define TCPIP_STACK_SIZE     128
#define DHCPD_STACK_SIZE     64
#define NETAPP1_STACK_SIZE    128
#define NETAPP2_STACK_SIZE    128
#define NETAPP3_STACK_SIZE    128
#define NETMGR_STACK_SIZE    80
#define CLI_TASK_STACK_SIZE  64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   64
#define WIFI_TX_STACK_SIZE   64
#define DEV_INIT_STACK_SIZE   64
#define PING_THREAD_STACK_SIZE 0 //16 , ping thread doesn't enable now, I set staic size is zero to reduce data size.

#define TOTAL_STACK_SIZE (TMR_TASK_STACK_SIZE+ \
                          MLME_TASK_STACK_SIZE+ \
                          CMD_ENG_STACK_SIZE+ \
                          TCPIP_STACK_SIZE+ \
                          DHCPD_STACK_SIZE+ \
                          NETAPP1_STACK_SIZE+ \
						  NETAPP2_STACK_SIZE+ \
						  NETAPP3_STACK_SIZE+ \
                          NETMGR_STACK_SIZE+ \
                          CLI_TASK_STACK_SIZE+ \
						  RX_ISR_STACK_SIZE+ \
						  WIFI_RX_STACK_SIZE+ \
                          WIFI_TX_STACK_SIZE+ \
                          DEV_INIT_STACK_SIZE+ \
                          PING_THREAD_STACK_SIZE)
                          
/*============Memory========================*/
#ifndef __MTRACE_CHECK__
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
#endif//#__MTRACE_CHECK__
OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);

/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);
                          
#endif
