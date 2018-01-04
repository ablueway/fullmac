
#ifndef _OS_PORTING_H_
#define _OS_PORTING_H_
#include "ssv_types.h"
#include "ucos_ii.h"

#define LINUX_SIM   0xFFFF
#define HOST_PLATFORM_SEL LINUX_SIM

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

#define NETAPP_PRIORITY_1  NETAPP_PRIORITY+1
#define NETAPP_PRIORITY_2  NETAPP_PRIORITY+2
#define NETMGR_PRIORITY    NETAPP_PRIORITY+3
#define VDO_ENC_PRIO       NETAPP_PRIORITY+4
#define MJPG_STREAMER_PRIO NETAPP_PRIORITY+5
#define TASK_END_PRIO      NETAPP_PRIORITY+6  //31

//#define TICK_RATE_MS portTICK_RATE_MS
//#define TASK_IDLE_STK_SIZE configMINIMAL_STACK_SIZE


/*============Console setting===================*/
#define PRINTF LOG_PRINTF
#define FFLUSH(x)

 //Changed __linux__ to __SSV_UNIX_SIM__ for CYGWIN compatibility

#define hal_print printf

extern void put_char(char ch);
extern int get_char(void);


#define hal_putchar   put_char
#define hal_getchar   get_char

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
#define MV_PLATFORM
//#define EXT_RX_INT
#define OS_NO_SUPPORT_PROFILING
/* sdio rx int, use card int trigger, pulldown/pullup */
#define SDIO_CARD_INT_TRIGGER

#define CCMRAM  __attribute__((section("ccmram")))


/*============SSV-DRV setting===================*/
#define INTERFACE "sdio"
//#if (defined _WIN32)
//#define	CONFIG_RX_POLL              1
//#define	CONFIG_RX_AUTO_ACK_INT      0
//#define snprintf                    _snprintf
//#else
#define	CONFIG_RX_POLL      1
#define SDRV_INCLUDE_SDIO   1
//#endif

/*============Stack Size (unint: 16bytes)===================*/
#if 1
#define TMR_TASK_STACK_SIZE  32   //32
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   64   //64
#define TCPIP_STACK_SIZE     128  //  128
#define DHCPD_STACK_SIZE     64    // 64
#define NETAPP1_STACK_SIZE   128  // 128
#define NETAPP2_STACK_SIZE   128   //128
#define NETAPP3_STACK_SIZE   0   //144   //128
#define NETMGR_STACK_SIZE    64  //64
#define CLI_TASK_STACK_SIZE  64  //64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   64  // 64
#define WIFI_TX_STACK_SIZE   64  //64
#define PING_THREAD_STACK_SIZE 0 //16 , ping thread doesn't enable now, I set staic size is zero to reduce data size.

#endif

#if 0
#define TMR_TASK_STACK_SIZE  48   //32
#define MLME_TASK_STACK_SIZE 0
#define CMD_ENG_STACK_SIZE   96   //64
#define TCPIP_STACK_SIZE     144  //  128
#define DHCPD_STACK_SIZE     96    // 64
#define NETAPP1_STACK_SIZE   144  // 128
#define NETAPP2_STACK_SIZE   144   //128
#define NETAPP3_STACK_SIZE   0   //144   //128
#define NETMGR_STACK_SIZE    96  //64
#define CLI_TASK_STACK_SIZE  64  //64
#define RX_ISR_STACK_SIZE    0
#define WIFI_RX_STACK_SIZE   96  // 64
#define WIFI_TX_STACK_SIZE   96  //64
#define PING_THREAD_STACK_SIZE 0 //16 , ping thread doesn't enable now, I set staic size is zero to reduce data size.
#endif
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
#ifndef __MTRACE_CHECK__
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void __OS_MemFree( void *m );
#endif//#__MTRACE_CHECK__
OS_APIs void OS_MemSET(void *pdest, u8 byte, u32 size);
OS_APIs void OS_MemCPY(void *pdest, const void *psrc, u32 size);
/*=========================================*/
void platform_ldo_en_pin_init(void);
void platform_ldo_en(bool en);
void platform_read_firmware(void *d,void *s,u32 len);
#endif
