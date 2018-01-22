/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

/* TODO:aaron */
//#include <linux/kernel.h>
#include <ssv_types.h>

#define GP_19B 0 //CPU 96MHz SPI
#define GP_15B 1 //144M SPI
#define GP_22B 2 //196M SDIO
/*============GP Platform selection===================*/
#define HOST_PLATFORM_SEL GP_15B

/*============Task Priority===================*/
#define RX_ISR_PRIORITY    0 //No use

//#include "application.h"
#include "os.h"
#define TMR_TASK_PRIORITY  18
#define MLME_TASK_PRIORITY 19
#define CMD_ENG_PRIORITY   20
#define WIFI_RX_PRIORITY   21
#define WIFI_TX_PRIORITY   22
#define TCPIP_PRIORITY     23
#define DHCPD_PRIORITY     24
#define NETAPP_PRIORITY    25

#define NETAPP_PRIORITY_1  NETAPP_PRIORITY+1
#define NETAPP_PRIORITY_2  NETAPP_PRIORITY+2
#define NETAPP_PRIORITY_3  NETAPP_PRIORITY+3
#define NETAPP_PRIORITY_4  NETAPP_PRIORITY+4
#define NETMGR_PRIORITY    NETAPP_PRIORITY+5
#define VDO_ENC_PRIO       NETAPP_PRIORITY+6
#define MJPG_STREAMER_PRIO NETAPP_PRIORITY+7
#define TASK_END_PRIO      NETAPP_PRIORITY+8  //33

/*============Console setting===================*/
#define hal_print printk
//#define PRINTF print_string
//#define stdout NULL
#define FFLUSH(x)


/* TODO:aaron */
/*============Compiler setting===================*/
#define ARM_ADS
#define PACK( __Declaration__ ) //__packed __Declaration__;
#undef STRUCT_PACKED
#define STRUCT_PACKED //__packed
#define UNION_PACKED //__packed
//LWIP PACK Definition
#define PACK_STRUCT_BEGIN //__packed
#define PACK_STRUCT_FIELD(x) //__packed x

//#define inline //__inline

#define ALIGN_ARRAY(a) //__align(a)
/*============SSV-DRV setting===================*/
#define	CONFIG_RX_POLL      0
//#define INTERFACE "sdio"
//#define INTERFACE "spi"
#define INTERFACE "usb"

#define SDRV_INCLUDE_SPI    0
#define SDRV_INCLUDE_SDIO   0

#define SDRV_INCLUDE_SIM	0
#define SDRV_INCLUDE_UART	0
#define SDRV_INCLUDE_USB	0

#define SDIO_CARD_INT_TRIGGER

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  64
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128
#define TCPIP_STACK_SIZE     128
#define DHCPD_STACK_SIZE     64
#define NETAPP1_STACK_SIZE    128
#define NETAPP2_STACK_SIZE    128
#define NETAPP3_STACK_SIZE    128
#define NETAPP4_STACK_SIZE    128
#define NETAPP5_STACK_SIZE    128
#define NETMGR_STACK_SIZE    80
#define CLI_TASK_STACK_SIZE  64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   64
#define WIFI_TX_STACK_SIZE   64
#define PING_THREAD_STACK_SIZE 0 //16 , ping thread doesn't enable now, I set staic size is zero to reduce data size.

#define TOTAL_STACK_SIZE (TMR_TASK_STACK_SIZE+ \
                          MLME_TASK_STACK_SIZE+ \
                          CMD_ENG_STACK_SIZE+ \
                          TCPIP_STACK_SIZE+ \
                          DHCPD_STACK_SIZE+ \
                          NETAPP1_STACK_SIZE+ \
						  NETAPP2_STACK_SIZE+ \
						  NETAPP3_STACK_SIZE+ \
						  NETAPP4_STACK_SIZE+ \
						  NETAPP5_STACK_SIZE+ \
                          NETMGR_STACK_SIZE+ \
                          CLI_TASK_STACK_SIZE+ \
						  RX_ISR_STACK_SIZE+ \
						  WIFI_RX_STACK_SIZE+ \
                          WIFI_TX_STACK_SIZE+ \
                          PING_THREAD_STACK_SIZE)
                          
/*============Memory========================*/
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);


/*============ Platform HW dependence ====================*/
void platform_dev_init(void); 

void platform_udelay(u32 us_delay);

#endif
