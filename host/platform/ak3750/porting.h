/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#include <ssv_types.h>

//#include "akdefine.h"
#include "akos_api.h"
//#include "hal_print.h"
//#include "Eng_Debug.h"

#define  AK3760C  0x20

/*============GP Platform selection===================*/
#define HOST_PLATFORM_SEL AK3760C
#define EXT_RX_INT

/*============Task Priority===================*/
#define RX_ISR_PRIORITY    0 //No use for uCOS2
#if 0
#define TMR_TASK_PRIORITY  10
#define MLME_TASK_PRIORITY 10
#define CMD_ENG_PRIORITY   10
#define WIFI_RX_PRIORITY   10
#define WIFI_TX_PRIORITY   10
#define TCPIP_PRIORITY     11
#define DHCPD_PRIORITY     11
#define NETAPP_PRIORITY    11

#define NETAPP_PRIORITY_1  11
#define NETAPP_PRIORITY_2  11
#define NETMGR_PRIORITY    11
#define VDO_ENC_PRIO       11
#define MJPG_STREAMER_PRIO 11
#define TASK_END_PRIO      11
#else
#define TMR_TASK_PRIORITY  9
#define MLME_TASK_PRIORITY 9
#define CMD_ENG_PRIORITY   9
#define WIFI_RX_PRIORITY   10
#define WIFI_TX_PRIORITY   10
#define TCPIP_PRIORITY     11
#define DHCPD_PRIORITY     11
#define NETAPP_PRIORITY    11

#define NETAPP_PRIORITY_1  11
#define NETAPP_PRIORITY_2  11
#define NETAPP_PRIORITY_3  11
#define NETAPP_PRIORITY_4  11
#define NETMGR_PRIORITY    11
#define VDO_ENC_PRIO       11
#define MJPG_STREAMER_PRIO 11
#define TASK_END_PRIO      11

#endif
/*============Console setting===================*/
extern int	ConsolePrint(const char *format, ...);

#define hal_print ConsolePrint
#define FFLUSH(x)

#define hal_putchar(ch) putch((char)ch) 
//#define hal_getchar  getch //getchar 
extern u8 hal_getchar();

//extern u8 hal_getchar(void);


/*============Compiler setting===================*/
#define ARM_ADS
#define PACK( __Declaration__ ) __packed __Declaration__;
#undef STRUCT_PACKED
#define STRUCT_PACKED __packed
#define UNION_PACKED __packed
//LWIP PACK Definition
#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_FIELD(x) __packed x
#define inline __inline
#define ALIGN_ARRAY(a) __align(a)

#define OS_NO_SUPPORT_PROFILING

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

/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);

#endif
