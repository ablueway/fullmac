#include "fsystem.h"
#include "globals.h"
#include "drv_l1_sw_timer.h"

#ifndef READ_ONLY

#define WIPE_OUIT_DBG   0
#define ms_EACH_STEP    25

#if (WIPE_OUIT_DBG==1)
    #define FS_DEBUG
#else
    #define FS_DEBUG    DBG_PRINT
#endif

#ifndef _UNLINK_STEP
#define _UNLINK_STEP
    typedef enum {
        UNLINK_STEP_DISABLE=0,
        UNLINK_STEP_ENABLE=1,
        UNLINK_STEP_WORK=2
    } UNLINK_STEP;
#endif //_UNLINK_STEP


static UNLINK_STEP unlink_step_en;
static struct dpb *dpbp_step;
static INT32U st_id;
static INT32U unlink_step=0;
static INT8U unlink_dsk;
static ExFATExtDIR *unlink_ext_dir;


UNLINK_STEP unlink_step_status_get(void);
void unlink_step_init(f_node_ptr fnp, CLUSTER st);  // for file system internal use
INT32S unlink_step_work(void);
void unlink_step_end(void);
static void unlink_step_uninit(void);

#if WIPE_OUIT_DBG == 1
INT32U cluster_total_nums=0;
#endif

// wipe out all FAT entries starting from st for create, delete, etc.
void wipe_out_clusters(struct dpb *dpbp, CLUSTER st)
{
  	REG CLUSTER next;

  	// Loop from start until either a FREE entry is encountered (due to a fractured file system) 
  	// or the last cluster is encountered.
  	while (st != LONG_LAST_CLUSTER) {		
    	// get the next cluster pointed to
    	next = next_cluster(dpbp, st);

    	// just exit if a damaged file system exists
    	if (next==FREE || next==1) 
        {
          #if WIPE_OUIT_DBG == 1
            FS_DEBUG ("Total cluster nums %d\r\n",cluster_total_nums);
          #endif
      		goto END;
      	}

    	// zap the FAT pointed to
    	link_fat(dpbp, st, FREE,NULL);	// Tristan: link_fat belongs to file system layer, should not be called by logic layer
    	st = next;
        #if WIPE_OUIT_DBG==1
         cluster_total_nums++;
        #endif
        
  	}

  	if (ISFAT32(dpbp)) 
    {
    	write_fsinfo(dpbp);		// Tristan: write_fsinfo belongs to file system layer, should not be called by logic layer
    }
    
END:
    return;
}


#if  WITHEXFAT == 1
void exfat_wipe_out_clusters(f_node_ptr fnp)
{
  	CLUSTER next,st,Num=1,AvidCluslong;
	st=fnp->f_start_cluster=getdstart(fnp->f_dpb, &fnp->f_dir);
	if(ISEXFAT(fnp->f_dpb))
	{
		INT32U clucount;
		clucount=fnp->f_dpb->dpb_secsize;
		clucount=(clucount<<(fnp->f_dpb->dpb_shftcnt))-1;
		clucount=((fnp->ext_dir->DataLength)+clucount)/(fnp->f_dpb->dpb_secsize);
		AvidCluslong=clucount>>(fnp->f_dpb->dpb_shftcnt);				
	}

  	// Loop from start until either a FREE entry is encountered (due to a fractured file system) 
  	// or the last cluster is encountered.
  	while (st != LONG_LAST_CLUSTER) {		
		if (NOFatChain(fnp->ext_dir->SecondaryFlags)) 
		{
			next=fnp->f_start_cluster + Num;
			if(Num>=AvidCluslong)
			{
				next=LONG_LAST_CLUSTER;
			}
		}
		else	
		{
	    	next = next_cluster(fnp->f_dpb, st);
		}
    	// just exit if a damaged file system exists
    	if (next==FREE || next==1) 
        {
          #if WIPE_OUIT_DBG == 1
            FS_DEBUG ("Total cluster nums %d\r\n",cluster_total_nums);
          #endif
      		goto END;
      	}

    	// zap the FAT pointed to
    	link_fat(fnp->f_dpb, st, FREE,fnp->ext_dir->SecondaryFlags);	// Tristan: link_fat belongs to file system layer, should not be called by logic layer
    	st = next;
		Num++;
        #if WIPE_OUIT_DBG==1
         cluster_total_nums++;
        #endif
        
  	}
END:
    return;
}
#endif

UNLINK_STEP unlink_step_status_get(void)
{
    return unlink_step_en;
}

void unlink_step_reg(void)
{
    unlink_step=0;
    unlink_step_en=UNLINK_STEP_ENABLE;
}

void unlink_step_init(f_node_ptr fnp, CLUSTER st)
{
	struct dpb *dpbp;
    
    
  #if WITHEXFAT == 1
    unlink_ext_dir = fnp->ext_dir;   
  #endif
    
	dpbp = fnp->f_dpb;
	unlink_dsk = fnp->f_dpb->dpb_unit; 

    st_id=st;  
    // FS_DEBUG ("\r\nInit ST ID:0x%x\r\n",st_id);
    unlink_step=0;
    unlink_step_en=UNLINK_STEP_ENABLE;
    dpbp_step=dpbp;

  #if WIPE_OUIT_DBG == 1
    cluster_total_nums=0;
  #endif
    
}

/// 優化 cache, 以 delete write 為優先考量

INT32S unlink_step_run(void)
{
  	REG CLUSTER next;
    struct dpb *dpbp = dpbp_step;
    INT32S step_cnt;
    INT32S ret=1;
    INT32U time_temp1; // = sw_timer_get_counter_L();
    INT32S spend_time;
    INT8U SecondFlag = 0;
   #if WITHEXFAT == 1
    INT32U AvidCluslong;
   #endif
   
   
    if (unlink_step_en==UNLINK_STEP_DISABLE)
    {
        return 0;
   	}

    if ((st_id==0xFFFFFFFF) || (st_id==0)) {
        unlink_step_en=UNLINK_STEP_DISABLE;
        ret = 0;
        goto END;
    }

    time_temp1 = sw_timer_get_counter_L();
    
    unlink_step_en=UNLINK_STEP_WORK;    
    unlink_step++;
    
    
  
    
  #if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		INT32U clucount;
		clucount=dpbp->dpb_secsize;
		clucount=(clucount<<(dpbp->dpb_shftcnt))-1;
		clucount=((unlink_ext_dir->DataLength)+clucount)/(dpbp->dpb_secsize);
		AvidCluslong=clucount>>(dpbp->dpb_shftcnt);				
	    SecondFlag = unlink_ext_dir->SecondaryFlags;
	}
  #endif
    
    step_cnt=0;
  	while (st_id != LONG_LAST_CLUSTER) 
    {
      #if WITHEXFAT == 1
        if(ISEXFAT(dpbp)&&NOFatChain(SecondFlag))
        {
			next=st_id + 1;
			if((next-st_id)>=AvidCluslong)
			{
				next=LONG_LAST_CLUSTER;
			}
        }
        else
      #endif  
        {
       	    // get the next cluster pointed to 
    	    next = next_cluster(dpbp, st_id);
        }
    	// just exit if a damaged file system exists
    	if (next==FREE || next==1) {
            unlink_step_en=UNLINK_STEP_DISABLE;
      		ret = 0;
            goto END;
      	}
      #if WIPE_OUIT_DBG == 1  
        cluster_total_nums++;
      #endif
    	// zap the FAT pointed to
    	link_fat(dpbp, st_id, FREE,SecondFlag);	// Tristan: link_fat belongs to file system layer, should not be called by logic layer
    	// and just follow the linked list
    	st_id = next;
        step_cnt++;
        spend_time = (INT32S) (sw_timer_get_counter_L()-time_temp1);

        // spend time < 0 mean the clock timer count overflow-ed
        if (spend_time>=ms_EACH_STEP || spend_time<0) {
            ret=1;  // still need next work
            goto END;
        } 
  	}

END:
    
    if (st_id == LONG_LAST_CLUSTER || ret==0)
    {
        ret=0;  // no next time del step
        unlink_step_uninit();
    }

    if (unlink_step_en!=UNLINK_STEP_DISABLE) 
    {
        FS_DEBUG ("%d:del %d sector with time:%dms , ret=0x%x\r\n",unlink_step,step_cnt,spend_time,ret);
    }
    return ret; // still need next work step
}

void unlink_step_end(void)
{
   // struct dpb *dpbp = &dpbp_step;

    // flush unlik step
    while(1) 
    {
        if (unlink_step_run()==0) {
            break;
        }
    }

    unlink_step_uninit();

}

void unlink_step_uninit(void)
{

    if (unlink_step_en!=UNLINK_STEP_DISABLE)
    {
      struct dpb *dpbp = dpbp_step;
        unlink_step=0;
        unlink_step_en=UNLINK_STEP_DISABLE;
        st_id=0xFFFFFFFF;
        
      #if WIPE_OUIT_DBG == 1
        FS_DEBUG ("\r\nDel Step Uninit\r\n");
        FS_DEBUG ("Total cluster nums %d\r\n",cluster_total_nums);
      #endif
      	if (ISFAT32(dpbp)) {
        	write_fsinfo(dpbp);		// Tristan: write_fsinfo belongs to file system layer, should not be called by logic layer
        }

        flush_buffers(unlink_dsk);          
    }
    return;
}

#endif
