/***************************************************************************
* nand_storage.c
*
* Purpose: This is used for USB MSDC to access the storage read/write and others operations
*
* Author: Zurong
*
* Date: 2012/06/22
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
****************************************************************************/


#include "project.h"
#include "drv_l2_nand_ext.h"
#include "gplib_print_string.h"
#include "drv_l2_usbd_msdc.h"
#include "drv_l1_cache.h"

#if (defined NAND_APP_EN) && (NAND_APP_EN == 1)

// CodePacker app partition that contain FAT FS that you want to access through msdc
#define USE_NAND_NUM	1
static UINT8 real_part_id_map[USE_NAND_NUM]={1};

static INT32U nand_lba[USE_NAND_NUM];
static INT32U nand_seccount[USE_NAND_NUM];
static UINT16 nandAppPartTotal;
static UINT32 nandAppPartTotalSector;
static NF_APP_PART_INFO nandAppPartInfo[USE_NAND_NUM];
static UINT8 bootInitStatus = 1;

static INT32S drv_l2_msc_nand_init(	void* priv)
{
	INT32S ret;
	
	DBG_PRINT(" app ~init~ \r\n");
	ret = NandBootInit();
	if (ret == 0)
		ret = NandAppInitParts(&nandAppPartTotal, &nandAppPartTotalSector);
	
	if (ret != 0)
		bootInitStatus = 1;
	else
		bootInitStatus = 0;
	DBG_PRINT(" app ~init~ done=%d ret=%d\r\n", bootInitStatus, ret);
		
	return ret;
}

static INT32S drv_l2_msc_nand_uninit(void* priv)
{
	UINT32 ret = 0;
	
	DBG_PRINT(" app ~uninit~ bootInitStatus=%d\r\n", bootInitStatus);
	
	#if ((defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1))
	if (bootInitStatus == 0)
	{
		NandBootEnableWrite();
		ret = NandBootFlush();
		NandBootDisableWrite();
	}
	#endif
	DBG_PRINT(" app ~uninit~ done=%d \r\n", ret);
	
	return ret;
}

static void drv_l2_msc_nand_get_info(void* priv,STORAGE_DRV_INFO *info)
{
	INT32U 	part_id = (INT32U) priv;
	
	NandAppGetPartInfo(real_part_id_map[part_id], &nandAppPartInfo[part_id]);
	info->nSectors = nandAppPartInfo[part_id].part_size;
	info->nBytesPerSector = 512;
	DBG_PRINT(" ~get info~ real part[%d]=%d siz=%d sectors\r\n", part_id, real_part_id_map[part_id], nandAppPartInfo[part_id].part_size);
}

static INT32S drv_l2_msc_nand_read_cmd(void* priv, INT32U lba,INT32U seccount)
{
	INT32U part_id = (INT32U) priv;
	
	nand_lba[part_id] = lba;
	nand_seccount[part_id] = seccount;
	//DBG_PRINT(" ~read cmd LBA[%d] len[%d]~ \r\n",lba,seccount);
	return 0;
}

static INT32S drv_l2_msc_nand_read_dma_start(void* priv, INT32U *buf,INT8U ifwait, INT32U seccount)
{
	INT32U part_id = (INT32U) priv;
	INT32S	sts = 1;
	
	//DBG_PRINT(" ~read start len[%d]~ ",seccount);
	if((seccount != 0) && (nand_seccount[part_id] >= seccount))
	{
		sts = NandBootReadSector(nand_lba[part_id] + nandAppPartInfo[part_id].startSector, seccount , (INT32U)buf);
	    if(sts)
	    {
			DBG_PRINT(" ~read start LBA[%d] len[%d] failed \r\n",nand_lba[part_id] + nandAppPartInfo[part_id].startSector,seccount);
		}
		#if _DRV_L1_CACHE        
        cache_drain_range((INT32U) buf, (INT32U) (seccount<<9)); // drain things nand driver ever touch by cpu
        cache_invalid_range((INT32U)buf, (INT32U)(seccount<<9)); // inval so that usb always get from dram
		#endif
		nand_lba[part_id]  += seccount;
	    nand_seccount[part_id] -= seccount;	
	}
	else
	{
		DBG_PRINT(" ~read start LBA[%d] len[%d] over. left[%d] \r\n",nand_lba[part_id] + nandAppPartInfo[part_id].startSector, seccount, nand_seccount[part_id]);
	}
	
	return sts;	
}

static INT32S drv_l2_msc_nand_read_stop(void* priv)
{   
	//DBG_PRINT(" ~read stop~ \r\n");
    return 0;
}

static INT32S drv_l2_msc_nand_write_cmd(void* priv, INT32U lba,INT32U seccount)
{
	INT32U part_id = (INT32U) priv;
	
	nand_lba[part_id] = lba;
	nand_seccount[part_id] = seccount;
	//DBG_PRINT(" ~write cmd LBA[%d] len[%d]~ \r\n",lba,seccount);
	return 0;
}

static INT32S drv_l2_msc_nand_write_dma_start(void* priv, INT32U *buf,INT8U ifwait, INT32U seccount)
{
	INT32U part_id = (INT32U) priv;
	INT32S	sts = 1;	
	
	//DBG_PRINT(" ~write start len[%d]~ ",seccount);
	
	if((seccount!= 0) && (nand_seccount[part_id]>=seccount))
	{
		#if _DRV_L1_CACHE        
        cache_drain_range((INT32U) buf, (INT32U) (seccount<<9)); // if the buffer being touch by cpu, drain it
        cache_invalid_range((INT32U)buf, (INT32U)(seccount<<9)); // inval so that nand driver always get from dram,
        														 // when usbd BOT_DMA_RAM_BUF_ORDER is 0, this inval must apply
		#endif
		NandBootEnableWrite();
		sts = NandBootWriteSector(nand_lba[part_id] + nandAppPartInfo[part_id].startSector, seccount , (INT32U)buf);
		NandBootDisableWrite();
		if(sts)
		{
			DBG_PRINT(" ~write start LBA[%d] len[%d] failed \r\n",nand_lba[part_id] + nandAppPartInfo[part_id].startSector,seccount);
		}	
	    nand_lba[part_id]  += seccount;
	    nand_seccount[part_id] -= seccount;		
	}	
	else
	{
		DBG_PRINT(" ~write start LBA[%d] len[%d] over. left[%d] \r\n",nand_lba[part_id] + nandAppPartInfo[part_id].startSector,seccount, nand_seccount[part_id]);
	}	
	
	//DBG_PRINT(" ~status[%d]~ \r\n",sts);
	return sts;		
}

static INT32S drv_l2_msc_nand_write_stop(void* priv)
{
   // DBG_PRINT(" ~write stop~ \r\n");
    return 0;
}

static INT32S  drv_l2_msc_nand_dma_check(void* priv)
{
	//DBG_PRINT(" ~dam check~ \r\n");
	return 0;
}

/* RAM disk access functions table */
MDSC_LUN_STORAGE_DRV const gp_msdc_nandapp0 =
{
	(void*)0,							//private data, to mean partition index of partition with FAT
    drv_l2_msc_nand_init,				//init
    drv_l2_msc_nand_uninit,				//uninit
    NULL,								//insert event
    drv_l2_msc_nand_get_info,			//get driver info
    drv_l2_msc_nand_read_cmd,			//read command phase
    drv_l2_msc_nand_read_dma_start,		//read DMA phase
    drv_l2_msc_nand_read_stop,			//read command end phase
    drv_l2_msc_nand_write_cmd,			//write command phase
    drv_l2_msc_nand_write_dma_start,	//write DMA phase
    drv_l2_msc_nand_write_stop,			//write command end phase
    drv_l2_msc_nand_dma_check			//check DMA buffer state
};

#if USE_NAND_NUM > 1
MDSC_LUN_STORAGE_DRV const gp_msdc_nandapp1 =
{
	(void*)1,							//private data, to mean partition index of partition with FAT
    drv_l2_msc_nand_init,				//init
    drv_l2_msc_nand_uninit,				//uninit
    NULL,								//insert event
    drv_l2_msc_nand_get_info,			//get driver info
    drv_l2_msc_nand_read_cmd,			//read command phase
    drv_l2_msc_nand_read_dma_start,		//read DMA phase
    drv_l2_msc_nand_read_stop,			//read command end phase
    drv_l2_msc_nand_write_cmd,			//write command phase
    drv_l2_msc_nand_write_dma_start,	//write DMA phase
    drv_l2_msc_nand_write_stop,			//write command end phase
    drv_l2_msc_nand_dma_check			//check DMA buffer state
};
#endif
#endif