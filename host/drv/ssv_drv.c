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


#define MAX_SSV6XXX_DRV     2
static s16                  s_drv_cnt;
static struct ssv6xxx_drv_ops *s_drv_array[MAX_SSV6XXX_DRV];
static struct ssv6xxx_drv_ops   *s_drv_cur;

bool    ssv6xxx_drv_register(struct ssv6xxx_drv_ops *ssv_drv);
bool    ssv6xxx_drv_unregister(u16 i);

static bool _ssv6xxx_drv_started = false;
static OsSemaphore ssvdrv_rx_sphr;
u32 drv_trx_time=0; //ms

OsMutex drvMutex;

extern struct ssv6xxx_drv_ops g_drv_usb_linux;


//#ifndef __SSV_UNIX_SIM__
#if (__SSV_UNIX_SIM__ == 0)
u32 free_tx_page=0;
u32 free_tx_id=0;
extern struct Host_cfg g_host_cfg;

void ssvdrv_rx_isr(void)
{
    OS_SemSignal(ssvdrv_rx_sphr);
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
        OS_MUTEX_LOCK(txsrcMutex);
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
        OS_MUTEX_UNLOCK(txsrcMutex);

        if(need_check_tx_src)
        {
            if(rty_cnt > 0){
                rty_cnt--;

                //update_tx_resource
                OS_MUTEX_LOCK(txsrcMutex);
                ssv_hal_get_tx_resources(&tx_rcs.free_page,&tx_rcs.free_id,&tx_rcs.free_space);
                OS_MUTEX_UNLOCK(txsrcMutex);
            }
            else
            {
                LOG_DEBUGF(LOG_TXRX, ("%s(): free page = %d, free id = %d, pg_cnt = %d, frm len =%d\r\n",__FUNCTION__, tx_rcs.free_page, tx_rcs.free_id, page_count, frame_len));        
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
        OS_MUTEX_LOCK(txsrcMutex);
        ssv_hal_get_tx_resources(&tx_rcs.free_page,&tx_rcs.free_id,&tx_rcs.free_space);
        OS_MUTEX_UNLOCK(txsrcMutex);

        times++;
    }
    return TRUE;
}

#endif//#ifndef __SSV_UNIX_SIM__

#if 0 //( CONFIG_CMD_BUS_TEST==1)
extern OsMsgQ spi_qevt;
#endif
void  CmdEng_RxHdlData(void *frame);
u32 ssv6xxx_drv_get_handle()
{
    u32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->handle == NULL)
    {
        SDRV_WARN("%s() : NO handle() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return -1;
    }
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->handle();
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}

bool ssv6xxx_drv_ack_int()
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->ack_int == NULL)
    {
        SDRV_WARN("%s() : NO ack_int() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->ack_int();
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_register(struct ssv6xxx_drv_ops *ssv_drv)
{
    u16 i;

    if (s_drv_cnt == MAX_SSV6XXX_DRV)
    {
        SDRV_ERROR("%s s_drv_cnt = MAX_SSV6XXX_DRV!\r\n",__FUNCTION__);
        return false;
    }
    SDRV_TRACE("%s() <= : 0x%08x, %s\r\n", __FUNCTION__, ssv_drv, ssv_drv->name);

    // find empty slot in array
    for (i = 0; i < MAX_SSV6XXX_DRV; i++)
    {
        if (s_drv_array[i] == NULL)
        {
            s_drv_array[i] = ssv_drv;
            s_drv_cnt++;
            SDRV_TRACE("%s() => : ok! s_drv_cnt = %d, i = %d, ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cnt, i, ssv_drv, ssv_drv->name);
            return TRUE;
        }
    }

    /* never reach here! */
    SDRV_FATAL("%s should never reach here!\r\n",__FUNCTION__);
    return FALSE;
}

bool ssv6xxx_drv_unregister(u16 i)
{
    if (s_drv_cnt == 0)
    {
        SDRV_WARN("%s() : s_drv_cnt = 0, return true!\r\n", __FUNCTION__);
        return TRUE;
    }
    SDRV_TRACE("%s() <= : i = %d, 0x%08x, %s\r\n", __FUNCTION__, i, s_drv_array[i], s_drv_array[i]->name);

    // find matching slot in array
    s_drv_array[i] = 0;
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
            LOG_PRINTF("%-10s : 0x%08x (selected)\r\n", s_drv_array[i]->name, (unsigned int)s_drv_array[i]);
        }
        else
        {
            LOG_PRINTF("%-10s : 0x%08x\r\n", s_drv_array[i]->name, (unsigned int)s_drv_array[i]);
        }
    }
    LOG_PRINTF("%-10s = %d\r\n", "TOTAL", s_drv_cnt);
}

bool ssv6xxx_drv_module_init(void)
{
    SDRV_TRACE("%s() <=\r\n", __FUNCTION__);

    s_drv_cnt = 0;
    MEMSET(s_drv_array, 0x00, MAX_SSV6XXX_DRV * sizeof(struct ssv6xxx_drv_ops *));
    s_drv_cur = 0;

    // register each driver
//#if (SDRV_INCLUDE_SIM)
//	ssv6xxx_drv_register(&g_drv_sim);
//#endif
	ssv6xxx_drv_register(&g_drv_usb_linux);
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
        return FALSE;
    if (OS_SUCCESS != OS_MutexInit(&txsrcMutex))
        return FALSE;

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
            if (s_drv_array[i]->close != NULL)
                s_drv_array[i]->close();

            if (!ssv6xxx_drv_unregister((u16)i))
                SDRV_WARN("ssv6xxx_drv_unregister(%d) fail!\r\n", i);
        }
    }

    s_drv_cur = 0;
    OS_MutexDelete(drvMutex);
    OS_MutexDelete(txsrcMutex);
    SDRV_TRACE("%s() =>\r\n", __FUNCTION__);
    return;
}

bool ssv6xxx_drv_select(char name[32])
{
    u16 i;
    //bool bRet;
    struct ssv6xxx_drv_ops *drv_target;

    SDRV_TRACE("%s() <= : name = %s, s_drv_cnt = %d, s_drv_cur = (0x%08x)\r\n",
		__FUNCTION__, name, s_drv_cnt, (unsigned int)s_drv_cur);

	/* s_drv_cnt = 2 */
    if (s_drv_cnt == 0)
		SDRV_FAIL("%s s_drv_cnt = 0\r\n",__FUNCTION__);

    // find the matching ssv_drv
    drv_target = NULL;
    for (i = 0; i < s_drv_cnt; i++)
    {
    	/* name = "spi" */
        if (STRCMP(name, s_drv_array[i]->name) == 0)
        {
            drv_target = s_drv_array[i];
            break;
        }
    }

    if (drv_target == NULL)
    {
	    LOG_PRINTF("ssv driver '%s' is NOT available now!\r\n", name);
        // ssv6xxx_drv_list();
        return FALSE;
    }
    // if the target drv = current drv, just return
    if (drv_target == s_drv_cur)
    {
	    LOG_PRINTF("ssv drv '%s' is already in selection.\r\n", drv_target->name);
        // ssv6xxx_drv_list();
        return TRUE;
    }

    if (drv_target->open != NULL)
    {
        if (drv_target->open() == false)
    	{
			SDRV_FAIL("open() fail! in s_drv_cur (0x%08x, %s)\r\n", 
					(unsigned int)drv_target, drv_target->name);
	        return FALSE;
		}
	}
    if (drv_target->init != NULL)
    {
        if (drv_target->init() == false)
    	{
			SDRV_FAIL("open() fail! in s_drv_cur (0x%08x, %s)\r\n", 
						(unsigned int)drv_target, drv_target->name);
	        return FALSE;
		}
	}

    s_drv_cur = drv_target;
    LOG_PRINTF("select drv -> %-10s : 0x%08x\r\n", 
		s_drv_cur->name, (unsigned int)s_drv_cur);
    // ssv6xxx_drv_list();
    return TRUE;
}

bool ssv6xxx_drv_open(void)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->open != NULL);

    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->open();
    OS_MUTEX_UNLOCK(drvMutex);

    return ret;
}

bool ssv6xxx_drv_close(void)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->close != NULL);

    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->close();
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_init(void)
{
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->init != NULL);

    return (s_drv_cur->init());
}

s32 ssv6xxx_drv_recv(u8 *dat, size_t len)
{
    s32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->recv == NULL)
    {
        SDRV_WARN("%s() : NO recv() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return -1;
    }
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->recv(dat, len);
    if(retVal>0)
        drv_trx_time = os_tick2ms(OS_GetSysTick());
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}

bool ssv6xxx_drv_get_name(char name[32])
{
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

	STRCPY(name, s_drv_cur->name);
    return TRUE;
}

bool ssv6xxx_drv_ioctl(u32 ctl_code,
                            void *in_buf, size_t in_size,
                            void *out_buf, size_t out_size,
                            size_t *bytes_ret)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->ioctl == NULL)
    {
        SDRV_WARN("%s() : NO ioctl() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->ioctl(ctl_code, in_buf, in_size, out_buf, out_size, bytes_ret);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

#if CONFIG_STATUS_CHECK
extern u32 g_dump_tx;
extern void _packetdump(const char *title, const u8 *buf,
                             size_t len);

#endif

s32 ssv6xxx_drv_send(void *dat, size_t len)
{
    s32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->send == NULL)
    {
        SDRV_WARN("%s() : NO send() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return -1;
    }


#if CONFIG_STATUS_CHECK
    if(g_dump_tx)
        _packetdump("ssv6xxx_drv_send", dat, len);
#endif

    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->send(dat, len);
    if(retVal>0)
        drv_trx_time = os_tick2ms(OS_GetSysTick());
    OS_MUTEX_UNLOCK(drvMutex);

    return retVal;
}

u32 ssv6xxx_drv_read_reg(u32 addr)
{
    u32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_reg == NULL)
    {
        SDRV_WARN("%s() : NO read_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return -1;
    }
    
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->read_reg(addr);
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}

bool ssv6xxx_drv_write_reg(u32 addr, u32 data)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_reg == NULL)
    {
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_reg(addr, data);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_set_reg(u32 _REG_,u32 _VAL_,u32 _SHIFT_, u32 _MASK_)
{
    u32 regVal;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);


    regVal= ssv6xxx_drv_read_reg(_REG_);
    regVal =((((_VAL_) << _SHIFT_) & ~_MASK_) | (regVal & _MASK_));
    if(TRUE==ssv6xxx_drv_write_reg(_REG_, regVal))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
bool ssv6xxx_drv_write_byte(u32 addr, u32 data)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_byte == NULL)
    {
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_byte(1,addr, data);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_read_byte(u32 addr)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_byte == NULL)
    {
        SDRV_WARN("%s() : NO read_byte() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->read_byte(1,addr);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_write_sram (u32 addr, u8 *data, u32 size)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_sram == NULL)
    {
        SDRV_WARN("%s() : NO write_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_sram(addr, data, size);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_read_sram (u32 addr, u8 *data, u32 size)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_sram == NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->read_sram(addr, data, size);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

u32 ssv6xxx_drv_write_fw_to_sram(u8 *fw_bin, u32 fw_bin_len, u32 block_size)
{
    u32 checkSum=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_fw_to_sram== NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return 0;
    }
    OS_MUTEX_LOCK(drvMutex);
    checkSum=s_drv_cur->write_fw_to_sram(fw_bin, fw_bin_len,block_size);
    OS_MUTEX_UNLOCK(drvMutex);
    return checkSum;
}

bool ssv6xxx_drv_start (void)
{
#ifdef __linux__
    if ((_ssv6xxx_drv_started != TRUE) && (s_drv_cur->start != NULL))
    {
        // if sdio, use ioctl to start the HCI
        s_drv_cur->start();
    }
#endif
    _ssv6xxx_drv_started = TRUE;
    return TRUE;
}

bool ssv6xxx_drv_stop (void)
{
#ifdef __linux__
    if ((_ssv6xxx_drv_started != FALSE) && (s_drv_cur->stop != NULL))
    {
        s_drv_cur->stop();
    }
#endif
    _ssv6xxx_drv_started = FALSE;
    return TRUE;
}

static bool _ssv6xxx_irq_st = false;

bool ssv6xxx_drv_irq_enable(bool is_isr)
{
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);
    if(!is_isr)
        OS_MUTEX_LOCK(drvMutex);
    if (s_drv_cur->ssv_irq_enable== NULL)
    {        
        if(!is_isr)
            OS_MUTEX_UNLOCK(drvMutex);
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    s_drv_cur->ssv_irq_enable();
    _ssv6xxx_irq_st = true;
    if(!is_isr)
        OS_MUTEX_UNLOCK(drvMutex);
    return TRUE;

}

bool ssv6xxx_drv_irq_disable(bool is_isr)
{
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if(!is_isr)
        OS_MUTEX_LOCK(drvMutex);
    if (s_drv_cur->ssv_irq_disable== NULL)
    {
        if(!is_isr)
            OS_MUTEX_UNLOCK(drvMutex);
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, (unsigned int)s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    s_drv_cur->ssv_irq_disable();
    _ssv6xxx_irq_st = false;
    if(!is_isr)
        OS_MUTEX_UNLOCK(drvMutex);
    return TRUE;

}

bool ssv6xxx_drv_irq_status(void)
{
    return _ssv6xxx_irq_st;
}

u32 ssv6xxx_drv_get_TRX_time_stamp(void)
{
    u32 last_trx_time;
    
    //OS_MUTEX_LOCK(drvMutex);
    last_trx_time =  drv_trx_time;
    //OS_MUTEX_UNLOCK(drvMutex);
    return last_trx_time;
}

bool ssv6xxx_drv_wakeup_wifi(bool sw)
{
    bool ret;

    if (s_drv_cur->wakeup_wifi== NULL)
    {
        SDRV_WARN("%s() : NO wakeup_wifi() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    
    //ssv6xxx_drv_irq_disable(false);
    {
        //u32 val;
        OS_MUTEX_LOCK(drvMutex);
        //val = OS_EnterCritical();
        //LOG_PRINTF("%s,sw=%d\r\n",__func__,sw);
        ret = s_drv_cur->wakeup_wifi(sw);
        //OS_ExitCritical(val);
        OS_MUTEX_UNLOCK(drvMutex);
    }
    //ssv6xxx_drv_irq_enable(false);
    return ret;
}
bool ssv6xxx_drv_detect_card(void)
{
    bool ret=FALSE;
    
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    if(s_drv_cur->detect_card != NULL)
    {
        OS_MUTEX_LOCK(drvMutex);
        ret = s_drv_cur->detect_card();
        ret=s_drv_cur->open();
        //ret=s_drv_cur->init();
        OS_MUTEX_UNLOCK(drvMutex);
    }

    return ret;
}

