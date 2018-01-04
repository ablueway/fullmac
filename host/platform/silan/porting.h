/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#include "ssv_types.h"

#include <ctype.h>

/* ============Platform selection=================== */

/* compile not support zero array */
//#define NOT_SUPPORT_ZERO_ARRAY

/* sdio rx int, use card int trigger, pulldown/pullup */
//#define SDIO_CARD_INT_TRIGGER

/* platfrom, 4byte align */
//#define ETH_PAD_SIZE  2

/*============Task Priority===================*/

#define RX_ISR_PRIORITY    SCHED_PRIORITY_DEFAULT// 
#define TMR_TASK_PRIORITY  (SCHED_PRIORITY_DEFAULT + 108)
#define MLME_TASK_PRIORITY (SCHED_PRIORITY_DEFAULT + 108)
#define CMD_ENG_PRIORITY   (SCHED_PRIORITY_DEFAULT + 108)
#define WIFI_RX_PRIORITY   (SCHED_PRIORITY_DEFAULT + 108)
#define WIFI_TX_PRIORITY   (SCHED_PRIORITY_DEFAULT + 108)
#define TCPIP_PRIORITY     (SCHED_PRIORITY_DEFAULT + 107)
#define DHCPD_PRIORITY     (SCHED_PRIORITY_DEFAULT + 107)
#define NETAPP_PRIORITY    (SCHED_PRIORITY_DEFAULT + 107)

#define NETAPP_PRIORITY_1  (SCHED_PRIORITY_DEFAULT + 107)
#define NETAPP_PRIORITY_2  (SCHED_PRIORITY_DEFAULT + 107)
#define NETMGR_PRIORITY    (SCHED_PRIORITY_DEFAULT + 107)
#define VDO_ENC_PRIO       (SCHED_PRIORITY_DEFAULT + 107)
#define MJPG_STREAMER_PRIO (SCHED_PRIORITY_DEFAULT + 107)
#define TASK_END_PRIO      (SCHED_PRIORITY_DEFAULT + 107)  //207


/*============Console setting===================*/
#define hal_print(fmt, ...) do {printf(fmt, ##__VA_ARGS__); FFLUSH(0);}while(0)
#define hal_putchar(ch) do {putchar((char)ch);}while(0) 

#define FFLUSH(x) do {fflush(stdout);}while(0) 

extern u8 hal_getchar(void);

#define TICK_RATE_MS 10

/*============Compiler setting===================*/
#define ARM_ADS
#undef STRUCT_PACKED

#define STRUCT_PACKED __attribute__ ((packed))
#define UNION_PACKED __attribute__ ((packed))

//LWIP PACK Definition
#define PACK_STRUCT_BEGIN   
#define PACK_STRUCT_END     
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
#define TMR_TASK_STACK_SIZE  64  //32 + 16 
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128  //64  + 32
#define TCPIP_STACK_SIZE     144 //128 + 16
#define DHCPD_STACK_SIZE     96  //64  + 32
#define NETAPP1_STACK_SIZE   144 //128 + 16
#define NETAPP2_STACK_SIZE   144 //128 + 16
#define NETAPP3_STACK_SIZE   144 //128 + 16
#define NETMGR_STACK_SIZE    96 //64  + 32
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
                          
#endif
