/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/

#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <os.h>
#include <ssv_types.h>
#include <msgevt.h>
#include <log.h>



#ifdef CHECK_MSG_EVT_RANGE
static u32      min_msg_evt_addr;
static u32      max_msg_evt_addr;
#endif // CHECK_MSG_EVT_RANGE

#define MSG_EVT_MIN     48
#define MSG_EVT_DEBUG	0
#if (MSG_EVT_DEBUG)
#define MSGQ_STR(q)		(((q) == MBOX_SOFT_MAC)    ? "SOFT_MAC"    : \
						 ((q) == MBOX_MLME)        ? "MLME"        : \
						 ((q) == MBOX_CMD_ENGINE)  ? "CMD_ENGINE"  : \
						 ((q) == MBOX_SIM_RX_DRIVER)  ? "SIM_RX_DRIVER"  : "UNKNOWN")
#endif	/* MSG_EVT_DEBUG */

static u32      g_max_evt;
/* Message Queue Length: */
#define MBOX_MAX_MSG_EVENT			g_max_evt             
// Max message event in the system. The vaule should be larger than the amount of RX ID

//static MsgEvent sg_msgevent[MBOX_MAX_MSG_EVENT];
static MsgEvent * volatile sg_msgevt_free_list;
static OsMutex sg_msgevt_mutex;

extern u32 g_free_msgevt_cnt;

void msg_evt_show(void)
{
    OS_TASK_ENTER_CRITICAL();
    LOG_PRINTF("free msg evt:%d\r\n",g_free_msgevt_cnt);
    OS_TASK_EXIT_CRITICAL();
}

void *msg_evt_alloc_0(void)
{
    MsgEvent *msgev;
    if (sg_msgevt_free_list==NULL)
        return NULL;
    msgev = sg_msgevt_free_list;
	#ifdef CHECK_MSG_EVT_RANGE
    if (((u32)msgev < min_msg_evt_addr) || ((u32)msgev > max_msg_evt_addr))
    {
        LOG_PRINTF("A MSG: 0x%08X - 0x%08X ~ 0x%08X\n", (u32)msgev,
                    min_msg_evt_addr, max_msg_evt_addr);
    }
    ASSERT_RET((((u32)msgev >= min_msg_evt_addr) && ((u32)msgev <= max_msg_evt_addr)), 
               OS_FAILED);
	#endif // CHECK_MSG_EVT_RANGE
    sg_msgevt_free_list = (MsgEvent *)msgev->MsgData;
    g_free_msgevt_cnt --;
    return msgev;
}


void *msg_evt_alloc(void)
{
    void *msgev = NULL;
    //OS_MutexLock(sg_msgevt_mutex);
    OS_TASK_ENTER_CRITICAL();
    msgev = msg_evt_alloc_0();
    //OS_MutexUnLock(sg_msgevt_mutex);
    OS_TASK_EXIT_CRITICAL();
    
    if (msgev == NULL)
    {
        #if (MSG_EVT_DEBUG)
        LOG_DEBUG("%s: Failed to allocate msgev\r\n",__FUNCTION__);
        #endif
    }
    
    return msgev;
}

void msg_evt_free_0(MsgEvent *msgev)
{
	#ifdef CHECK_MSG_EVT_RANGE
    if (((u32)msgev < min_msg_evt_addr) || ((u32)msgev > max_msg_evt_addr))
    {
        LOG_PRINTF("F MSG: 0x%08X - 0x%08X ~ 0x%08X\n", (u32)msgev,
                    min_msg_evt_addr, max_msg_evt_addr);
        return;
    }
    //ASSERT_RET((((u32)msgev >= min_msg_evt_addr) && ((u32)msgev <= max_msg_evt_addr)), 
    //           OS_FAILED);
	#endif // CHECK_MSG_EVT_RANGE
    msgev->MsgData = (u32)sg_msgevt_free_list;
    sg_msgevt_free_list = msgev;
    g_free_msgevt_cnt ++;
}

void msg_evt_free(MsgEvent *msgev)
{
    //OS_MutexLock(sg_msgevt_mutex);
    OS_TASK_ENTER_CRITICAL();
    msg_evt_free_0(msgev);
    OS_TASK_EXIT_CRITICAL();
    //OS_MutexUnLock(sg_msgevt_mutex);
}

s32 msg_evt_post(OsMsgQ msgevq, MsgEvent *msgev)
{
    OsMsgQEntry MsgEntry;

#if (MSG_EVT_DEBUG)
	log_printf("msg_evt_post (%12s): 0x%08x\n\r", MSGQ_STR(msgevq), msgev);	
#endif

	ASSERT_RET(msgevq != NULL, OS_FAILED);
    MsgEntry.MsgCmd  = 0;
    MsgEntry.MsgData = (void *)msgev;
	return OS_MsgQEnqueue(msgevq, &MsgEntry, gOsFromISR);
}

#ifdef CHECK_MSG_EVT_RANGE
extern u32 is_sta_timer (u32 addr);
#endif // CHECK_MSG_EVT_RANGE

s32 msg_evt_fetch(OsMsgQ msgevq, MsgEvent **msgev)
{
    OsMsgQEntry MsgEntry;
    s32 res;
    MsgEvent *msg_ret = (MsgEvent *)NULL;
    
    ASSERT_RET(msgevq != NULL, OS_FAILED);
    res = OS_MsgQDequeue(msgevq, &MsgEntry, 0, gOsFromISR);
    ASSERT_RET(MsgEntry.MsgCmd == 0, res);

    if (res == OS_SUCCESS)
        msg_ret = (MsgEvent *)MsgEntry.MsgData;
	#ifdef CHECK_MSG_EVT_RANGE
    if (((u32)msg_ret < min_msg_evt_addr) || ((u32)msg_ret > max_msg_evt_addr))
    {
        if (!is_sta_timer((u32)msg_ret))
        {
            LOG_PRINTF("T MSG: 0x%08X - 0x%08X ~ 0x%08X\n", (u32)msg_ret,
                        min_msg_evt_addr, max_msg_evt_addr);
            ASSERT_RET((((u32)msg_ret >= min_msg_evt_addr) && ((u32)msg_ret <= max_msg_evt_addr)), 
                       OS_FAILED);
        }
    }
	#endif // CHECK_MSG_EVT_RANGE
	#if (MSG_EVT_DEBUG)
	log_printf("msg_evt_fetch(%12s): 0x%08x\n\r", MSGQ_STR(msgevq), msg_ret);	
	#endif

	*msgev = msg_ret;
	return res;
}

s32 msg_evt_fetch_timeout(OsMsgQ msgevq, MsgEvent **msgev, u32 msTimout)
{

    OsMsgQEntry MsgEntry;
    s32 res;
    MsgEvent *msg_ret = (MsgEvent *)NULL;

    ASSERT_RET(msgevq != NULL, OS_FAILED);
    
    if((msTimout < TICK_RATE_MS)&&(msTimout))
    {
        msTimout = TICK_RATE_MS;
    }
    res = OS_MsgQDequeue(msgevq, &MsgEntry, OS_MS2TICK(msTimout), gOsFromISR);


    if (res == OS_SUCCESS)
    {
        ASSERT_RET(MsgEntry.MsgCmd == 0, res);
        msg_ret = (MsgEvent *)MsgEntry.MsgData;
    }
#ifdef CHECK_MSG_EVT_RANGE
    if (((u32)msg_ret < min_msg_evt_addr) || ((u32)msg_ret > max_msg_evt_addr))
    {
        if (!is_sta_timer((u32)msg_ret))
        {
            LOG_PRINTF("T MSG: 0x%08X - 0x%08X ~ 0x%08X\n", (u32)msg_ret,
                        min_msg_evt_addr, max_msg_evt_addr);
            ASSERT_RET((((u32)msg_ret >= min_msg_evt_addr) && ((u32)msg_ret <= max_msg_evt_addr)),
                       OS_FAILED);
        }
    }
#endif // CHECK_MSG_EVT_RANGE
#if (MSG_EVT_DEBUG)
    //LOG_PRINTF("msg_evt_fetch_(%12s): 0x%08x\r\n", MSGQ_STR(msgevq), msg_ret);
#endif

    *msgev = msg_ret;
    return res;


}

s32 msg_evt_init(u32 max_evt_cnt)
{
    u32 idx;
    u32 size;
    
    OS_TASK_ENTER_CRITICAL();
    OS_MutexInit(&sg_msgevt_mutex);
    /**
     *  Base on pbuf pool size to allocate msg_evt number
     *  Event max num shall >= pool size
     */
     /* MSG_EVT_MIN(48) */
    if (max_evt_cnt < MSG_EVT_MIN)
        max_evt_cnt = MSG_EVT_MIN;

	/* max_evt_cnt = 112 */
    MBOX_MAX_MSG_EVENT = max_evt_cnt;
    size = sizeof(MsgEvent)*MBOX_MAX_MSG_EVENT;
    sg_msgevt_free_list = (MsgEvent *)OS_MemAlloc(size);//sg_msgevent
    ASSERT_RET(sg_msgevt_free_list, OS_FAILED);
    
    LOG_PRINTF("msg_evt_init. Max =%d\r\n",MBOX_MAX_MSG_EVENT);


	/* MBOX_MAX_MSG_EVENT(112) */
	for (idx = 0; idx < MBOX_MAX_MSG_EVENT - 1; idx++)
    {
        sg_msgevt_free_list[idx].MsgData = (u32)&sg_msgevt_free_list[idx+1];        
    }
    sg_msgevt_free_list[idx].MsgData = 0;
    g_free_msgevt_cnt = MBOX_MAX_MSG_EVENT;

/* not in */
#ifdef CHECK_MSG_EVT_RANGE
	min_msg_evt_addr = (u32)sg_msgevt_free_list;
	max_msg_evt_addr = ((u32)&sg_msgevt_free_list[MBOX_MAX_MSG_EVENT]) - 1;
#endif // CHECK_MSG_EVT_RANGE
    OS_TASK_EXIT_CRITICAL();

    return OS_SUCCESS;
}

s32 msg_evt_post_data1 (OsMsgQ msgevq, msgevt_type msg_type, u32 msg_data)
{
    MsgEvent *msg_ev = (MsgEvent *)(gOsFromISR ? msg_evt_alloc_0() : msg_evt_alloc());
    ASSERT_RET(msg_ev, OS_FAILED);
    msg_ev->MsgType  = msg_type;
    msg_ev->MsgData  = msg_data;
    
    if (OS_SUCCESS == msg_evt_post(msgevq, msg_ev))
        return OS_SUCCESS;

    if (gOsFromISR)
        msg_evt_free_0(msg_ev);
    else
        msg_evt_free(msg_ev);

    return OS_FAILED;
} // end of - msg_evt_post_14o -




