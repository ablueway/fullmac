/***************************************************************************
* sdcdisk_storage.c
*
* Purpose: This is used for USB MSDC to access the storage read/write and others operations
*
* Author:
*
* Date: 2012/06/19
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
****************************************************************************/
#include "project.h"
#include "gplib_mm_gplus.h"
#include "drv_l1_cache.h"
#include "drv_l2_usbd_msdc.h"
#include "driver_l1_cfg.h"
#include "drv_l2_sd.h"

//#define DBG_SD_MSG
#define RETRY_NUM 5
#define SD_OUT_SYM	0xFFFFFFFF

enum {
	SD_REMOVE = 0,
	SD_INSERT
};

static OS_EVENT *rw_sem = NULL;
static INT32U sd_status0 = SD_REMOVE;
static INT32U sd_status1 = SD_REMOVE;
static INT32U sd_status_bak0 = SD_REMOVE;
static INT32U sd_status_bak1 = SD_REMOVE;
static INT32U ramcurlba0 = SD_OUT_SYM;
static INT32U ramcurlba1 = SD_OUT_SYM;
/*****************************************************
    Functions API
*****************************************************/
/*****************************************************
    RAMDISK API
*****************************************************/
static INT32S usbd_delay(INT32S num)
{
	int i;
	for (i=0;i<num;++i)
		R_RANDOM0 = i;
	return 0;
}

static INT32S SDCARD_Initial_Storage(void* priv)
{
	INT32S ret = 0;
	INT8U err;		

       if((INT32U)priv == 0)
       {
	    if (sd_status0 == SD_REMOVE)  // 在 SDCARD_Insertion 可能已經 init SD
	   {
		OSSemPend(rw_sem, 0, &err); 	// 讓 SD 卡同時只被一個線程存取 	
		ret = drvl2_sd_init();
		OSSemPost(rw_sem);
		if (ret==0) {
			DBG_PRINT("[MSDC Mode] SD Card0 Insert\r\n");
			sd_status0 = SD_INSERT;
		}
	   }
	   ramcurlba0 = SD_OUT_SYM;
       }
	else
	{
	    if (sd_status1 == SD_REMOVE)  // 在 SDCARD_Insertion 可能已經 init SD
	   {
		OSSemPend(rw_sem, 0, &err); 	// 讓 SD 卡同時只被一個線程存取 	
		ret = drvl2_sd1_init();
		OSSemPost(rw_sem);
		if (ret==0) {
			DBG_PRINT("[MSDC Mode] SD Card1 Insert\r\n");
			sd_status1 = SD_INSERT;
		}
	   }
	   ramcurlba1 = SD_OUT_SYM;
	}
	
	return ret;
}

static INT32S SDCARD_Uninitial_Storage(void* priv)
{
	INT8U err;	

	 OSSemPend(rw_sem, 0, &err);	 // 讓 SD 卡同時只被一個線程存取 
	 if((INT32U)priv == 0)
	 	drvl2_sd_card_remove();
	 else
	       drvl2_sd1_card_remove();
	 OSSemPost(rw_sem);
	 
	 return 0;
}

static void SDCARD_GetDrvInfo_Storage(void* priv, STORAGE_DRV_INFO *info)
{
	INT32U size;
	INT8U err;	

	OSSemPend(rw_sem, 0, &err); 	// 讓 SD 卡同時只被一個線程存取 
	if((INT32U)priv == 0)
	{
	        size = drvl2_sd_sector_number_get();
		 DBG_PRINT("SD0 total sector %d\r\n", size);
	}
	else
	{
		 size = drvl2_sd1_sector_number_get();
		 DBG_PRINT("SD1 total sector %d\r\n", size);
	}
	OSSemPost(rw_sem);

	info->nSectors = size;
	info->nBytesPerSector = 512;
}

static INT32S SDCARD_ReadSectorDMA(void* priv, INT32U *buf,INT8U ifwait,INT32U seccount)
{	
	INT32S ret;
	INT32S i;
	INT8U err;
	
	for (i=0;i<RETRY_NUM;++i)
	{
		OSSemPend(rw_sem, 0, &err);		// 讓 SD 卡同時只被一個線程存取	
		if((INT32U)priv == 0)
			ret = drvl2_sd_read(ramcurlba0, buf, seccount);
		else
		       ret = drvl2_sd1_read(ramcurlba1, buf, seccount);
		OSSemPost(rw_sem);
		if (ret == 0) {
			break;			
		}
		usbd_delay(0x1);
	}

    if(ret == 0)
    {
        if((INT32U)priv == 0)
        {
            #ifdef DBG_SD_MSG		
	         DBG_PRINT("SD0 Read lba addr %d\r\n", ramcurlba0);
	     #endif
		  ramcurlba0 += seccount;		
            return 0;    
        }
	 else
	 {
	     #ifdef DBG_SD_MSG		
	         DBG_PRINT("SD1 Read lba addr %d\r\n", ramcurlba1);
	     #endif
		  ramcurlba1 += seccount;		
            return 0;
	 }
    }
    else
    {
        if((INT32U)priv == 0)
        {
            #ifdef DBG_SD_MSG
    	         DBG_PRINT("SD0 Read lba(%d) return -1\r\n", ramcurlba0);
	     #endif
		  ramcurlba0 = SD_OUT_SYM;    
		  sd_status0 = SD_REMOVE;			 
            return -1;
        }
	 else
	 {
	     #ifdef DBG_SD_MSG
    	         DBG_PRINT("SD1 Read lba(%d) return -1\r\n", ramcurlba1);
	     #endif
		  ramcurlba1 = SD_OUT_SYM;    
		  sd_status1 = SD_REMOVE;			 
            return -1;
	 }
    }
}

static INT32S SDCARD_WriteSectorDMA(void* priv, INT32U *buf, INT8U ifwait, INT32U seccount)
{
	INT32S ret;
	INT32S i;
	INT8U err;

	for (i=0;i<RETRY_NUM;++i)
	{	
		OSSemPend(rw_sem, 0, &err);		// 讓 SD 卡同時只被一個線程存取
		if((INT32U)priv == 0)
			ret = drvl2_sd_write(ramcurlba0, buf, seccount);
		else
		       ret = drvl2_sd1_write(ramcurlba1, buf, seccount);
		OSSemPost(rw_sem);
		if (ret == 0) {
			break;
		}
		usbd_delay(0x1);
	}

    if(ret == 0)
    {
         if((INT32U)priv == 0)
         {
             #ifdef DBG_SD_MSG
                 DBG_PRINT("SD0 Write lba addr %d\r\n", ramcurlba0);
             #endif
		   ramcurlba0 += seccount;		
	      return 0;
         }
         else
         {
             #ifdef DBG_SD_MSG
                 DBG_PRINT("SD1 Write lba addr %d\r\n", ramcurlba1);
             #endif
		   ramcurlba1 += seccount;		
	      return 0;
         }
    }
    else
    {
         if((INT32U)priv ==0)
         {
             #ifdef DBG_SD_MSG
    	          DBG_PRINT("SD0 Write lba(%d) return -1\r\n", ramcurlba0);
             #endif
		   ramcurlba0 = SD_OUT_SYM;    
		   sd_status0 = SD_REMOVE;			
             return -1;
         }
         else
         {
             #ifdef DBG_SD_MSG
    	          DBG_PRINT("SD1 Write lba(%d) return -1\r\n", ramcurlba1);
             #endif
		   ramcurlba1 = SD_OUT_SYM;    
		   sd_status1 = SD_REMOVE;			
             return -1;
         }
    }
}

static INT32S	SDCARD_ReadCmdPhase(void* priv, INT32U lba,INT32U seccount)
{

    //DBG_PRINT("Read start lba %d\r\n", lba);
    if((INT32U)priv == 0)
        ramcurlba0 = lba;
    else
	ramcurlba1 = lba;
    return 0;
}

static INT32S 	SDCARD_ReadEndCmdPhase(void* priv)
{
    //DBG_PRINT("Read end\r\n\r\n");
    if((INT32U)priv == 0)
        ramcurlba0 = SD_OUT_SYM;    
    else
        ramcurlba1 = SD_OUT_SYM; 
    return 0;
}

static INT32S	SDCARD_WriteCmdPhase(void* priv, INT32U lba, INT32U seccount)
{
    //DBG_PRINT("Write start lba %d\r\n", lba);
    if((INT32U)priv == 0)
        ramcurlba0 = lba;
    else
	ramcurlba1 = lba;
    return 0;
}

static INT32S 	SDCARD_WriteEndCmdPhase(void* priv)
{
   //DBG_PRINT("Write end\r\n\r\n");
   if((INT32U)priv == 0)
       ramcurlba0 = SD_OUT_SYM;
   else
   	ramcurlba1 = SD_OUT_SYM;
    return 0;
}

static INT32S  SDCARD_CheckDmaCheckState(void* priv)
{
    //DBG_PRINT("RAMDISK_CheckDmaCheckState\r\n");
    return STORAGE_CHECK_READY;
}

static INT32S  SDCARD_Insertion(void* priv)
{
	INT32S ret = -1;
	INT8U err;

	OSSemPend(rw_sem, 0, &err);		// 讓 SD 卡同時只被一個線程存取
	if((INT32U)priv == 0)
	{
	    if (ramcurlba0==SD_OUT_SYM)
	   {
		if (sd_status0==SD_INSERT) {
			 ret = drvl2_sdc_live_response();
			#ifdef DBG_SD_MSG		
			    if(ret != 0)
			        DBG_PRINT("SD0 Insert but live detect fail\r\n");
			#endif
		}
		else {
			 ret = drvl2_sd_init();
			#ifdef DBG_SD_MSG		
			    if(ret == 0)
			        DBG_PRINT("SD0 Init OK\r\n");
			#endif
		}
	    }
	    else
	    {
		ret = 0;	// 正在讀寫
	    }
	}
	else
	{
	    if (ramcurlba1==SD_OUT_SYM)
	   {
		if (sd_status1==SD_INSERT) {
			 ret = drvl2_sd1_live_response();
			#ifdef DBG_SD_MSG		
			    if(ret != 0)
			        DBG_PRINT("SD1 Insert but live detect fail\r\n");
			#endif
		}
		else {
			 ret = drvl2_sd1_init();
			#ifdef DBG_SD_MSG		
			    if(ret == 0)
			        DBG_PRINT("SD1 Init OK\r\n");
			#endif
		}
	    }
	    else
	    {
		ret = 0;	// 正在讀寫
	    }
	}
	OSSemPost(rw_sem);	

       if((INT32U)priv == 0)
       {
	    sd_status_bak0 = sd_status0;
	    if (ret == 0)	sd_status0 = SD_INSERT;
	    else			sd_status0 = SD_REMOVE;

	    if (sd_status0 != sd_status_bak0)
	    {
		// 若螢幕保護開時，要點亮背光
		///if(screen_saver_enable) {
		///	screen_saver_enable = 0;
	      	///	ap_state_handling_lcd_backlight_switch(1);
		///	DBG_PRINT("turn on backlight\r\n");
		///}	
	    }
       }
	else
	{
           sd_status_bak1 = sd_status1;
	    if (ret == 0)	sd_status1 = SD_INSERT;
	    else			sd_status1 = SD_REMOVE;

	    if (sd_status1 != sd_status_bak1)
	    {
		// 若螢幕保護開時，要點亮背光
		///if(screen_saver_enable) {
		///	screen_saver_enable = 0;
	      	///	ap_state_handling_lcd_backlight_switch(1);
		///	DBG_PRINT("turn on backlight\r\n");
		///}	
	    }
	}
	return ret;
}

/* SDC0 disk access functions table */
MDSC_LUN_STORAGE_DRV const gp_msdc_sd0 =
{
    0,                         //private data
    SDCARD_Initial_Storage,    //init
    SDCARD_Uninitial_Storage,  //uninit
    SDCARD_Insertion,          //insert event
    SDCARD_GetDrvInfo_Storage, //get driver info
    SDCARD_ReadCmdPhase,       //read command phase
    SDCARD_ReadSectorDMA,      //read DMA phase
    SDCARD_ReadEndCmdPhase,    //read command end phase
    SDCARD_WriteCmdPhase,      //write command phase
    SDCARD_WriteSectorDMA,     //write DMA phase
    SDCARD_WriteEndCmdPhase,   //write command end phase
    SDCARD_CheckDmaCheckState  //check DMA buffer state
};

/* SDC1 disk access functions table */
MDSC_LUN_STORAGE_DRV const gp_msdc_sd1 =
{
    ((void*)1),                //private data
    SDCARD_Initial_Storage,    //init
    SDCARD_Uninitial_Storage,  //uninit
    SDCARD_Insertion,          //insert event
    SDCARD_GetDrvInfo_Storage, //get driver info
    SDCARD_ReadCmdPhase,       //read command phase
    SDCARD_ReadSectorDMA,      //read DMA phase
    SDCARD_ReadEndCmdPhase,    //read command end phase
    SDCARD_WriteCmdPhase,      //write command phase
    SDCARD_WriteSectorDMA,     //write DMA phase
    SDCARD_WriteEndCmdPhase,   //write command end phase
    SDCARD_CheckDmaCheckState  //check DMA buffer state
};