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

#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	(defined(NAND2_EN) && (NAND2_EN == 1))

// CodePacker data partition that contain FAT FS that you want to access through msdc	
#define USE_NAND_NUM	1
static UINT8 real_part_id_map[USE_NAND_NUM]={0}; // TODO: This file currently fix to use part 0 as msdc daat area

static INT32U nand_lba[USE_NAND_NUM];
//static INT32U *nand_buf[USE_NAND_NUM];
static INT32U nand_seccount[USE_NAND_NUM];
//static INT8U nand_dir[USE_NAND_NUM];
//static INT8U nand_cmd[USE_NAND_NUM];
static INT32U nand_ofs[USE_NAND_NUM];
static INT32U nand_size[USE_NAND_NUM];
static UINT8 ever_call_CalculateFATArea = 0;
static UINT8 bootInitStatus = 1;

static INT32S drv_l2_msc_nand_init(	void* priv)
{
	INT32S ret;
	
	DBG_PRINT(" data ~init~ \r\n");
	ret = DrvNand_initial();
	if (ret != 0)
		bootInitStatus = 1;
	else
		bootInitStatus = 0;
	DBG_PRINT(" data  ~init~ done=%d ret=%d\r\n", bootInitStatus, ret);
	return ret;
}

static INT32S drv_l2_msc_nand_uninit(void* priv)
{
	INT16U ret = 0;
	
	DBG_PRINT(" data ~uninit~ bootInitStatus=%d\r\n", bootInitStatus);
	
	if (bootInitStatus == 0)
		ret = DrvNand_flush_allblk();
	DBG_PRINT(" data ~uninit~ done=%d \r\n", ret);
	
	return ret;
}


void GetNfPartInfo(INT32U *ppart_offset, INT32U *ppart_size, INT32U part_id)
{
	if (!ever_call_CalculateFATArea)
	{
		UINT32 ret;
		
		ret = CalculateFATArea();
		if (ret != 0)
		{
			*ppart_size = 0;
			*ppart_offset = 0;
			return;
		}
		
		ever_call_CalculateFATArea = 1;
	}
	
	//if (part_id == 0)
	{
		if(Nand_part0_size_Get() > (DrvNand_get_Size() - Nand_part0_Offset_Get())) 
		{    
			*ppart_offset = Nand_part0_Offset_Get();
        	*ppart_size = DrvNand_get_Size() - Nand_part0_Offset_Get();
		} else {
        	*ppart_size = Nand_part0_size_Get();
		}
	}
}

static void drv_l2_msc_nand_get_info(void* priv,STORAGE_DRV_INFO *info)
{
	INT32U 	part_id = (INT32U) priv;
	
	GetNfPartInfo(&nand_ofs[part_id],&nand_size[part_id],real_part_id_map[part_id]);
	info->nSectors = nand_size[part_id];
	info->nBytesPerSector = 512;
	DBG_PRINT(" ~get info~ \r\n");
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
		sts = DrvNand_read_sector(nand_lba[part_id] + nand_ofs[part_id], seccount , (INT32U)buf);
	    if(sts)
	    {
			DBG_PRINT(" ~read start LBA[%d] len[%d] failed \r\n",nand_lba[part_id] + nand_ofs[part_id],seccount);
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
		DBG_PRINT(" ~read start LBA[%d] len[%d] over. left[%d] \r\n",nand_lba[part_id] + nand_ofs[part_id],seccount, nand_seccount[part_id]);
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
		sts = DrvNand_write_sector(nand_lba[part_id] + nand_ofs[part_id], seccount , (INT32U)buf);
		if(sts)
		{
			DBG_PRINT(" ~write start LBA[%d] len[%d] failed \r\n",nand_lba[part_id] + nand_ofs[part_id],seccount);
		}	
	    nand_lba[part_id]  += seccount;
	    nand_seccount[part_id] -= seccount;		
	}	
	else
	{
		DBG_PRINT(" ~write start LBA[%d] len[%d] over. left[%d] \r\n",nand_lba[part_id] + nand_ofs[part_id],seccount, nand_seccount[part_id]);
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
MDSC_LUN_STORAGE_DRV const gp_msdc_nand0 =
{
	0,									//private data
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
MDSC_LUN_STORAGE_DRV const gp_msdc_nand1 =
{
	(void*)1,							//private data
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
