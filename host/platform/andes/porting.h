/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/



#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_
#include "hw/types.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

#define LINUX_SIM   0xFFFF
#define HOST_PLATFORM_SEL LINUX_SIM

/*============OS parameter setting===================*/
typedef pdTASK_CODE				    OsTask;
typedef xTaskHandle				    OsTaskHandle;
typedef xSemaphoreHandle			OsMutex;
typedef xSemaphoreHandle			OsSemaphore;
typedef xQueueHandle				OsMsgQ;
typedef xTimerHandle                OsTimer;
typedef tmrTIMER_CALLBACK           OsTimerHandler;

#define OS_TASK_ENTER_CRITICAL()        taskENTER_CRITICAL()
#define OS_TASK_EXIT_CRITICAL()         taskEXIT_CRITICAL()

#define RX_ISR_PRIORITY    3
#define CMD_ENG_PRIORITY        3
#define WIFI_TX_PRIORITY        3
#define WIFI_RX_PRIORITY        3
//#define WIFI_RX_PRIORITY        4

#define TCPIP_PRIORITY          2
#define NETAPP_PRIORITY         2
#define NETAPP_PRIORITY_1         2
#define NETAPP_PRIORITY_2         2
#define DHCPD_PRIORITY            2
#define NETMGR_PRIORITY        2

#define TMR_TASK_PRIORITY         1
#define MLME_TASK_PRIORITY      2
#define TASK_END_PRIO          2

/*============Console setting===================*/
#define PRINTF LOG_PRINTF
#define FFLUSH(x) fflush(x)
 //Changed __linux__ to __SSV_UNIX_SIM__ for CYGWIN compatibility
#ifndef __SSV_UNIX_SIM__
//#include <conio.h>
#define hal_getchar   getchar
#else
#define hal_getchar   getchar
#endif
#define hal_print printf
#define hal_putchar  putchar


/*============Compiler setting===================*/
//#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__));
//#else
//#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__; __pragma( pack(pop) )
//#endif
#undef STRUCT_PACKED
#define STRUCT_PACKED __attribute__ ((packed))
#define UNION_PACKED


/*============SSV-DRV setting===================*/
//#define INTERFACE "sdio"
#define INTERFACE "spi"
//#if (defined _WIN32)
//#define	CONFIG_RX_POLL              1
//#define	CONFIG_RX_AUTO_ACK_INT      0
//#define snprintf                    _snprintf
//#else
#define	CONFIG_RX_POLL      1
#define SDRV_INCLUDE_SDIO   0
#define SDRV_INCLUDE_SPI    1

//#endif

/*============Stack Size (unint: 16bytes)===================*/
#define TMR_TASK_STACK_SIZE  48
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   96
#define TCPIP_STACK_SIZE     128
#define DHCPD_STACK_SIZE     64
#define NETAPP1_STACK_SIZE    128
#define NETAPP2_STACK_SIZE    128
#define NETAPP3_STACK_SIZE    128
#define NETMGR_STACK_SIZE    80
#define CLI_TASK_STACK_SIZE  64
#define RX_ISR_STACK_SIZE    64
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
#endif
