/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#ifdef __linux__
#include <linux/kernel.h>
#endif

#include <host_config.h>
#include <ssv_regs.h>
#include <log.h>
#include <pbuf.h>

#include <os_wrapper.h>
#include <ssv_lib.h>


/**
 * Define Data structure and static global variables for simulation
 * and emulation use only.
 */
typedef struct HW_PBUF_Info_st 
{
    struct ssv_list_q pbf_list;
    u16         valid;
    u16         pbuf_id;
    u16         size;
    u16         pbuf_type;
    u32         pa;     /* physical address */
    //u32         va;     /* virtual address */

    /* logging infomation: */
    //u32         pktid;  /* Packet ID */
    //u32         fragid;
    //u8          parsed; /* 0: not parsed frame, 1: parsed frame */
    //u8          drop;   /* 0: not drop, 1: drop */
    //u32         rx2host; /* 0: not to host, 1: to host format */
    //u8          flow_buffer[1024];
    
} HW_PBUF_Info, *PHW_PBUF_Info;

//#define PBUF_VA_BASE                    0x8000
struct ssv_list_q free_notype_pbuf_hd;
struct ssv_list_q free_rxpbuf_hd;

#define MAX_RX_PBUF_NO   16



static HW_PBUF_Info *NoType_PBUF_Pool;
static HW_PBUF_Info *Rx_PBUF_Pool;
static u32 g_notype_pbuf_max_no;
static OsMutex PBUF_LogMutex;
static OsMutex sg_pbuf_mutex;





/**
 *  Packet buffer driver for SSV6200 Hardware Packet Buffer Manipulation.
 *  Three APIs are implemented for this driver:
 *
 *      @ PBUF_Init()           -Initialize hardware packet buffer setting 
 *                                      and a mutex.
 *      @ PBUF_MAlloc()       -Request a packet buffer from hardware.      
 *      @ PBUF_MFree()        -Release a packet buffer to hardware.
 *
 */

#if CONFIG_STATUS_CHECK
u32 g_pbuf_used=0;
#endif

s32 PBUF_Init(u32 pbuf_no)
{
#if (CONFIG_HOST_PLATFORM == 1)
    g_notype_pbuf_max_no = pbuf_no;
    NoType_PBUF_Pool = (HW_PBUF_Info *)OS_MemAlloc(sizeof(HW_PBUF_Info)*g_notype_pbuf_max_no);
    if(!NoType_PBUF_Pool)
    {   
        LOG_PRINTF("alloc NoType_PBUF_Pool fail\r\n");
        return OS_FAILED;
    }
    OS_MemSET((void *)NoType_PBUF_Pool, 0, sizeof(HW_PBUF_Info)*g_notype_pbuf_max_no);
    list_q_init((struct ssv_list_q *)&free_notype_pbuf_hd);

    Rx_PBUF_Pool = (HW_PBUF_Info *)OS_MemAlloc(sizeof(HW_PBUF_Info)*MAX_RX_PBUF_NO);
    if(!Rx_PBUF_Pool)
    {   
        LOG_PRINTF("alloc Rx_PBUF_Pool fail\r\n");
        return OS_FAILED;
    }
    OS_MemSET((void *)Rx_PBUF_Pool, 0, sizeof(HW_PBUF_Info)*MAX_RX_PBUF_NO);
    list_q_init((struct ssv_list_q *)&free_rxpbuf_hd);

    OS_MutexInit(&PBUF_LogMutex);
#endif    
    OS_MutexInit(&sg_pbuf_mutex);


#if  CONFIG_STATUS_CHECK
    g_pbuf_used = 0;
#endif //CONFIG_STATUS_CHECK

    return OS_SUCCESS;
}




#if CONFIG_STATUS_CHECK
void SSV_PBUF_Status(void){
    LOG_PRINTF("MEM SSV PBUF\r\n");    
    LOG_PRINTF("avail: %d\r\n", g_notype_pbuf_max_no-(g_pbuf_used&0xFFFF)); 
    LOG_PRINTF("used: %d,%d\r\n", (g_pbuf_used&0xFFFF),((g_pbuf_used>>16)&0xFFFF));
}
#endif//CONFIG_STATUS_CHECK

PKT_Info *__get_pbuf_from_freeQ(u32 size,struct ssv_list_q* fqd,PBuf_Type_E buf_type)
{
    HW_PBUF_Info* hpbf;
    PKT_Info *pkt_info=NULL;

    hpbf=(HW_PBUF_Info*)list_q_deq(fqd);
    
    if(hpbf->size < size) //re-allocate
    {
        OS_MemFree((void*)(hpbf->pa));
        
        pkt_info = (PKT_Info *)OS_MemAlloc(size);
        assert(pkt_info != NULL);
        OS_MemSET((void *)pkt_info, 0, size);
        hpbf->pa      = (u32)pkt_info;
        hpbf->valid   = 1;
        hpbf->size    = size;
        hpbf->pbuf_type = buf_type;
    }
    else
    {
        pkt_info = (PKT_Info *)hpbf->pa;
    }

    return pkt_info;

}

void *PBUF_MAlloc_Raw(u32 size, u32 need_header, PBuf_Type_E buf_type)
{
    PKT_Info *pkt_info=NULL;
	u32 extra_header=0;
    u32 i;
    OS_MutexLock(sg_pbuf_mutex);
    if (need_header)
    {
        extra_header = (PBU_OFFSET+ TXPB_RVSD*TX_PKT_RES_BASE);
	    size += extra_header;
    }
    //printf("PBUF_MAlloc_Raw len[%d]\n");
    switch(buf_type)
    {
        case NOTYPE_BUF:
        {
            if(list_q_len(&free_notype_pbuf_hd) == 0)
            {
                for(i=0; i<g_notype_pbuf_max_no; i++) {
                    if (NoType_PBUF_Pool[i].valid == 1) 
                        continue;
                    pkt_info = (PKT_Info *)OS_MemAlloc(size);
                    assert(pkt_info != NULL);
#if  CONFIG_STATUS_CHECK
                    g_pbuf_used++;
#endif
                    OS_MemSET((void *)pkt_info, 0, size);
            		OS_MemSET((void *)&NoType_PBUF_Pool[i], 0, sizeof(HW_PBUF_Info));
                    
                    NoType_PBUF_Pool[i].pbuf_type = buf_type;
                    NoType_PBUF_Pool[i].pa      = (u32)pkt_info;
                    //NoType_PBUF_Pool[i].va      = (PBUF_VA_BASE+i)<<16;
                    //NoType_PBUF_Pool[i].pktid   = 0xFFFFFFFF;
                    //NoType_PBUF_Pool[i].fragid  = 0xFFFFFFFF;
                    NoType_PBUF_Pool[i].valid   = 1;
                    NoType_PBUF_Pool[i].pbuf_id = i;
                    NoType_PBUF_Pool[i].size    = size;
                    break;        
                }
                if (i >= g_notype_pbuf_max_no)
                    pkt_info = NULL;
                        
            }
            else
            {
                pkt_info = __get_pbuf_from_freeQ(size, &free_notype_pbuf_hd, buf_type);
            }
        }
        break;
        case RX_BUF:
        {
            if(list_q_len(&free_rxpbuf_hd) == 0)
            {
                for(i=0; i<MAX_RX_PBUF_NO; i++) {
                    if (Rx_PBUF_Pool[i].valid == 1) 
                        continue;
                    pkt_info = (PKT_Info *)OS_MemAlloc(size);
                    if(pkt_info == NULL)
                    {
                        LOG_PRINTF("rxpbuf mem fail\r\n");
                        OS_MutexUnLock(sg_pbuf_mutex);    
                        return (void *)pkt_info;
                    }

#if  CONFIG_STATUS_CHECK
                    g_pbuf_used+=(1<<16);
#endif
                    OS_MemSET((void *)pkt_info, 0, size);
                    OS_MemSET((void *)&Rx_PBUF_Pool[i], 0, sizeof(HW_PBUF_Info));
                    
                    Rx_PBUF_Pool[i].pbuf_type = buf_type;
                    Rx_PBUF_Pool[i].pa      = (u32)pkt_info;
                    //Rx_PBUF_Pool[i].va      = (PBUF_VA_BASE+i)<<16;
                    //Rx_PBUF_Pool[i].pktid   = 0xFFFFFFFF;
                    //Rx_PBUF_Pool[i].fragid  = 0xFFFFFFFF;
                    Rx_PBUF_Pool[i].valid   = 1;
                    Rx_PBUF_Pool[i].pbuf_id = i;
                    Rx_PBUF_Pool[i].size    = size;
                    break;        
                }
                if (i >= MAX_RX_PBUF_NO)
                    pkt_info = NULL;
                        
            }
            else
            {
                pkt_info = __get_pbuf_from_freeQ(size, &free_rxpbuf_hd, buf_type);
            }
        }
        break;
        default:
        break;
    }
    OS_MutexUnLock(sg_pbuf_mutex);    

    return (void *)pkt_info;
}
static void __PBUF_MFree_0(void *PKTMSG)
{   
    /**
        * The following code is for simulation/emulation platform only.
        * In real chip, this code shall be replaced by manipulation of 
        * hardware packet engine.
        */ 
    u32 i;
    for(i=0; i<g_notype_pbuf_max_no; i++) {
        if (NoType_PBUF_Pool[i].valid== 0)
        continue;
        if ((u32)NoType_PBUF_Pool[i].pa != (u32)PKTMSG)
        continue;
        //FREE(PKTMSG); //keep to use next time.
        list_q_qtail(&free_notype_pbuf_hd,(struct ssv_list_q *)(&NoType_PBUF_Pool[i]));
        //NoType_PBUF_Pool[i].valid = 0;
                        
        break;
    }

    if(i >= g_notype_pbuf_max_no)
    {
        i=0;
        for(i=0; i<MAX_RX_PBUF_NO; i++) {
            if (Rx_PBUF_Pool[i].valid== 0)
                continue;
            if ((u32)Rx_PBUF_Pool[i].pa != (u32)PKTMSG)
                continue;
            //FREE(PKTMSG); //keep to use next time.
            list_q_qtail(&free_rxpbuf_hd,(struct ssv_list_q *)(&Rx_PBUF_Pool[i]));
                                    
            break;
    }
        
    if(i >= MAX_RX_PBUF_NO)
        ASSERT(i < MAX_RX_PBUF_NO);
}

}

static inline void __PBUF_MFree_1(void *PKTMSG)
{   
    OS_MutexLock(sg_pbuf_mutex);

    __PBUF_MFree_0(PKTMSG);    

    OS_MutexUnLock(sg_pbuf_mutex);
}

void _PBUF_MFree (void *PKTMSG)
{
	if (gOsFromISR)
		__PBUF_MFree_0(PKTMSG);
	else
		__PBUF_MFree_1(PKTMSG);
}

