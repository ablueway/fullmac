/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#include <linux/kthread.h>
#endif

#include <os.h>
#include <log.h>
#include <ssv_lib.h>
#include "ssv_timer.h"


extern void os_msg_free(void *msg);

OsMutex g_tmr_mutex;
struct ssv_list_q tmr_hd;
struct ssv_list_q free_tmr_hd;
struct ssv_list_q expired_tmr_hd;
extern struct task_info_st g_host_task_info[];
struct os_timer g_ssv_timer[SSV_TMR_MAX];

#ifdef __linux__
int SSV_Timer_Task( void *args );
#else
void SSV_Timer_Task( void *args );
#endif

struct task_info_st g_timer_task_info[] =
{
    { "ssv_tmr_task",    (OsMsgQ)0, 8,   OS_TMR_TASK_PRIO,   TMR_TASK_STACK_SIZE, NULL, SSV_Timer_Task},
};
#define MBOX_TMR_TASK        g_timer_task_info[0].qevt

void os_timer_init(void)
{
    int i;
    struct os_timer *pOSTimer;

    OS_MutexInit(&g_tmr_mutex);
    list_q_init((struct ssv_list_q *)&tmr_hd);
    list_q_init((struct ssv_list_q *)&free_tmr_hd);
    list_q_init((struct ssv_list_q *)&expired_tmr_hd);

    if (g_timer_task_info[0].qlength> 0) {
        ASSERT(OS_MsgQCreate(&g_timer_task_info[0].qevt,
        	(u32)g_timer_task_info[0].qlength) == OS_SUCCESS);
    }

    /* Create Registered Task: */
    OS_TaskCreate(g_timer_task_info[0].task_func,
    g_timer_task_info[0].task_name,
    g_timer_task_info[0].stack_size<<4,
    g_timer_task_info[0].args,
    g_timer_task_info[0].prio,
    NULL);

    MEMSET((void *)&g_ssv_timer,0,sizeof(g_ssv_timer));
    for (i = 0 ;i < SSV_TMR_MAX; i++)
    {
        pOSTimer = g_ssv_timer+i;//(struct os_timer *)OS_MemAlloc(sizeof(struct os_timer));
        list_q_qtail(&free_tmr_hd,(struct ssv_list_q *)pOSTimer);
    }
}

void _create_timer(struct os_timer *pOSTimer, u32 xElapsed)
{
    struct ssv_list_q *qhd = &tmr_hd;
    OS_MutexLock(g_tmr_mutex);

    pOSTimer->en_state = EN_TMR_CNT_DWN;
    if(qhd->qlen > 0)
    {
        struct ssv_list_q *next = qhd->next;
        struct os_timer *tmr_ptr;

        while (next != qhd)
        {
            tmr_ptr = (struct os_timer *)next;
            if(tmr_ptr->msRemian >= pOSTimer->msRemian)
            {
                list_q_insert(qhd,next->prev,(struct ssv_list_q *)pOSTimer);

                //update all timer remain time;
                next = ((struct ssv_list_q *)pOSTimer)->next;				
                while(next != qhd)
                {
                    tmr_ptr = (struct os_timer*)next;
                    if(tmr_ptr->msRemian >= pOSTimer->msRemian)
                        tmr_ptr->msRemian -= pOSTimer->msRemian;

                    next = next->next;
                }
                goto DONE;
            }
            else
            {
                pOSTimer->msRemian -= tmr_ptr->msRemian;
            }
            next = next->next;
        }

        list_q_qtail(qhd,(struct ssv_list_q *)pOSTimer);
    }
    else
    {
        list_q_qtail(qhd,(struct ssv_list_q *)pOSTimer);
    }
DONE:
    //LOG_PRINTF("add TMR(%x):%d,%d,%d,tick=%d\r\n",(u32)pOSTimer,pOSTimer->msRemian,pOSTimer->msTimeout,xElapsed,OS_MS2TICK(pOSTimer->msRemian));
    //LOG_PRINTF("%s %d,%d,%d,%d\r\n",__func__,tmr_hd.qlen,expired_tmr_hd.qlen,free_tmr_hd.qlen,pOSTimer->msRemian);
    OS_MutexUnLock(g_tmr_mutex);

}

void _update_all_timer(u32 xElapsed)
{
    struct ssv_list_q *qhd = &tmr_hd;
    struct ssv_list_q *next = NULL;
    struct os_timer *tmr_ptr;
    MsgEvent *pMsgEv=NULL;
    u32 evt_retry = 30;

    OS_MutexLock(g_tmr_mutex);

    //LOG_PRINTF("\r\nTMR update :%d\r\n",xElapsed);

    next = qhd->next;

    if (qhd->qlen > 0)//while(next != qhd)
    {
        tmr_ptr = (struct os_timer *)next;
        if (tmr_ptr->msRemian > xElapsed)
        {
            if(xElapsed>0)
                tmr_ptr->msRemian -= xElapsed;
            //    LOG_PRINTF("%x:%d,%x\r\n",(u32)tmr_ptr,tmr_ptr->msRemian,tmr_ptr->nMTData0);
            //break; //First one is the smallest one. if the smallest one > elapse ,no one will expired. so break!!
        }
        else
        {
            //LOG_PRINTF("Time's up:%,%d,%d,%x\r\n",tmr_ptr->msTimeout,tmr_ptr->msRemian,tmr_ptr->nMTData0);
            do
            {
                //next = next->next;
                tmr_ptr = (struct os_timer*)list_q_deq(qhd);
                
                //    LOG_PRINTF("Time's up:%x,next->remain=%d,data0=%d\r\n",(u32)tmr_ptr,((struct os_timer*)next)->msRemian,tmr_ptr->nMTData0);
                if(tmr_ptr->handler)
                {
EVT_AGN:
                    pMsgEv=msg_evt_alloc();
                    if(pMsgEv)
                    {
                        pMsgEv->MsgType=tmr_ptr->nMsgType;
                        pMsgEv->MsgData=(u32)tmr_ptr->handler;
                        pMsgEv->MsgData1=tmr_ptr->nMTData0;
                        pMsgEv->MsgData2=tmr_ptr->nMTData1;
                        pMsgEv->MsgData3=(u32)tmr_ptr;
                        if(tmr_ptr->infombx)
                        {
                            tmr_ptr->en_state = EN_TMR_TICK_POST;
                            list_q_qtail(&expired_tmr_hd,(struct ssv_list_q *)tmr_ptr);
                            OS_MutexUnLock(g_tmr_mutex);
                            msg_evt_post((OsMsgQ)tmr_ptr->infombx, pMsgEv);
                            OS_MutexLock(g_tmr_mutex);
                        }
                        else
                        {
                            //OS_MemFree((void*)tmr_ptr);
                            tmr_ptr->en_state = EN_TMR_FREE;
                            list_q_qtail(&free_tmr_hd,(struct ssv_list_q *)tmr_ptr);
                            os_msg_free(pMsgEv);
                            LOG_PRINTF("infombx error\r\n");
                        }
                    }
                    else
                    {
                        //OS_MemFree((void*)tmr_ptr);
                        while(evt_retry--)
                        {
                            OS_TickDelay(1);
                            LOG_PRINTF("tmr alloc evt retry %d\r\n",evt_retry);
                            goto EVT_AGN;
                        }
                        tmr_ptr->en_state = EN_TMR_FREE;
                        list_q_qtail(&free_tmr_hd,(struct ssv_list_q *)tmr_ptr);
                        LOG_PRINTF("tmr alloc evt fail %d\r\n",evt_retry);
                    }
                }
                else
                {
                    tmr_ptr->en_state = EN_TMR_FREE;
                    list_q_qtail(&free_tmr_hd,(struct ssv_list_q *)tmr_ptr);
                    LOG_PRINTF("invalid tmr\r\n");
                    //OS_MemFree((void*)tmr_ptr);
                }
                next = qhd->next;
            }while((next!=qhd)&&(((struct os_timer*)next)->msRemian==0));
        }
    }

    OS_MutexUnLock(g_tmr_mutex);

    //LOG_PRINTF("%s %d,%d,%d\r\n",__func__,tmr_hd.qlen,expired_tmr_hd.qlen,free_tmr_hd.qlen);
}

void _cancel_timer(timer_handler handler, u32 data1, u32 data2)
{
    struct ssv_list_q *qhd = &tmr_hd;
    OS_MutexLock(g_tmr_mutex);

    if(qhd->qlen > 0)
    {
        struct ssv_list_q *next = qhd->next;
        struct os_timer *tmr_ptr;        
        u32   msRemain=0;
        
        while(next != qhd)
        {
            tmr_ptr = (struct os_timer*)next;
            next = next->next;
            if (handler == tmr_ptr->handler &&
                tmr_ptr->nMTData0 == data1 &&
                tmr_ptr->nMTData1 == data2 )
            {
                msRemain = tmr_ptr->msRemian;
                tmr_ptr->handler = NULL;
                tmr_ptr->en_state = EN_TMR_FREE;
                list_q_remove(qhd,(struct ssv_list_q*)tmr_ptr);
                list_q_qtail(&free_tmr_hd,(struct ssv_list_q*)tmr_ptr);

                //re-add remain time for reset timer
                if(msRemain > 0)
                {
                    while(next != qhd)
                    {
                        tmr_ptr = (struct os_timer*)next;
                        tmr_ptr->msRemian += msRemain;                
                        next = next->next;
                    }
                }
                //LOG_PRINTF("cancel_timer:%x\r\n",(u32)tmr_ptr);
                //break;
            }
        }
    }

    qhd = &expired_tmr_hd;

    if(qhd->qlen > 0)
    {
        struct ssv_list_q* next = qhd->next;
        struct os_timer* tmr_ptr;

        while(next != qhd)
        {
            tmr_ptr = (struct os_timer*)next;
            next = next->next;
            if (handler == tmr_ptr->handler &&
                tmr_ptr->nMTData0 == data1 &&
                tmr_ptr->nMTData1 == data2 )
            {
                tmr_ptr->en_state = EN_TMR_CANCEL;
                //LOG_PRINTF("cancel expired timer:%x\r\n",(u32)tmr_ptr);
            }
        }
    }
    OS_MutexUnLock(g_tmr_mutex);
    //LOG_PRINTF("%s %d,%d,%d\r\n",__func__,tmr_hd.qlen,expired_tmr_hd.qlen,free_tmr_hd.qlen);
}

void _free_timer(struct os_timer *free_tmr)
{
    struct ssv_list_q *qhd = &expired_tmr_hd;
    OS_MutexLock(g_tmr_mutex);
    if(qhd->qlen > 0)
    {
        struct ssv_list_q* next = qhd->next;
        struct os_timer* tmr_ptr;

        while(next != qhd)
        {
            tmr_ptr = (struct os_timer*)next;
            next = next->next;
            if(tmr_ptr == free_tmr)
            {
                tmr_ptr->handler = NULL;
                tmr_ptr->en_state = EN_TMR_FREE;
                list_q_remove(qhd,(struct ssv_list_q*)tmr_ptr);
                list_q_qtail(&free_tmr_hd,(struct ssv_list_q*)tmr_ptr);
            }
        }
    }

    OS_MutexUnLock(g_tmr_mutex);
    //LOG_PRINTF("%s,%d,%d,%d\r\n",__func__,tmr_hd.qlen,expired_tmr_hd.qlen,free_tmr_hd.qlen);
}

#ifdef __linux__
int SSV_Timer_Task(void *args)
{
    u32 xStartTime, xEndTime, xElapsed;
    MsgEvent *MsgEv = NULL;
    struct os_timer *cur_tmr = NULL;

    //LOG_PRINTF("SSV_Timer_Task Start,hd=%x\r\n",(u32)&tmr_hd);
	while (!kthread_should_stop())
	{
        xElapsed = 0;
        xStartTime = OS_GetSysTick();
        cur_tmr = (struct os_timer *)tmr_hd.next;
        if(cur_tmr != (struct os_timer*)&tmr_hd)
        {
            //LOG_PRINTF("cur_tmr=%x,%d,%d,xStartTime=%d\r\n",(u32)cur_tmr,cur_tmr->msTimeout,cur_tmr->msRemian,xStartTime);
            if( OS_SUCCESS == msg_evt_fetch_timeout( MBOX_TMR_TASK, &MsgEv,cur_tmr->msRemian))
            {
                xEndTime = OS_GetSysTick();
                xElapsed = ( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
                //LOG_PRINTF("Not timeout wakup,cur_tmr=%x,%d,xElapsed=%d,xEndTime=%d,msgtype=%d\r\n",(u32)cur_tmr,cur_tmr->msRemian,xElapsed,xEndTime,MsgEv->MsgType);
            }
            else //Time out; expire timer
            {
                xElapsed = cur_tmr->msRemian;//( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
            }
        }
        else
        {
            //LOG_PRINTF("\r\nNO TMR\r\n");
            msg_evt_fetch(MBOX_TMR_TASK, &MsgEv); //There's no TMR.Block till get msg.
        }

        xStartTime = OS_GetSysTick();
        //if(xElapsed>0)
        {
            _update_all_timer(xElapsed);
        }

        if(MsgEv)
        {
            //LOG_PRINTF("MsgEv->MsgType=%d\r\n",MsgEv->MsgType);
            switch((enum msgtype_tmr)MsgEv->MsgType)
            {
                case TMR_EVT_CREATE:
                {
                    struct os_timer *pOSTimer = (struct os_timer *)MsgEv->MsgData;
                    _create_timer(pOSTimer,xElapsed);
                }
                break;
                case TMR_EVT_CANCEL:
                {
                    _cancel_timer((timer_handler)MsgEv->MsgData, MsgEv->MsgData1, MsgEv->MsgData2);
                }
                break;
                case TMR_EVT_FREE:
                {
                    _free_timer((struct os_timer*) MsgEv->MsgData);
                }
                break;
                default:
                break;
            }
            os_msg_free(MsgEv);
            MsgEv = NULL;
        }
        xEndTime = OS_GetSysTick();
        xElapsed = ( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
		
        //if(xElapsed>0)
        {
            _update_all_timer(xElapsed);
        }
    }
	return 0;
}
#else
void SSV_Timer_Task(void *args)
{
    u32 xStartTime, xEndTime, xElapsed;
    MsgEvent *MsgEv = NULL;
    struct os_timer* cur_tmr = NULL;

    //LOG_PRINTF("SSV_Timer_Task Start,hd=%x\r\n",(u32)&tmr_hd);
    while(1)
    {
        xElapsed = 0;
        xStartTime = OS_GetSysTick();
        cur_tmr = (struct os_timer*)tmr_hd.next;
        if(cur_tmr != (struct os_timer*)&tmr_hd)
        {
            //LOG_PRINTF("cur_tmr=%x,%d,%d,xStartTime=%d\r\n",(u32)cur_tmr,cur_tmr->msTimeout,cur_tmr->msRemian,xStartTime);
            if( OS_SUCCESS == msg_evt_fetch_timeout( MBOX_TMR_TASK, &MsgEv,cur_tmr->msRemian))
            {
                xEndTime = OS_GetSysTick();
                xElapsed = ( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
                //LOG_PRINTF("Not timeout wakup,cur_tmr=%x,%d,xElapsed=%d,xEndTime=%d,msgtype=%d\r\n",(u32)cur_tmr,cur_tmr->msRemian,xElapsed,xEndTime,MsgEv->MsgType);
            }
            else //Time out; expire timer
            {
                xElapsed = cur_tmr->msRemian;//( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
            }
        }
        else
        {
            //LOG_PRINTF("\r\nNO TMR\r\n");
            msg_evt_fetch(MBOX_TMR_TASK, &MsgEv); //There's no TMR.Block till get msg.
        }

        xStartTime = OS_GetSysTick();
        //if(xElapsed>0)
        {
            _update_all_timer(xElapsed);
        }

        if(MsgEv)
        {
            //LOG_PRINTF("MsgEv->MsgType=%d\r\n",MsgEv->MsgType);
            switch((enum msgtype_tmr)MsgEv->MsgType)
            {
                case TMR_EVT_CREATE:
                {
                    struct os_timer *pOSTimer = (struct os_timer *)MsgEv->MsgData;
                    _create_timer(pOSTimer,xElapsed);
                }
                break;
                case TMR_EVT_CANCEL:
                {
                    _cancel_timer((timer_handler)MsgEv->MsgData, MsgEv->MsgData1, MsgEv->MsgData2);
                }
                break;
                case TMR_EVT_FREE:
                {
                    _free_timer((struct os_timer*) MsgEv->MsgData);
                }
                break;
                default:
                break;
            }
            os_msg_free(MsgEv);
            MsgEv = NULL;
        }
        xEndTime = OS_GetSysTick();
        xElapsed = ( xEndTime - xStartTime ) * OS_TICK_RATE_MS;
		
        //if(xElapsed>0)
        {
            _update_all_timer(xElapsed);
        }
    }
}
#endif

s32 os_create_timer(u32 ms, timer_handler handler, void *data1, void *data2, void* mbx)
{
    s32 ret = 0;


    struct os_timer *pOSTimer;
    MsgEvent *pMsgEv=NULL;

    //pOSTimer = (struct os_timer *)OS_MemAlloc(sizeof(struct os_timer));
    OS_MutexLock(g_tmr_mutex);
    //LOG_PRINTF("free_tmr_hd len=%d\r\n",free_tmr_hd.qlen);
    pOSTimer = (struct os_timer*)list_q_deq(&free_tmr_hd);
    OS_MutexUnLock(g_tmr_mutex);
    //LOG_PRINTF("create TMR=%x\r\n",(u32)pOSTimer);
    if(pOSTimer)
    {
        pOSTimer->nMsgType = MEVT_HOST_TIMER;
        pOSTimer->handler = handler;
        pOSTimer->nMTData0 = (u32)data1;
        pOSTimer->nMTData1 = (u32)data2;
        pOSTimer->msTimeout = pOSTimer->msRemian = ms;
        pOSTimer->infombx = mbx;

        pMsgEv=msg_evt_alloc();
        if(pMsgEv)
        {
            pMsgEv->MsgType=TMR_EVT_CREATE;
            pMsgEv->MsgData=(u32)pOSTimer;
            pMsgEv->MsgData1=0;
            pMsgEv->MsgData2=0;
            pMsgEv->MsgData3=0;
            ret = msg_evt_post(MBOX_TMR_TASK, pMsgEv);
            return ret;
        }
    }
    ret = OS_FAILED;

    return ret;
}

s32 os_cancel_timer(timer_handler handler, u32 data1, u32 data2)
{
    s32 ret = 0;
    MsgEvent *pMsgEv=NULL;

    pMsgEv=msg_evt_alloc();
    if(pMsgEv)
    {
        pMsgEv->MsgType=TMR_EVT_CANCEL;
        pMsgEv->MsgData=(u32)handler;
        pMsgEv->MsgData1=data1;
        pMsgEv->MsgData2=data2;
        pMsgEv->MsgData3=0;
        ret = msg_evt_post(MBOX_TMR_TASK, pMsgEv);
        return ret;
    }
    else
    {
        return OS_FAILED;
    }
}

s32 os_free_timer(struct os_timer* free_tmr)
{
    //_free_timer(free_tmr);

    s32 ret = 0;
    MsgEvent *pMsgEv=NULL;
    //LOG_PRINTF("%s,%x\r\n",__func__,(u32)free_tmr);
    pMsgEv=msg_evt_alloc();
    if(pMsgEv)
    {
        pMsgEv->MsgType=TMR_EVT_FREE;
        pMsgEv->MsgData=(u32)free_tmr;
        ret = msg_evt_post(MBOX_TMR_TASK, pMsgEv);
        if(ret)
            LOG_PRINTF("1post evt fail\r\n");
        return ret;
    }
    else
    {
        LOG_PRINTF("2post evt fail\r\n");
        return OS_FAILED;
    }
}

void os_timer_expired(void* evt)
{
    MsgEvent* MsgEv = (MsgEvent*)evt;
    if(MsgEv)
    {
        struct os_timer* ptmr = (struct os_timer*)MsgEv->MsgData3;
        if(ptmr->en_state == EN_TMR_TICK_POST)
        {
            timer_handler thdr = (timer_handler)MsgEv->MsgData;
            thdr((void*)MsgEv->MsgData1, (void*)MsgEv->MsgData2);
        }
        else
        {
            LOG_PRINTF("invalid tmr state=%d\r\n",ptmr->en_state);
        }
        os_free_timer(ptmr);
    }
}

void test_expired_handler(void* data1, void* data2)
{
    LOG_PRINTF("test_expired_handler %x\r\n",(u32)data1);
}
u32 tmrData1;
void cmd_tmr(s32 argc, char *argv[])
{
    u16 timeout;
    u32 cmd = ssv6xxx_atoi(argv[1]);;
    //u32 data1;
    if(argc > 1)
    {
        switch(cmd)
        {
            case 1: //create timer
                timeout = (u16)ssv6xxx_atoi(argv[2]);
                tmrData1 = OS_Random();
                //LOG_PRINTF("test TMR = %dms,data1=%x\r\n",timeout,tmrData1);
                os_create_timer(timeout,test_expired_handler,(void*)tmrData1,NULL, (void*)MBOX_CMD_ENGINE);
                break;
            case 2: //cancel timer
                LOG_PRINTF("Cancel timer\r\n");
                os_cancel_timer(test_expired_handler,tmrData1, 0);
                break;
            case 3:
                //LOG_PRINTF("free timer\r\n");
                //os_free_timer(test_expired_handler,tmrData1, 0);
                LOG_PRINTF("%s %d,%d,%d\r\n",__func__,tmr_hd.qlen,expired_tmr_hd.qlen,free_tmr_hd.qlen);
                break;
        }
    }
}

