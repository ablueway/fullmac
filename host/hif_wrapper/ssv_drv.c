/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <msgevt.h>
#include <os_wrapper.h>
#include <cmd_def.h>
#include "ssv_drv.h"
#include "ssv_drv_if.h"
//#include <time.h>
#include <ssv_hal.h>
#include "hctrl.h"
#include <txrx_hdl.h>



#define MAX_SSV6XXX_DRV     1
static s16 s_drv_cnt;


//static struct ssv6xxx_drv_ops *s_drv_array[MAX_SSV6XXX_DRV];
///static struct ssv6xxx_drv_ops   *s_drv_cur;

static struct unified_drv_ops *s_drv_array[MAX_SSV6XXX_DRV];
struct ssv_unify_drv *s_drv_cur;


bool ssv6xxx_drv_register(struct unified_drv_ops *ssv_drv);
//bool ssv6xxx_drv_register(struct ssv6xxx_drv_ops *ssv_drv);
bool ssv6xxx_drv_unregister(u16 i);

static bool _ssv6xxx_drv_started = false;
static OsSemaphore ssvdrv_rx_sphr;
u32 drv_trx_time = 0; //ms

OsMutex drvMutex;

//extern struct ssv6xxx_drv_ops g_drv_usb_linux;
extern struct unified_drv_ops usb_ops;

extern struct ssv6xxx_usb_glue *usb_glue;

void CmdEng_RxHdlData(void *frame);

#if CONFIG_STATUS_CHECK
extern u32 g_dump_tx;
extern void _packetdump(const char *title, const u8 *buf, size_t len);
#endif

static bool _ssv6xxx_irq_st = false;

//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
u32 free_tx_page=0;
u32 free_tx_id=0;
extern struct Host_cfg g_host_cfg;

void ssvdrv_rx_isr(void)
{
    OS_SemSignal(&ssvdrv_rx_sphr);
}

bool ssv6xxx_drv_tx_resource_enough(u32 frame_len)
{
    u32 page_count=0;
    u32 rty_cnt=g_host_cfg.tx_retry_cnt;
    bool need_check_tx_src=FALSE;
    bool ret = TRUE;

    page_count=ssv_hal_bytes_to_pages(frame_len);

    do
    {
        OS_MUTEX_LOCK(&txsrcMutex);
        if ((tx_rcs.free_page < page_count) || (tx_rcs.free_id <= 0)|| (tx_rcs.free_space<= 0))
        {
            need_check_tx_src = TRUE;
            ret = FALSE;
        }
        else
	    {
			need_check_tx_src = FALSE;
            tx_rcs.free_page -= page_count;
            tx_rcs.free_id --;
            tx_rcs.free_space--;
            ret = TRUE;
        }
        OS_MUTEX_UNLOCK(&txsrcMutex);

        if(need_check_tx_src)
        {
            if(rty_cnt > 0){
                rty_cnt--;

                //update_tx_resource
                OS_MUTEX_LOCK(&txsrcMutex);
                ssv_hal_get_tx_resources(&tx_rcs.free_page,&tx_rcs.free_id,&tx_rcs.free_space);
                OS_MUTEX_UNLOCK(&txsrcMutex);
            }
            else
            {
                LOG_DEBUGF(LOG_TXRX, ("%s(): free page = %d, free id = %d, pg_cnt = %d, frm len =%d\r\n",
					__FUNCTION__, tx_rcs.free_page, tx_rcs.free_id, page_count, frame_len));        
                ret = FALSE;
                break;
            }
        }
    }while(need_check_tx_src);

    return ret;
}

bool ssv6xxx_drv_wait_tx_resource(u32 frame_len)
{
    u8 times=0;
//    struct ssv6xxx_hci_txq_info *pInfo=NULL;
//    u32 regVal;

    while(FALSE==ssv6xxx_drv_tx_resource_enough(frame_len))
    {
//        PRINTF("wait tx resource\n");
        if(times>=1){
            OS_TickDelay(1);
        }

        //update_tx_resource
        OS_MUTEX_LOCK(&txsrcMutex);
        ssv_hal_get_tx_resources(&tx_rcs.free_page,&tx_rcs.free_id,&tx_rcs.free_space);
        OS_MUTEX_UNLOCK(&txsrcMutex);

        times++;
    }
    return TRUE;
}

#endif//#ifndef __SSV_UNIX_SIM__

u32 ssv6xxx_drv_get_handle(void)
{
	u32 retVal = -1;
	
    if (s_drv_cur == NULL)
	{        
		SDRV_WARN("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return retVal;
	}
    OS_MUTEX_LOCK(drvMutex);
	if (s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
	{
		retVal = 0;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (s_drv_cur->drv_ops.handle != NULL))
	{
    	retVal = s_drv_cur->drv_ops.handle();	
	}
    OS_MUTEX_UNLOCK(drvMutex);
	if (retVal == -1)
	{
		SDRV_WARN("%s() : NO handle() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
	return retVal;
}

bool ssv6xxx_drv_ack_int(void)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{        
		SDRV_WARN("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}
	
    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (s_drv_cur->drv_ops.ack_int != NULL))
	{
    	ret = s_drv_cur->drv_ops.ack_int();	
	}
    OS_MUTEX_UNLOCK(drvMutex);
	if (ret != TRUE)
	{
        SDRV_WARN("%s() : NO ack_int() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->drv_ops.name);

	}
    return ret;
}

bool ssv6xxx_drv_register(struct ssv_unify_drv *ssv_drv)
{
    u16 i;
    bool ret = false;
    if (s_drv_cnt == MAX_SSV6XXX_DRV)
	{        
		SDRV_WARN("%s s_drv_cnt = MAX_SSV6XXX_DRV!\r\n",__FUNCTION__);
		return ret;
	}

    SDRV_TRACE("%s() <= : 0x%08x, %s\r\n", 
		__FUNCTION__, ssv_drv, ssv_drv->name);

    // find empty slot in array
    for (i = 0; i < MAX_SSV6XXX_DRV; i++)
    {
        if (s_drv_array[i] == NULL)
        {
            s_drv_array[i] = ssv_drv;
            s_drv_cnt++;
            SDRV_TRACE("%s() => : ok! s_drv_cnt = %d, i = %d, ssv_drv = (0x%08x, %s)\r\n", 
				__FUNCTION__, s_drv_cnt, i, ssv_drv, ssv_drv->name);

			ret = true;
        }
    }
	return ret;
}

bool ssv6xxx_drv_unregister(u16 i)
{
    if (s_drv_cnt == 0)
    {
        SDRV_WARN("%s() : s_drv_cnt = 0, return true!\r\n", __FUNCTION__);
        return TRUE;
    }
    SDRV_TRACE("%s() <= : i = %d, 0x%08x, %s\r\n", 
		__FUNCTION__, i, s_drv_array[i], s_drv_array[i]->name);

    // find matching slot in array
    s_drv_array[i] = NULL;
    s_drv_cnt--;
    SDRV_TRACE("%s() => : s_drv_cnt = %d\r\n", __FUNCTION__, s_drv_cnt);
    return TRUE;
}

void ssv6xxx_drv_list(void)
{
	u16 i;

    LOG_PRINTF("available in ssv driver module : %d\r\n",__LINE__);
    for (i = 0; i < s_drv_cnt; i++)
    {
        if (s_drv_cur == s_drv_array[i])
        {
            LOG_PRINTF("%-10s : 0x%08x (selected)\r\n", 
				s_drv_array[i]->name, (unsigned int)s_drv_array[i]);
        }
        else
        {
            LOG_PRINTF("%-10s : 0x%08x\r\n", 
				s_drv_array[i]->name, (unsigned int)s_drv_array[i]);
        }
    }
    LOG_PRINTF("%-10s = %d\r\n", "TOTAL", s_drv_cnt);
}

bool ssv6xxx_drv_module_init(void)
{
    SDRV_TRACE("%s() <=\r\n", __FUNCTION__);

    s_drv_cnt = 0;
    MEMSET(s_drv_array, 0x00, MAX_SSV6XXX_DRV * sizeof(struct ssv_unify_drv *));
    s_drv_cur = 0;

    // register each driver
//#if (SDRV_INCLUDE_SIM)
//	ssv6xxx_drv_register(&g_drv_sim);
//#endif
	ssv6xxx_drv_register(&usb_ops);
/* TODO: aaron */
/* in */
#if (SDRV_INCLUDE_SDIO == 1)
    ssv6xxx_drv_register((struct ssv6xxx_drv_ops *)&g_drv_sdio);
#endif
/* not in */
#if (_WIN32 == 1)
/* not in */
#if (SDRV_INCLUDE_UART == 1)
    ssv6xxx_drv_register((struct ssv6xxx_drv_ops *)&g_drv_uart);
#endif
/* not in */
#if (SDRV_INCLUDE_USB == 1)
    ssv6xxx_drv_register((struct ssv6xxx_drv_ops *)&g_drv_usb);
#endif
#endif /* _WIN32 */
/* in */
#if (SDRV_INCLUDE_SPI == 1)
    ssv6xxx_drv_register((struct ssv6xxx_drv_ops *)&g_drv_spi);
#endif

    OS_SemInit(&ssvdrv_rx_sphr, 256, 0);

    if (OS_SUCCESS != OS_MutexInit(&drvMutex))
	{
		return FALSE;
    }
	if (OS_SUCCESS != OS_MutexInit(&txsrcMutex))
	{        
		return FALSE;
	}
    return TRUE;
}

// ssv_drv module release
void ssv6xxx_drv_module_release(void)
{
    s16 i, tmp;

    SDRV_TRACE("%s() <= : s_drv_cnt = %d\r\n", __FUNCTION__, s_drv_cnt);

    // close each driver & unregister
    tmp = s_drv_cnt;
    for (i = 0; i < tmp; i++)
    {
        SDRV_TRACE("s_drv_array[%d] = 0x%08x\r\n", i, (unsigned int)s_drv_array[i]);

		if (s_drv_array[i] != NULL)
		{
			if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
			{
				SDRV_WARN("linux drv nonsuppoot close callback, drv_idx(%d)\n", i);
			}
			else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (s_drv_array[i]->drv_ops.close != NULL))
			{
				s_drv_array[i]->drv_ops.close();		
			}

	        if (!ssv6xxx_drv_unregister((u16)i))
			{
				SDRV_WARN("ssv6xxx_drv_unregister(%d) fail!\r\n", i);
			}			
		}
    }
    s_drv_cur = NULL;
    OS_MutexDelete(drvMutex);
    OS_MutexDelete(txsrcMutex);
    SDRV_TRACE("%s() =>\r\n", __FUNCTION__);
}

bool ssv6xxx_drv_select(char name[UNIFY_DRV_NAME_MAX_LEN], union unify_drv_info drv_info)
{
    u16 i;
	struct ssv_unify_drv *drv_target = NULL;

    SDRV_TRACE("%s() <= : name = %s, s_drv_cnt = %d, s_drv_cur = (0x%08x)\r\n",
		__FUNCTION__, name, s_drv_cnt, (unsigned int)s_drv_cur);

	/* s_drv_cnt = 2 */
    if (s_drv_cnt == 0)
	{	
		SDRV_FAIL("%s s_drv_cnt = 0\r\n",__FUNCTION__);
		return FALSE;
	}
    // find the matching ssv_drv
    for (i = 0; i < s_drv_cnt; i++)
    {
		if ((drv_info.fields.os_type == s_drv_array[i]->drv_info.fields.os_type) 				||
			(drv_info.fields.register_type == s_drv_array[i]->drv_info.fields.register_type) 	||
			(drv_info.fields.hw_type == s_drv_array[i]->drv_info.fields.hw_type))
		{
            drv_target = s_drv_array[i];
            break;
		}
#if 0	
    	/* name = "spi" */
        if (STRCMP(name, s_drv_array[i]->name) == 0)
        {
            drv_target = s_drv_array[i];
            break;
        }
#endif
    }

    if (drv_target == NULL)
    {
	    LOG_PRINTF("ssv driver '%s' is NOT available now!\r\n", name);
        return FALSE;
    }
    if (drv_target == s_drv_cur)
    {
	    LOG_PRINTF("ssv drv '%s' is already in selection.\r\n", drv_target->name);
		return TRUE;
    }

	if ((drv_target->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		SDRV_WARN("linux drv nonsuppoot open callback\n");
//		ret = TRUE;
	}
	else if ((drv_target->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (drv_target->drv_ops.open != NULL))
	{
		if (drv_target->drv_ops.open() == false)	
		{
			SDRV_FAIL("open() fail! in s_drv_cur (0x%08x, %s)\r\n", 
					(unsigned int)drv_target, drv_target->name);
			return FALSE;
		}
	}

	if ((drv_target->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		SDRV_WARN("linux drv nonsuppoot init callback\n");
//		ret = TRUE;
	}
	else if ((drv_target->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (drv_target->drv_ops.init != NULL))
	{
		if (drv_target->drv_ops.init() == false)
    	{
			SDRV_FAIL("init() fail! in s_drv_cur (0x%08x, %s)\r\n", 
						(unsigned int)drv_target, drv_target->name);
	        return FALSE;
		}
	}

    s_drv_cur = drv_target;
    LOG_PRINTF("select drv -> %-10s : 0x%08x\r\n", 
		s_drv_cur->name, (unsigned int)s_drv_cur);
    return TRUE;
}

bool ssv6xxx_drv_open(void)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		SDRV_WARN("linux drv nonsuppoot open callback\n");
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (s_drv_cur->drv_ops.open != NULL))
	{
		ret = s_drv_cur->drv_ops.open();
	}
    OS_MUTEX_UNLOCK(drvMutex);

    return ret;
}

bool ssv6xxx_drv_close(void)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		SDRV_WARN("linux drv nonsuppoot close callback\n");
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (s_drv_cur->drv_ops.close != NULL))
	{
		ret = s_drv_cur->drv_ops.close();
	}
    OS_MUTEX_UNLOCK(drvMutex);

    return ret;
}


bool ssv6xxx_drv_init(void)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		/*TODO(aaron): need offer callback to notify rx task handle rx data */		
//		s_drv_cur->drv_ops.hwif_rx_task(usb_glue, NULL, NULL);		
		SDRV_WARN("linux drv nonsuppoot init callback\n");
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (s_drv_cur->drv_ops.init != NULL))
	{
		ret = s_drv_cur->drv_ops.init();
	}

    return ret;
}

s32 ssv6xxx_drv_recv(u8 *dat, size_t len)
{
    s32 retVal = -1;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return retVal;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
									&& (s_drv_cur->drv_ops.read != NULL))
	{
		/* TODO(aaron): fix the rx data path */
		retVal = s_drv_cur->drv_ops.read(usb_glue, dat, len);
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.recv != NULL))
	{
		retVal = s_drv_cur->drv_ops.recv(dat, len);
	}

    if (retVal > 0)
	{
	    drv_trx_time = os_tick2ms(OS_GetSysTick());
	}
    OS_MUTEX_UNLOCK(drvMutex);
	
    return retVal;
}

bool ssv6xxx_drv_get_name(char name[32])
{
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return FALSE;
	}

	STRCPY(name, s_drv_cur->name);
    return TRUE;
}

bool ssv6xxx_drv_ioctl(u32 ctl_code, void *in_buf, size_t in_size,
				void *out_buf, size_t out_size, size_t *bytes_ret)
{
    bool ret = FALSE;
	
    if (s_drv_cur == NULL)
	{        
		SDRV_WARN("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}
	
    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
		SDRV_WARN("%s linux non-support ioctol callback \n");
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (s_drv_cur->drv_ops.ioctl != NULL))
	{
    	ret = s_drv_cur->drv_ops.ioctl(ctl_code, in_buf, in_size, out_buf, out_size, bytes_ret);	
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret != TRUE)
	{
        SDRV_WARN("%s() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->drv_ops.name);

	}
    return ret;
}


s32 ssv6xxx_drv_send(void *dat, size_t len)
{
    s32 retVal = -1;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return retVal;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
									&& (s_drv_cur->drv_ops.write != NULL))
	{
		/* TODO(aaron): fix the tx data path */
		retVal = s_drv_cur->drv_ops.write(usb_glue, dat, len, 0x0);	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.send != NULL))
	{
		retVal = s_drv_cur->drv_ops.send(dat, len);
	}

    if (retVal > 0)
	{
	    drv_trx_time = os_tick2ms(OS_GetSysTick());
	}
    OS_MUTEX_UNLOCK(drvMutex);
	
    return retVal;
}

u32 ssv6xxx_drv_read_reg(u32 addr)
{
    s32 retVal = -1;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return retVal;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
									&& (s_drv_cur->drv_ops.readreg != NULL))
	{
		s_drv_cur->drv_ops.readreg(usb_glue, addr, &retVal);	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.read_reg != NULL))
	{
    	retVal = s_drv_cur->drv_ops.read_reg(addr);
	}
    OS_MUTEX_UNLOCK(drvMutex);


	if (retVal == -1)
	{
		SDRV_WARN("%s() : NO read_reg() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
	
    return retVal;
}

bool ssv6xxx_drv_write_reg(u32 addr, u32 data)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
	    SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
									&& (s_drv_cur->drv_ops.writereg != NULL))
	{
    	ret = s_drv_cur->drv_ops.writereg(usb_glue, addr, data);	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.write_reg != NULL))
	{
		ret = s_drv_cur->drv_ops.write_reg(addr, data);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}	
    return ret;
}

bool ssv6xxx_drv_set_reg(u32 _REG_,u32 _VAL_,u32 _SHIFT_, u32 _MASK_)
{
    u32 regVal = 0;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return FALSE;
	}

    regVal = ssv6xxx_drv_read_reg(_REG_);
    regVal =((((_VAL_) << _SHIFT_) & ~_MASK_) | (regVal & _MASK_));

	return ssv6xxx_drv_write_reg(_REG_, regVal);
}
bool ssv6xxx_drv_write_byte(u32 addr, u32 data)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write-byte call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.write_byte != NULL))
	{
		ret = s_drv_cur->drv_ops.write_byte(1, addr, data);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
    return ret;
}

bool ssv6xxx_drv_read_byte(u32 addr)
{
    bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support read-byte call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.read_byte != NULL))
	{
	    ret = s_drv_cur->drv_ops.read_byte(1, addr);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
    return ret;
}

bool ssv6xxx_drv_write_sram(u32 addr, u8 *data, u32 size)
{
	bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write_sram call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.write_sram != NULL))
	{
    	ret = s_drv_cur->drv_ops.write_sram(addr, data, size);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
    return ret;
}

bool ssv6xxx_drv_read_sram(u32 addr, u8 *data, u32 size)
{
	bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write_sram call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.read_sram != NULL))
	{
    	ret = s_drv_cur->drv_ops.read_sram(addr, data, size);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
    return ret;
}

u32 ssv6xxx_drv_write_fw_to_sram(u8 *fw_bin, u32 fw_bin_len, u32 block_size)
{
    u32 checkSum = 0;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return 0;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write_sram call back\n",__FUNCTION__);
    	checkSum = 0;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
							&& (s_drv_cur->drv_ops.write_fw_to_sram != NULL))
	{
    	checkSum = s_drv_cur->drv_ops.write_fw_to_sram(fw_bin, fw_bin_len, block_size);
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (checkSum == 0)
	{
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
    return checkSum;
}

bool ssv6xxx_drv_start(void)
{
	bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write_sram call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (_ssv6xxx_drv_started != TRUE) 
										&& (s_drv_cur->drv_ops.start != NULL))
	{
        // if sdio, use ioctl to start the HCI
        s_drv_cur->drv_ops.start();
		ret = TRUE;	
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == TRUE)
	{
		_ssv6xxx_drv_started = TRUE;
	}
	else
	{
        SDRV_WARN("%s() : in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}

    return ret;
}

bool ssv6xxx_drv_stop(void)
{
	bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support write_sram call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
										&& (_ssv6xxx_drv_started != FALSE) 
										&& (s_drv_cur->drv_ops.stop != NULL))
	{
        // if sdio, use ioctl to start the HCI
        s_drv_cur->drv_ops.stop();
		ret = TRUE;	
	}
    OS_MUTEX_UNLOCK(drvMutex);
	
	if (ret == TRUE)
	{
	    _ssv6xxx_drv_started = FALSE;
	}
	else
	{
		SDRV_WARN("%s() : in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}

    return ret;
}


bool ssv6xxx_drv_irq_enable(bool is_isr)
{
	bool ret = FALSE;

    if (s_drv_cur == NULL)
	{        
		SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return FALSE;
	}
	
	if (!is_isr) 
	{
		OS_MUTEX_LOCK(drvMutex);
	}

	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
								&& (s_drv_cur->drv_ops.irq_enable != NULL))
	{
		/* TODO(aaron): fix the enable irq */
		s_drv_cur->drv_ops.irq_enable(usb_glue);
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (s_drv_cur->drv_ops.ssv_irq_enable != NULL))
	{
    	s_drv_cur->drv_ops.ssv_irq_enable();	
		ret = TRUE;
	}
	
	if (ret == TRUE)
	{
	    _ssv6xxx_irq_st = true;
	}
	else
	{
        SDRV_WARN("%s() : in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
	
	if (!is_isr) 
	{
        OS_MUTEX_UNLOCK(drvMutex);	
	}

	return ret;
}

bool ssv6xxx_drv_irq_disable(bool is_isr)
{
	bool ret = FALSE;

    if (s_drv_cur == NULL)
	{        
		SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
	}
	
	if (!is_isr) 
	{
		OS_MUTEX_LOCK(drvMutex);
	}

	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX)
								&& (s_drv_cur->drv_ops.irq_disable != NULL))
	{
		bool is_wait_irq = false;
		/* TODO(aaron): fix the enable irq */
    	s_drv_cur->drv_ops.irq_disable(usb_glue, is_wait_irq);
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
								&& (s_drv_cur->drv_ops.ssv_irq_disable != NULL))
	{
    	s_drv_cur->drv_ops.ssv_irq_disable();	
		ret = TRUE;
	}
	
	if (ret == TRUE)
	{
	    _ssv6xxx_irq_st = false;
	}
	else
	{
        SDRV_WARN("%s() :  in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}
	
	if (!is_isr) 
	{
        OS_MUTEX_UNLOCK(drvMutex);	
	}

	return ret;
}


bool ssv6xxx_drv_irq_status(void)
{
    return _ssv6xxx_irq_st;
}

u32 ssv6xxx_drv_get_TRX_time_stamp(void)
{
    u32 last_trx_time = 0;    
    last_trx_time = drv_trx_time;
    return last_trx_time;
}

bool ssv6xxx_drv_wakeup_wifi(bool sw)
{
	bool ret = FALSE;

    if (s_drv_cur == NULL)
	{        
		SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
	}

	if (s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
	{
		/* TODO: aaron: linux drv not support wake on WLAN */
		ret = TRUE;
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.wakeup_wifi != NULL))
	{
    	s_drv_cur->drv_ops.wakeup_wifi(sw);	
		ret = TRUE;
	}
	
	if (ret == FALSE)
	{
        SDRV_WARN("%s() : NO wakeup_wifi() in ssv_drv = (0x%08x, %s)\r\n", 
							__FUNCTION__, s_drv_cur, s_drv_cur->name);
	}
	return ret;
}

bool ssv6xxx_drv_detect_card(void)
{
	bool ret = FALSE;
    if (s_drv_cur == NULL)
	{
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
		return ret;
	}

    OS_MUTEX_LOCK(drvMutex);
	if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_LINUX))
	{
	    SDRV_FAIL("%s linux drv non-support detect_card call back\n",__FUNCTION__);
    	ret = TRUE;	
	}
	else if ((s_drv_cur->drv_info.fields.os_type == DRV_INFO_FLAG_OS_TYPE_RTOS)
									&& (s_drv_cur->drv_ops.detect_card != NULL)
									&& (s_drv_cur->drv_ops.open != NULL))
	{
        ret = s_drv_cur->drv_ops.detect_card();
        ret = s_drv_cur->drv_ops.open();
	}
    OS_MUTEX_UNLOCK(drvMutex);

	if (ret == FALSE)
	{
        SDRV_WARN("%s() : in ssv_drv = (0x%08x, %s)\r\n", 
			__FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
	}

    return ret;
}

