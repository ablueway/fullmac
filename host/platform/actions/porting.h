/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_

#define _STRING_H_
#include <ssv_types.h>
#include <ucos/os.h>
#include <ucos/os_cpu.h>
#include <kmod_calls.h>

#undef __SSV_UNIX_SIM__
#define _SSV_EX_LIB_H_

#define LWIP_TIMEVAL_PRIVATE 0
typedef  os_event_t  OS_EVENT;
typedef  os_sem_data_t OS_SEM_DATA;
typedef  unsigned int   OS_CPU_SR;                /* Define size of CPU status register (PSW = 32 bits) */
typedef  unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */

#define HAVE_TYPE_SSIZE_T

#define OS_Q_COPY_MSG 1

#define abort() 
#define stdout 0
#define OSCnt os_cnt
#define OSTaskCreate os_task_create
#define OSTaskCreateExt os_task_create_ext

#define OSTaskDel os_task_del
#define OSSemCreate os_sem_create
#define OSSemPend os_sem_pend
#define OSSemPost os_sem_post
#define OSSemDel os_sem_del
#define OSSemQuery os_sem_query

extern OS_EVENT  *OSQCreate (u8 *start, s16 qsize, s16 msgsize);
extern s8 OSQDel (OS_EVENT *pevent, INT8U opt);
extern s8 OSQPost (OS_EVENT *pevent, void *msg);
extern s8  OSQPend (OS_EVENT *pevent, s16 timeout, s8 *recvbuff);

#define OSTimeDly os_time_dly
#define OSTimeGet os_time_get

#define OSSchedLock os_sched_lock
#define OSSchedUnlock os_sched_unlock

/* sdio temp buffer from malloc, or from bss */
#define SDIO_MALLOC_BUFFER

#define HOST_PLATFORM_SEL 

#ifndef in_range
#define in_range(c, lo, up)  ((u8)c >= lo && (u8)c <= up)
#endif

#ifndef isprint
#define isprint(c)           in_range(c, 0x20, 0x7f)
#endif

#ifndef  isdigit 
#define isdigit(c)           in_range(c, '0', '9')
#endif

#ifndef isxdigit
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#endif

#ifndef islower
#define islower(c)           in_range(c, 'a', 'z')
#endif

#ifndef isspace
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#endif

/*============Task Priority===================*/
#define RX_ISR_PRIORITY    0 //No use
#define TMR_TASK_PRIORITY  4
#define MLME_TASK_PRIORITY 25
#define CMD_ENG_PRIORITY   5
#define WIFI_RX_PRIORITY   6
#define WIFI_TX_PRIORITY   7
#define TCPIP_PRIORITY     8
#define DHCPD_PRIORITY     26 
#define NETMGR_PRIORITY    27 

#define NETAPP_PRIORITY    9
#define NETAPP_PRIORITY_1  28 
//#define NETAPP_PRIORITY_2  NETAPP_PRIORITY+2
#define TASK_END_PRIO      29//NETAPP_PRIORITY+3

/*============Console setting===================*/
#define hal_print printf
#define FFLUSH(x) //fflush(x)
#define hal_putchar(ch) printf("%c",ch) 
extern u8 hal_getchar();


/*============Compiler setting===================*/
#define ARM_ADS
#define STRUCT_PACKED __attribute__ ((packed))
#define UNION_PACKED __attribute__ ((packed))
//LWIP PACK Definition
#define PACK_STRUCT_BEGIN   //#pragma pack(1)
#define PACK_STRUCT_END     //#pragma pack()
#define PACK_STRUCT_STRUCT     __attribute__ ((packed))
#define PACK(__Declaration__) __Declaration__ PACK_STRUCT_STRUCT;
#define PACK_STRUCT_FIELD(x)    x
#define ALIGN_ARRAY(a) __attribute__ ((aligned(a)))

#define inline __inline

/*============SSV-DRV setting===================*/
#define	CONFIG_RX_POLL      0
#define INTERFACE "sdio"
#define SDRV_INCLUDE_SDIO   1

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  64  //48 + 16 
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   128  //96  + 32
#define TCPIP_STACK_SIZE     144 //128 + 16
#define DHCPD_STACK_SIZE     96  //64  + 32
#define NETAPP1_STACK_SIZE   144 //128 + 16
#define NETAPP2_STACK_SIZE   144 //128 + 16
#define NETAPP3_STACK_SIZE   0 //128 + 16
#define NETMGR_STACK_SIZE    144  //80  + 32
#define CLI_TASK_STACK_SIZE  80
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   96  //64  + 32 
#define WIFI_TX_STACK_SIZE   96  //64  + 32 
#define DEV_INIT_STACK_SIZE  64   //64
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
                          DEV_INIT_STACK_SIZE+\
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
