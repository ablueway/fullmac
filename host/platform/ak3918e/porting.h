/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#define _GNU_SOURCE
#define __USE_GNU

#include <ssv_types.h>
#include "akos_api.h"
#include "hal_print.h"
#include <stddef.h>

#define __need_timeval
#include "time.h"

#define  AK3918E  0x21

/*============GP Platform selection===================*/
#define HOST_PLATFORM_SEL AK3918E
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
#define TMR_TASK_PRIORITY  8
#define MLME_TASK_PRIORITY 8
#define CMD_ENG_PRIORITY   8
#define WIFI_RX_PRIORITY   8
#define WIFI_TX_PRIORITY   8
#define TCPIP_PRIORITY     9
#define DHCPD_PRIORITY     9
#define NETAPP_PRIORITY    8//11

#define NETAPP_PRIORITY_1  8//11
#define NETAPP_PRIORITY_2  9
#define NETMGR_PRIORITY    10
#define TASK_END_PRIO      10

#endif

typedef    unsigned char          T_U8;       /* unsigned 8 bit integer */
typedef    unsigned short         T_U16;      /* unsigned 16 bit integer */
typedef    unsigned long          T_U32;      /* unsigned 32 bit integer */
typedef    signed char            T_S8;       /* signed 8 bit integer */
typedef    signed short           T_S16;      /* signed 16 bit integer */
typedef    signed long            T_S32;      /* signed 32 bit integer */
typedef    void                   T_VOID;     /* void */
typedef T_VOID *                T_pVOID;    /* pointer of void data */

#define AK_FALSE                0
#define AK_TRUE                 1
#define AK_NULL                 ((T_pVOID)0)

/*============Console setting===================*/
extern signed long akprintf(unsigned char level, const char * mStr, const char * s, ...);

#define hal_print(...) akprintf(C3, M_DRVSYS, ##__VA_ARGS__)
#define FFLUSH(x)

#define hal_putchar(ch) putch((char)ch) 

extern u8 hal_getchar();

extern int l3_errno;
//#define l3_errno errno
#undef sys_msleep

/*============Compiler setting===================*/
#define ARM_ADS
#define STRUCT_PACKED __attribute__ ((packed))
#define UNION_PACKED __attribute__ ((packed))
//LWIP PACK Definition
#define PACK_STRUCT_BEGIN   //#pragma pack(1)
#define PACK_STRUCT_END     __attribute__((packed))
#define PACK_STRUCT_STRUCT     __attribute__ ((packed))
#define PACK(__Declaration__) __Declaration__ PACK_STRUCT_STRUCT;
#define PACK_STRUCT_FIELD(x)    x
#define ALIGN_ARRAY(a) __attribute__ ((aligned(a)))

#define OS_NO_SUPPORT_PROFILING

/*============SSV-DRV setting===================*/
//#define INTERFACE "sdio"
#define INTERFACE "spi"
#define	CONFIG_RX_POLL      0
#define SDRV_INCLUDE_SDIO    1
#define SDRV_INCLUDE_SPI     1

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  80 //32
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128 //64
#define TCPIP_STACK_SIZE     0
#define DHCPD_STACK_SIZE     0
#define NETAPP1_STACK_SIZE   196 //0 //128
#define NETAPP2_STACK_SIZE   196
#define NETAPP3_STACK_SIZE   0
#define NETMGR_STACK_SIZE    128
#define CLI_TASK_STACK_SIZE  196 //64 //0 //64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   128
#define WIFI_TX_STACK_SIZE   128
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
                          PING_THREAD_STACK_SIZE)
                          
/*============Memory========================*/
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);

/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);
void platform_udelay(u32 us_delay);

#endif
