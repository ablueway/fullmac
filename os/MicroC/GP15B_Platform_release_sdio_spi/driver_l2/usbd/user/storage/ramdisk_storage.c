/***************************************************************************
* ramdisk_storage.c
*
* Purpose: This is used for USB MSDC to access the storage read/write and others operations
*
* Author: Eugene Hsu
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

//#define	C_RAMDISK_SIZE		0x100000     //1M bytes
#define	C_RAMDISK_SIZE		0x8000     //32k bytes
static INT32U ramcurlba;
static INT8S *gRamDiskPtr = NULL;
#define C_RAMDISK_SECTOR_NUM    (C_RAMDISK_SIZE >> 9)
ALIGN4 static INT8S ramdiskbuf[C_RAMDISK_SIZE];
/*****************************************************
    Functions API
*****************************************************/
/*****************************************************
    RAMDISK API
*****************************************************/
static INT32S RAMDISK_Initial_Storage(void* priv)
{
	gRamDiskPtr = (INT8S *)ramdiskbuf;
	//gp_malloc_align(C_RAMDISK_SIZE, 32);

	ramcurlba = 0;
	if(gRamDiskPtr == NULL)
		return -1;
	else
		return 0;
}

static INT32S RAMDISK_Uninitial_Storage(void* priv)
{
	if(gRamDiskPtr)
	{
		//gp_free(gRamDiskPtr);
		gRamDiskPtr = NULL;
	}
	return 0;
}

static void RAMDISK_GetDrvInfo_Storage(void* priv, STORAGE_DRV_INFO *info)
{
	//DBG_PRINT("total sector %d\r\n", size);
	info->nSectors = C_RAMDISK_SECTOR_NUM;
	info->nBytesPerSector = 512;
}

static INT32S RAMDISK_ReadSector(void* priv, INT32U *buf,INT8U ifwait,INT32U seccount)
{	
    if(seccount < C_RAMDISK_SECTOR_NUM)
    {
        //DBG_PRINT("Read DMA addr 0x%x, len 0x%x\r\n", (gRamDiskPtr + (ramcurlba << 9)), (seccount << 9));
#if _DRV_L1_CACHE
     	cache_invalid_range((INT32U) (gRamDiskPtr+(ramcurlba<<9)), (seccount<<9));
#endif     	
        gp_memcpy((INT8S *)buf, (INT8S *)gRamDiskPtr+(ramcurlba<<9), (seccount<<9));
        //memcpy(buf, gRamDiskPtr+(ramcurlba<<9), seccount<<9);
#if _DRV_L1_CACHE        
        cache_drain_range((INT32U)buf, (INT32U) (seccount<<9));
#endif        
        ramcurlba += seccount;
        return 0;    
    }
    else
    {
    	DBG_PRINT("Read DMA return -1\r\n");
        return -1;
    }
}

static INT32S RAMDISK_WriteSector(void* priv, INT32U *buf, INT8U ifwait, INT32U seccount)
{
    if(seccount < C_RAMDISK_SECTOR_NUM)
    {
#if _DRV_L1_CACHE
    	cache_invalid_range((INT32U)buf, (seccount<<9));
#endif    	
        //DBG_PRINT("Write DMA addr 0x%x, len 0x%x\r\n", (gRamDiskPtr + (ramcurlba << 9)), (seccount << 9));
		gp_memcpy((INT8S *)gRamDiskPtr+(ramcurlba<<9), (INT8S *)buf, (seccount<<9));
		//memcpy(gRamDiskPtr+(ramcurlba<<9), buf, seccount<<9);
#if _DRV_L1_CACHE        
        cache_drain_range((INT32U) (gRamDiskPtr+(ramcurlba<<9)), (INT32U) (seccount<<9));
#endif        
        ramcurlba += seccount;
        return 0;
    }
    else
    {
    	DBG_PRINT("Write DMA return -1\r\n");
        return -1;
    }
}

static INT32S RAMDISK_ReadCmdPhase(void* priv, INT32U lba,INT32U seccount)
{

    //DBG_PRINT("Read start lba %d\r\n", lba);
    ramcurlba = lba;
    return 0;
}

static INT32S RAMDISK_ReadEndCmdPhase(void* priv)
{
    //DBG_PRINT("Read end\r\n\r\n");
    ramcurlba = 0;
    return 0;
}

static INT32S RAMDISK_WriteCmdPhase(void* priv, INT32U lba, INT32U seccount)
{
    //DBG_PRINT("Write start lba %d\r\n", lba);
    ramcurlba = lba;
    return 0;
}

static INT32S  RAMDISK_WriteEndCmdPhase(void* priv)
{
   //DBG_PRINT("Write end\r\n\r\n");
    ramcurlba = 0;
    return 0;
}

static INT32S  RAMDISK_CheckDmaCheckState(void* priv)
{
    //DBG_PRINT("RAMDISK_CheckDmaCheckState\r\n");
    return STORAGE_CHECK_READY;
}

/* RAM disk access functions table */
MDSC_LUN_STORAGE_DRV const gp_msdc_ramdisk =
{
	NULL,						//private data
    RAMDISK_Initial_Storage,    //init
    RAMDISK_Uninitial_Storage,  //uninit
    NULL,                       //insert event
    RAMDISK_GetDrvInfo_Storage, //get driver info
    RAMDISK_ReadCmdPhase,       //read command phase
    RAMDISK_ReadSector,         //read DMA phase
    RAMDISK_ReadEndCmdPhase,    //read command end phase
    RAMDISK_WriteCmdPhase,      //write command phase
    RAMDISK_WriteSector,        //write DMA phase
    RAMDISK_WriteEndCmdPhase,   //write command end phase
    RAMDISK_CheckDmaCheckState  //check DMA buffer state
};