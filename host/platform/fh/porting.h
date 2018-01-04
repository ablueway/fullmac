/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#include <ssv_types.h>
#include "printf.h"
#include "ucos_ii.h"

#define  FH8610  0x10

/*============GP Platform selection===================*/
#define HOST_PLATFORM_SEL FH8610

/* compile not support zero array */
#define NOT_SUPPORT_ZERO_ARRAY

/* sdio rx int, use card int trigger, pulldown/pullup */
#define SDIO_CARD_INT_TRIGGER

/* fh platfrom, 4byte align */
#define ETH_PAD_SIZE  2

/*============Task Priority===================*/
#define RX_ISR_PRIORITY    0 //No use for uCOS2
#define TMR_TASK_PRIORITY  18
#define MLME_TASK_PRIORITY 19
#define CMD_ENG_PRIORITY   20
#define WIFI_RX_PRIORITY   21
#define WIFI_TX_PRIORITY   22
#define TCPIP_PRIORITY     23
#define DHCPD_PRIORITY     24
#define NETAPP_PRIORITY    25

#define NETAPP_PRIORITY_1  NETAPP_PRIORITY+1  // 26
#define NETAPP_PRIORITY_2  NETAPP_PRIORITY+2  // 27
#define NETMGR_PRIORITY    NETAPP_PRIORITY+3  // 28
#define TASK_END_PRIO      NETAPP_PRIORITY+4  // 29

/*============Console setting===================*/
#define hal_print fhprintf
#define FFLUSH(x)

#define hal_putchar(ch) fhOutChar((char)ch) 
extern u8 hal_getchar(void);


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
#define ALIGN_ARRAY(a) __attribute__ ((aligned(a)))

/*============SSV-DRV setting===================*/
#define INTERFACE "sdio"
#define	CONFIG_RX_POLL      0
#define SDRV_INCLUDE_SDIO    1

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  64  //48 + 16 
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128  //96  + 32
#define TCPIP_STACK_SIZE     144 //128 + 16
#define DHCPD_STACK_SIZE     96  //64  + 32
#define NETAPP1_STACK_SIZE   144 //128 + 16
#define NETAPP2_STACK_SIZE   144 //128 + 16
#define NETAPP3_STACK_SIZE   144 //128 + 16
#define NETMGR_STACK_SIZE    112  //80  + 32
#define CLI_TASK_STACK_SIZE  64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   96  //64  + 32 
#define WIFI_TX_STACK_SIZE   96  //64  + 32 
#define DEV_INIT_STACK_SIZE  0   //64
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
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);

/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);

//=====================Platform wifi rc setting=======================
void ssv6xxx_wifi_set_rc_mask_api(u32 mask);
void ssv6xxx_wifi_set_rc_resent_api(bool on);
void ssv6xxx_wifi_set_rc_per_api(u32 up_per, u32 down_per);
                          
#endif
