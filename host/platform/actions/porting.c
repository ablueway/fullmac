/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#include <rtos.h>
#include <porting.h>
#include <ssv_lib.h>

#define APP_INCLUDE_WIFI_FW

int (*act_uart_read)(unsigned char *, int);
//kernel_sym获取act_uart_read函数入口

/*=============console==================*/
u8 hal_getchar(void)
{
    u8 data=0;
    if (act_uart_read == NULL)
    {
        act_uart_read = (int (*)(unsigned char *, int))kernel_sym("act_uart_read");
    }
    if (act_uart_read)
    {
        while (act_uart_read(&data, 1) != 1)
        {
            OS_TickDelay(1);
        }
    }
    else
    {
        hal_print("hal_getchar ERROR\r\n");
    }
    
    return data;
}

/*=============Memory==================*/
OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void * ptr = NULL;
    ptr = kmalloc(size, 0);
    if (ptr) OS_MemSET(ptr,0,size);
    return ptr;
}

/**
 *  We do not recommend using OS_MemFree() API
 *  because we do not want to support memory
 *  management mechanism in embedded system.
 */
OS_APIs void __OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
    kfree(m);
}

void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    ssv6xxx_memcpy(pdest,psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    ssv6xxx_memset(pdest,byte,size);
}

OS_EVENT  *OSQCreate (u8 *start, s16 qsize, s16 msgsize)
{
    OS_EVENT * p_event = os_q_create(start,qsize * msgsize / sizeof( msg_t ));
    if (p_event)
    {
        //hal_print("p_event = %p os_event_type = %d\r\n", p_event, p_event->os_event_type);
    }
    
    return p_event;
}

s8 OSQDel (OS_EVENT *pevent, INT8U opt)
{
    s8 err = 0;
    os_q_del(pevent,opt,&err);
    if (err != OS_NO_ERR)
    {
        hal_print("os_q_del err=%d, pevent = %p, os_event_type = %d\r\n", err, pevent, pevent->os_event_type);
    }
    return err;
}

s8 OSQPost (OS_EVENT *pevent, void *msg)
{
    s8 err = 0;
    msg_t message;
    
    if (pevent == NULL)
    {
        hal_print("OSQPost err=%d, pevent = %p\r\n", err, pevent);
        return OS_ERR_EVENT_TYPE;
    }

    message.msg.pointer = *(unsigned int *)msg;
    err = os_q_post(pevent, &message);
    if ((err != OS_Q_FULL) && (err != OS_NO_ERR))
    {
        hal_print("os_q_post err=%d, pevent = %p, os_event_type = %d\r\n", err, pevent, pevent->os_event_type);
    }
    
    return err;
}

s8  OSQPend (OS_EVENT *pevent, s16 timeout, s8 *recvbuff)
{
    s8 err = 0;
    msg_t message;
    unsigned int value = 0;
    if (pevent == NULL)
    {
        hal_print("OSQPend err=%d, pevent = %p\r\n", err, pevent);
        return OS_ERR_EVENT_TYPE;
    }
    err = os_q_pend(pevent,&message,timeout);
    if (err == 1)
    {
       value = message.msg.pointer;
       ssv6xxx_memcpy(recvbuff, (s8 *)&value, sizeof(unsigned int));
       err = 0;
    }
    else
    {
        if (err != OS_TIMEOUT)
        {
            hal_print("os_q_pend err=%d, pevent = %p os_event_type = %d\r\n", err, pevent, pevent->os_event_type);
        }
    }
    
    return err;    
}

//=====================Platform LDO EN ping setting=======================
//#define SSV_LDO_EN_PIN              IO_E1

void platform_ldo_en_pin_init(void)
{
#ifdef SSV_LDO_EN_PIN
#endif
}

void platform_ldo_en(bool en)
{
#ifdef SSV_LDO_EN_PIN
#endif
}

//=====================find fw to download=======================
#include <firmware/ssv6200_uart_bin.h>
bool platform_download_firmware(void)
{
    //LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin));
    return ssv6xxx_download_fw((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));
}

void platform_read_firmware(void *d,void *s,u32 len)
{
    OS_MemCPY(d,s,len);
}
