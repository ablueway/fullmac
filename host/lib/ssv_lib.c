/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#define SSV_LIB_C

#include <hw/config.h>
#include <host_config.h>
#include <stdarg.h>
#include <hdr80211.h>
#include <ssv_drv.h>
#include <log.h>
#include <ssv_common.h>
#if (AP_MODE_ENABLE == 1)
#include <ap_info.h>
#endif
#include "dev.h"
#include <ssv_hal.h>
#include "hctrl.h"
#include "ssv_dev.h"
#include <core/recover.h>
#if (CONFIG_HOST_PLATFORM == 0)
#include <uart/drv_uart.h>
#endif

#include "ssv_lib.h"
#include <ssv_timer.h>

#include "efuse.h"

#define CONSOLE_UART   SSV6XXX_UART0

// return  -1 : fail
//        >=0 : ok
extern struct Host_cfg g_host_cfg;
LIB_APIs size_t ssv6xxx_strlen(const char *s);

void llist_head_init(struct ssv_llist_head *lhd)
{
    lhd->last = NULL;
    lhd->list = NULL;
    lhd->llen = 0;
}

struct ssv_llist *llist_pop(struct ssv_llist_head *lhd)
{
    struct ssv_llist *elm = lhd->list;
    if (elm != NULL)
    {
        lhd->list = elm->next;
        elm->next = NULL;
        lhd->llen--;
        return elm;
    }
    else
        return NULL;

}

void llist_push(struct ssv_llist_head *lhd, struct ssv_llist *new)
{
    if(lhd->list == NULL)
        lhd->list = new;
    else
        lhd->last->next = new;

    new->next = NULL;
    lhd->last = new;
    lhd->llen++;
}

u32 llist_l_len(struct ssv_llist_head *lhd)
{
    return lhd->llen;
}

struct ssv_llist *llist_pop_safe(struct ssv_llist_head *lhd, OsMutex *pmtx)
{
    struct ssv_llist *_list = NULL;
    OS_MutexLock(*pmtx);
    _list = llist_pop(lhd);
    OS_MutexUnLock(*pmtx);
    return _list;
}

void llist_push_safe(struct ssv_llist_head *lhd, struct ssv_llist *new, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    llist_push(lhd, new);
    OS_MutexUnLock(*pmtx);
}

u32 llist_l_len_safe(struct ssv_llist_head *lhd, OsMutex *pmtx)
{
    u32 len = 0;
    OS_MutexLock(*pmtx);
    len = llist_l_len(lhd);
    OS_MutexUnLock(*pmtx);
    return len;
}

void list_q_init(struct ssv_list_q *qhd)
{
    qhd->prev = (struct ssv_list_q *)qhd;
    qhd->next = (struct ssv_list_q *)qhd;
    qhd->qlen = 0;
}

void list_q_qtail(struct ssv_list_q *qhd, struct ssv_list_q *newq)
{
    struct ssv_list_q *next = qhd;
    struct ssv_list_q *prev = qhd->prev;

    newq->next = next;
    newq->prev = prev;
    next->prev = newq;
    prev->next = newq;
    qhd->qlen++;
}

void list_q_insert(struct ssv_list_q *qhd, struct ssv_list_q *prev, struct ssv_list_q *newq)
{
    struct ssv_list_q *next = prev->next;

    newq->next = next;
    newq->prev = prev;
    next->prev = newq;
    prev->next = newq;
    qhd->qlen++;
}

void list_q_remove(struct ssv_list_q *qhd,struct ssv_list_q *curt)
{
    struct ssv_list_q *next = curt->next;
    struct ssv_list_q *prev = curt->prev;

    prev->next = next;
    next->prev = prev;
    qhd->qlen--;
}

struct ssv_list_q *list_q_deq(struct ssv_list_q *qhd)
{
    struct ssv_list_q *next, *prev;
    struct ssv_list_q *elm = qhd->next;

    if((qhd->qlen > 0) && (elm != NULL))
    {
        qhd->qlen--;
        next        = elm->next;
        prev        = elm->prev;
        elm->next   = NULL;
        elm->prev   = NULL;
        next->prev  = prev;
        prev->next  = next;

        return elm;
    }else{
        return NULL;
    }
}
unsigned int list_q_len(struct ssv_list_q *qhd)
{
    return qhd->qlen;
}

u32 list_q_len_safe(struct ssv_list_q *q, OsMutex *pmtx)
{
    u32 len = 0;
    OS_MutexLock(*pmtx);
    len = q->qlen;
    OS_MutexUnLock(*pmtx);
    return len;
}

void list_q_qtail_safe(struct ssv_list_q *qhd, struct ssv_list_q *newq, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_qtail(qhd, newq);
    OS_MutexUnLock(*pmtx);
}

struct ssv_list_q *list_q_deq_safe(struct ssv_list_q *qhd, OsMutex *pmtx)
{
    struct ssv_list_q *_list = NULL;
    OS_MutexLock(*pmtx);
    _list = list_q_deq(qhd);
    OS_MutexUnLock(*pmtx);
    return _list;
}

void list_q_insert_safe(struct ssv_list_q *qhd, struct ssv_list_q *prev, struct ssv_list_q *newq, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_insert(qhd, prev, newq);
    OS_MutexUnLock(*pmtx);
}

void list_q_remove_safe(struct ssv_list_q *qhd,struct ssv_list_q *curt, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_remove(qhd, curt);
    OS_MutexUnLock(*pmtx);
}

LIB_APIs s32 ssv6xxx_strrpos(const char *str, char delimiter)
{
	const char *p;

	for (p = (str + ssv6xxx_strlen(str)) - 1; (s32)p>=(s32)str; p--)
	{
		if (*p == delimiter)
			return ((s32)p - (s32)str);
	}

	return -1;	// find no matching delimiter

}

LIB_APIs s32	ssv6xxx_isalpha(s32 c)
{
	if (('A' <= c) && (c <= 'Z'))
		return 1;
	if (('a' <= c) && (c <= 'z'))
		return 1;
	return 0;
}

LIB_APIs s32 ssv6xxx_str_toupper(char *s)
{
	while (*s)
	{
		*s = ssv6xxx_toupper(*s);
		s++;
	}
	return 0;
}

LIB_APIs s32 ssv6xxx_str_tolower(char *s)
{
	while (*s)
	{
		*s = ssv6xxx_tolower(*s);
		s++;
	}
	return 0;
}

LIB_APIs u32 ssv6xxx_atoi_base( const char *s, u32 base )
{
    u32  idx, upbound=base-1;
    u32  value = 0, v;

    while( (v = (u8)*s) != 0 ) {
        idx = v - '0';
        if ( idx > 10 && base==16 ) {
            idx = (v >= 'a') ? (v - 'a') : (v - 'A');
            idx += 10;
        }
        if ( idx > upbound )
            break;
        value = value * base + idx;
        s++;
    }

    return value;
}

LIB_APIs s32 ssv6xxx_atoi( const char *s )
{
    u32 neg=0, value, base=10;

    if ( *s=='0' ) {
        switch (*++s) {
        case 'x':
        case 'X': base = 16; break;
        case 'b':
        case 'B': base = 2; break;
        default: return 0;
        }
        s++;
    }
    else if ( *s=='-' ) {
        neg = 1;
        s++;
    }

    value = ssv6xxx_atoi_base(s, base);

    if ( neg==1 )
        return -(s32)value;
    return (s32)value;

}


#if (CONFIG_HOST_PLATFORM == 1)
u64 ssv6xxx_64atoi( char *s )
{
    u8 bchar='A', idx, upbound=9;
    u32 neg=0, value=0, base=10;

    if ( *s=='0' ) {
        switch (*++s) {
                case 'x': bchar = 'a';
                case 'X': base = 16; upbound = 15; break;
                case 'b':
                case 'B': base = 2; upbound = 1; break;
                default: return 0;
        }
        s++;
    }
    else if ( *s=='-' ) {
        neg = 1;
        s++;
    }

    while( *s ) {
        idx = (u8)*s - '0';
        if ( base==16 && (*s>=bchar) && (*s<=(bchar+5)) )
        {
                idx = (u8)10 + (u8)*s - bchar;
        }
        if ( idx > upbound )
        {
                break;
        }
        value = value * base + idx;
        s++;
    }

    if ( neg==1 )
        return -(s32)value;
    return (u64)value;

}
#endif




LIB_APIs char ssv6xxx_toupper(char ch)
{
	if (('a' <= ch) && (ch <= 'z'))
		return ('A' + (ch - 'a'));

	// else, make the original ch unchanged
	return ch;
}

LIB_APIs char ssv6xxx_tolower(char ch)
{
	if (('A' <= ch) && (ch <= 'Z'))
		return ('a' + (ch - 'A'));

	// else, make the original ch unchanged
	return ch;
}

LIB_APIs s32 ssv6xxx_isupper(char ch)
{
    return (ch >= 'A' && ch <= 'Z');
}

LIB_APIs s32 ssv6xxx_strcmp( const char *s0, const char *s1 )
{
    s32 c1, c2;

    do {
        c1 = (u8) *s0++;
        c2 = (u8) *s1++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}

LIB_APIs s32 ssv6xxx_strncmp(const char *s1, const char *s2, size_t n)
{
  if ( !n )
      return(0);

  while (--n && *s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return( *s1 - *s2 );
}

LIB_APIs char *ssv6xxx_strcat(char *s, const char *append)
{
    char *save = s;

    while (*s) { s++; }
    while ((*s++ = *append++) != 0) { }
    return(save);
}

LIB_APIs char *ssv6xxx_strncat(char *s, const char *append, size_t n)
{
     char* save = s;
     while(*s){ s++; }
     while((n--)&&((*s++ = *append++) != 0)){}
     *s='\0';
     return(save);
}

/*Not considering the case of memory overlap*/
LIB_APIs char *ssv6xxx_strcpy(char *dst, const char *src)
{
    char *ret = dst;
    assert(dst != NULL);
    assert(src != NULL);


    while((* dst++ = * src++) != '\0')
        ;
    return ret;
}

LIB_APIs char *ssv6xxx_strncpy(char *dst, const char *src, size_t n)
{
    register char *d = dst;
    register const char *s = src;

    if (n != 0) {
        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                *d++ = 0;
                break;
            }
        } while (--n != 0);
    }
    return (dst);
}


LIB_APIs size_t ssv6xxx_strlen(const char *s)
{
    const char *ptr = s;
    while (*ptr) ptr++;
    return (size_t)ptr-(size_t)s;
}

LIB_APIs char *ssv6xxx_strchr(const char * s, char c)
{
    const char *p = s;
    while(*p != c && *p)
        p++;
    return (*p=='\0' ? NULL : (char *)p);
}

LIB_APIs void *ssv6xxx_memset(void *s, s32 c, size_t n)
{
    if ( NULL != s ) {
		u8 * ps= (u8 *)s;
		const u8 * pes= ps+n;
        while( ps != pes )
			*(ps++) = (u8) c;
    }
    return s;
}


LIB_APIs void *ssv6xxx_memcpy(void *dest, const void *src, size_t n)
{
    u8 *d = dest;
    const u8 *s = src;

    while (n-- > 0)
      *d++ = *s++;
    return dest;
}


LIB_APIs s32 ssv6xxx_memcmp(const void *s1, const void *s2, size_t n)
{
    const u8 *u1 = (const u8 *)s1, *u2 = (const u8 *)s2;

    while (n--) {
        s32 d = *u1++ - *u2++;
        if (d != 0)
            return d;
    }
    /*
    for ( ; n-- ; s1++, s2++) {
        u1 = *(u8 *)s1;
        u2 = *(u8 *)s2;
        if (u1 != u2)
            return (u1-u2);
    } */
    return 0;
}

#if 0


//extern s32 gOsInFatal;
LIB_APIs void fatal_printf(const char *format, ...)
{

#if 0
   va_list args;


//   gOsInFatal = 1;
   /*lint -save -e530 */
   va_start( args, format );
   /*lint -restore */
   ret = print( 0, 0, format, args );
   va_end(args);
#endif
//    printf(format, ...);


}
#endif

#if 0
LIB_APIs void ssv6xxx_printf(const char *format, ...)
{
   va_list args;

   /*lint -save -e530 */
   va_start( args, format );
   /*lint -restore */

    printf(format, args);
//   ret = print( 0, 0, format, args );
   va_end(args);
}


LIB_APIs void ssv6xxx_snprintf(char *out, size_t size, const char *format, ...)
{
#if 0
    va_list args;
    s32     ret;
    /*lint -save -e530 */
    va_start( args, format ); /*lint -restore */
    ret = print( out, (out + size - 1), format, args );
    va_end(args);
#endif
}

LIB_APIs void ssv6xxx_vsnprintf(char *out, size_t size, const char *format, va_list args)
{
#if 0
	return print( out, (out + size - 1), format, args );
#endif
}
#endif

//LIB_APIs s32 putstr (const char *s, size_t len)
//{
//    return  printstr(0, 0, s, len);
//}


//LIB_APIs s32 snputstr (char *out, size_t size, const char *s, size_t len)
//{
//    return  printstr( &out, (out + size - 1), s, len);
//}


//#endif


#if (CLI_ENABLE==1 && CONFIG_HOST_PLATFORM==0)
LIB_APIs s32 kbhit(void)
{
    return drv_uart_rx_ready(CONSOLE_UART);
}


LIB_APIs s32 getch(void)
{
    return drv_uart_rx(CONSOLE_UART);
}


LIB_APIs s32 putchar(s32 ch)
{
    return drv_uart_tx(CONSOLE_UART, ch);
}
#endif


#if 0
LIB_APIs void ssv6xxx_raw_dump(u8 *data, s32 len)
{
	ssv6xxx_raw_dump_ex(data, len, true, 10, 10, 16, LOG_LEVEL_ON, LOG_MODULE_EMPTY);
	return;
}


LIB_APIs bool ssv6xxx_raw_dump_ex(u8 *data, s32 len, bool with_addr, u8 addr_radix, s8 line_cols, u8 radix, u32 log_level, u32 log_module)
{
    s32 i;

	// check input parameters
	if ((addr_radix != 10) && (addr_radix != 16))
	{
		LOG_ERROR("%s(): invalid value 'addr_radix' = %d\n\r", __FUNCTION__, addr_radix);
		return false;
	}
	if ((line_cols != 8) && (line_cols != 10) && (line_cols != 16) && (line_cols != -1))
	{
		LOG_ERROR("%s(): invalid value 'line_cols' = %d\n\r", __FUNCTION__, line_cols);
		return false;
	}
	if ((radix != 10) && (radix != 16))
	{
		LOG_ERROR("%s(): invalid value 'radix' = %d\n\r", __FUNCTION__, radix);
		return false;
	}

	if (len == 0)	return true;

	// if ONLY have one line
	if (line_cols == -1)
	{
		LOG_TAG_SUPPRESS_ON();
		// only print addr heading at one time
		if ((with_addr == true))
		{
			if      (addr_radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%08d: ", 0);
			else if (addr_radix == 16)	LOG_PRINTF_LM(log_level, log_module, "0x%08x: ", 0);
		}

		for (i=0; i<len; i++)
		{
			// print data
			if	    (radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%4d ",  data[i]);
			else if (radix == 16)	LOG_PRINTF_LM(log_level, log_module, "%02x ", data[i]);
		}
		LOG_PRINTF_LM(log_level, log_module, "\n\r");
		LOG_TAG_SUPPRESS_OFF();
		return true;
	}

	// normal case
	LOG_TAG_SUPPRESS_ON();
    for (i=0; i<len; i++)
	{
		// print addr heading
		if ((with_addr == true) && (i % line_cols) == 0)
		{
			if      (addr_radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%08d: ", i);
			else if (addr_radix == 16)	LOG_PRINTF_LM(log_level, log_module, "0x%08x: ", i);
		}
		// print data
		if	    (radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%4d ",  data[i]);
		else if (radix == 16)	LOG_PRINTF_LM(log_level, log_module, "%02x ", data[i]);
		// print newline
        if (((i+1) % line_cols) == 0)
            LOG_PRINTF_LM(log_level, log_module, "\n\r");
    }
    LOG_PRINTF_LM(log_level, log_module, "\n\r");
	LOG_TAG_SUPPRESS_OFF();
	return true;
}
#endif

#define ONE_RAW 16
void _packetdump(const char *title, const u8 *buf,
                             size_t len)
{
    size_t i;
    LOG_DEBUGF(LOG_L2_DATA, ("%s - hexdump(len=%d):\r\n    ", title, len));



    if (buf == NULL) {
        LOG_DEBUGF(LOG_L2_DATA, (" [NULL]"));
    }else{


        for (i = 0; i < ONE_RAW; i++)
            LOG_DEBUGF(LOG_L2_DATA, ("%02X ", i));

        LOG_DEBUGF(LOG_L2_DATA,("\r\n---\r\n00|"));


        for (i = 0; i < len; i++){
            LOG_DEBUGF(LOG_L2_DATA,(" %02x", buf[i]));
            if((i+1)%ONE_RAW ==0)
                LOG_DEBUGF(LOG_L2_DATA,("\r\n%02x|", (i+1)));
        }
    }
    LOG_DEBUGF(LOG_L2_DATA,("\r\n-----------------------------\r\n"));
}



void pkt_dump_txinfo(void *p)
{
    ssv_hal_dump_txinfo(p);
	return;
}

void pkt_dump_rxinfo(void *p)
{
    ssv_hal_dump_rxinfo(p);
	return;
}


LIB_APIs void hex_dump (const void *addr, u32 size)
{
    u32 i, j;
    const u32 *data = (const u32 *)addr;

/* TODO: aaron */
//#if (__SSV_UNIX_SIM__)
#if (__SSV_UNIX_SIM__ == 1)
	LOG_TAG_SUPPRESS_ON();
#endif
    LOG_PRINTF("        ");
    for (i = 0; i < 8; i++)
        LOG_PRINTF("       %02X", i*sizeof(u32));

    LOG_PRINTF("\r\n--------");
    for (i = 0; i < 8; i++)
        LOG_PRINTF("+--------");

    for (i = 0; i < size; i+= 8)
    {
        LOG_PRINTF("\r\n%08X:%08X", (s32)data, data[0]);
        for (j = 1; j < 8; j++)
        {
            LOG_PRINTF(" %08X", data[j]);
        }
        data = &data[8];
    }
    LOG_PRINTF("\r\n");

/* TODO: aaron */
//#if (__SSV_UNIX_SIM__)
#if (__SSV_UNIX_SIM__ == 1)
    LOG_TAG_SUPPRESS_OFF();
#endif
    return;
}

#if 0 //def __linux__

/* the halt() already implement in arch/x86/include/asm/paravirt.h */

#else
LIB_APIs void ssv_halt(void)
{    
	while (1) ;
}

#endif

extern s32 _ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len, bool blocking,const bool mutexLock);

#define CHIP_ID_SSV6051Q_OLD 0x70000000
#define CHIP_ID_SSV6051Z 0x71000000
#define CHIP_ID_SSV6052Q 0x72000000
#define CHIP_ID_SSV6051Q 0x73000000
#define CHIP_ID_SSV6060P 0x74000000
#define CHIP_ID_SSV6051P 0x75000000
#define CHIP_ID_SSV6030P 0x76000000
//Check efuse chip id. 
ssv6xxx_result check_efuse_chip_id(void)
{
    u32 efuse_chip_id;

    efuse_chip_id = read_chip_id();
    if(
#if (CONFIG_CHIP_ID == SSV6030P)
       (efuse_chip_id == CHIP_ID_SSV6030P) ||
#elif (CONFIG_CHIP_ID == SSV6051Z)
       (efuse_chip_id == CHIP_ID_SSV6051Z) ||
#elif (CONFIG_CHIP_ID == SSV6052Q)
       (efuse_chip_id == CHIP_ID_SSV6052Q) ||
#elif ((CONFIG_CHIP_ID == SSV6006B)||(CONFIG_CHIP_ID==SSV6006C))
        (1)||
#else
# error "Please redefine CONFIG_CHIP_ID!!"
#endif
       (efuse_chip_id == 0))
    {
		LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);    
        return SSV6XXX_SUCCESS;
    }
    else
    {
        LOG_PRINTF("\33[31m[Error] Wi-fi CHIP ID mismatch iComm %s!\33[0m\r\n",ssv_hal_get_chip_name());
        //while(1);
        return SSV6XXX_WRONG_CHIP_ID;
    }
}
extern const char *ssv_version;
extern const char *ssv_date;
extern const char *rlsversion;

int ssv6xxx_init_mac(void* vif)
{
    ssv_vif *vf = (ssv_vif *)vif;
    if (gDeviceInfo->recovering != TRUE)
    {
    	LOG_DEBUG("\33[35mRELEASE VERSION: %s SW VERSION: %s\r\nBUILD DATE: %s\33[0m\r\n",
			rlsversion,ssv_version, ssv_date);
    }
    return ssv_hal_init_mac(vf->self_mac);
}

int ssv6xxx_init_sta_mac(u32 wifi_mode)
{
    return ssv_hal_init_sta_mac(wifi_mode);
}

int ssv6xxx_init_ap_mac(void* vif)
{
#if (AP_MODE_ENABLE == 1)
    ssv_vif* vf=(ssv_vif*)vif;
    ssv6xxx_sec_type sec_type=gDeviceInfo->APInfo->sec_type;

    if(0==ssv_hal_init_ap_mac(vf->self_mac,gDeviceInfo->APInfo->nCurrentChannel))
    {
        //if((sec_type==SSV6XXX_SEC_WEP_40)||(sec_type==SSV6XXX_SEC_WEP_104))
        //{
        //    ssv_hal_ap_wep_setting(sec_type,gDeviceInfo->APInfo->password,vf->idx);
        //}
        return 0;
    }
    else
    {
        return -1;
    }
#else
    return -1;
#endif
}

int ssv6xxx_promiscuous_enable(void)
{
   return ssv_hal_promiscuous_enable();
}

int ssv6xxx_promiscuous_disable(void)
{
   return ssv_hal_promiscuous_disable();
}


u32 g_hw_enable = false;
extern void check_watchdog_timer(void *data1, void *data2);
extern struct task_info_st g_host_task_info[];
extern s32 g_watchdog_check_time;
extern void timer_sta_reorder_release(void* data1, void* data2);

void ssv6xxx_HW_disable(void)
{
	g_hw_enable = FALSE;
#if(RECOVER_ENABLE == 1)
   ssv_hal_watchdog_disable();
#if(RECOVER_MECHANISM == 1)
    os_cancel_timer(check_watchdog_timer,(u32)NULL,(u32)NULL);
#endif //#if(RECOVER_MECHANISM == 1)
#else
	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
	ssv_hal_watchdog_disable();
#endif //#if(RECOVER_ENABLE == 1)
    //os_cancel_timer(timer_sta_reorder_release,(u32)NULL,(u32)NULL);

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);

    ssv6xxx_drv_irq_disable(false);

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    ssv_hal_rf_disable();

    //Disable MCU
    ssv_hal_mcu_disable();

	LOG_PRINTF("%s()at line(%d)\n",__FUNCTION__,__LINE__);
    return;
}
void ssv6xxx_HW_enable(void)
{
    ssv_hal_rf_enable();

    //Enable MCU, it's duplicate, load fw has already set this bit
    ssv_hal_mcu_enable();

    ssv6xxx_drv_irq_enable(false);
#if(RECOVER_ENABLE == 1)
    ssv_hal_watchdog_enable();
#if(RECOVER_MECHANISM == 1)
    os_create_timer(IPC_CHECK_TIMER, check_watchdog_timer, NULL, NULL, (void*)TIMEOUT_TASK);
#endif //#if(RECOVER_MECHANISM == 1)
#endif //#if(RECOVER_ENABLE == 1)
    //os_create_timer(HT_RX_REORDER_BUF_TIMEOUT,timer_sta_reorder_release,NULL,NULL,(void*)TIMEOUT_TASK);

    g_hw_enable = TRUE;
    return;


}

bool ssv6xxx_download_fw(u8 *bin, u32 len)
{
    if(0==MAC_LOAD_FW(bin,len))
        return TRUE;
    else
        return FALSE;
}

void ssv6xxx_set_wakeup_bb_gpio(bool en)
{
    ssv_hal_set_wakeup_bb_gpio(en);
}

#if(ENABLE_DYNAMIC_RX_SENSITIVE==1)
void ssv6xxx_sta_mode_disconnect(void *Info)
{
    struct StaInfo* SInfo = (struct StaInfo *)Info;
    OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
    SInfo->status=DISCONNECT;
    #if(SW_8023TO80211==1)
    OS_MemSET(SInfo->seq_ctl,0,sizeof(SInfo->seq_ctl));
    #endif
    MEMSET(SInfo->joincfg, 0, sizeof(struct cfg_join_request));
    MEMSET(SInfo->joincfg_backup, 0, sizeof(struct cfg_join_request));
    gDeviceInfo->cci_current_level=0;
    gDeviceInfo->cci_current_gate=0;
    ssv_hal_update_cci_setting(MAX_CCI_SENSITIVE);
    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);

}

void ssv6xxx_sta_mode_connect(void *Info)
{
    struct StaInfo* SInfo = (struct StaInfo *)Info;
    OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
    SInfo->status = CONNECT;
    MEMCPY(SInfo->joincfg, SInfo->joincfg_backup, sizeof(struct cfg_join_request));
    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
}
#endif

