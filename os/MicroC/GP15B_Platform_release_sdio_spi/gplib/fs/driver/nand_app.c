/*
* Purpose: Nand interface of File system 
*
* Author:
*
* Date: 2008/4/19
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.01
*
* History:
        0  V1.00 :
*       1. V1.01 : 1.Adding DrvNand_bchtable_alloc for bch table initial
                   and allocte memorys for bch tables
        2. v1.02 : modify nand flash initial                  
                   by wschung 2008/05/28 
*/
#define CREAT_DRIVERLAYER_STRUCT

#include "fsystem.h"
#include "drv_l2_nand_ext.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined NAND_APP_EN) && (NAND_APP_EN == 1)

extern UINT32 DrvL1_Nand_Init(void);
extern UINT32 NandBootFormat_UP(UINT16 format_type, UINT16 format_step);

#define NVRAM_OS_LOCK(...)
#define NVRAM_OS_UNLOCK(...)
                        
//================================================================//

// CodePacker app partition that contain FAT FS that you want to access through FS
#define USE_NAND_NUM	1
static UINT8 real_part_id_map[USE_NAND_NUM]={1};

static INT16U gAPPinitFlag = 0;
static UINT16 nandAppPartTotal;
static UINT32 nandAppPartTotalSector;
static NF_APP_PART_INFO nandAppPartInfo[USE_NAND_NUM];	
static UINT8 part_id = 0;

static INT32U GetNFConfig_Update(void);
INT32U VendorNFAPPFormat(void);

/************************************************************************/
/* NAND_Initial 
func:	
		对物理器件进行使能
		使用复位指令，使器件进行强制复位动作
		初始化 驱动层 的转换表

input:	void

output:	0		成功
		－1		失败
   
note:	初始化成功后，设置了一个全局变量 bNandInit 

                                                                  */
/************************************************************************/
INT32S NAND_APP_Initial(void)
{
	SINT32 ret;
	
	
	//App init
	if(!gAPPinitFlag)
	{
	    ret = GetNFConfig_Update();
	    if(!ret)
			ret = NandBootInit();
		if(!ret)
			ret = NandAppInitParts(&nandAppPartTotal, &nandAppPartTotalSector);
		if(!ret)
			gAPPinitFlag = 1;
		#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
		else
		{
			//format app area
			ret = VendorNFAPPFormat();
			if (ret == 0)
				ret = NandBootInit();
			if(!ret)
				ret = NandAppInitParts(&nandAppPartTotal, &nandAppPartTotalSector);
			if(ret!=0)
			{
				gAPPinitFlag = 0;
				DBG_PRINT("======NAND_App_Initial Fail====\r\n");
				return ret;
			}			
			else
				gAPPinitFlag = 1;
		}
		#endif
		if(gAPPinitFlag == 1)
			DBG_PRINT ("Nand Flash APP init OK \r\n");
		else
			DBG_PRINT ("Nand Flash APP init Fail \r\n");	
	}
	
			
	DBG_PRINT("======NAND_APP_Initial success====\r\n");
	return 0;
}

static INT32U GetNFConfig_Update(void)
{
	INT32U ret;	
	
	ret = NandAppInfoAutoGet();
	
	if (ret == 0)
	{
		if (DrvL1_Nand_Init() != 0)
			ret = 0xff1f;
	}
		
	return ret;
}

INT32U VendorNFAPPFormat(void)
{
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
	INT16U wFormatType;
	INT16U wFormatStep, wFormatTotalSteps;
	#endif
	INT32U dwRet = 0;
	
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
	
	dwRet = GetNFConfig_Update();
	if (dwRet != 0)
		return dwRet;

    wFormatTotalSteps = NandAppGetTotalFormatStep();

	NandBootEnableWrite();    //erase APP Area,enable write
	
	//DEL wFormatType = GetCMDValueEx(17);
	
	//DEL wFormatStep =  GetCMDValueEx(21);
	//DEL dwRet = NandBootFormat_UP(wFormatType,wFormatStep); 
	
	wFormatType = 0x10;
	for(wFormatStep = 0; wFormatStep < wFormatTotalSteps; wFormatStep++)	
		dwRet = NandBootFormat_UP(wFormatType,wFormatStep);
	
	if(dwRet == 0)
	{
		DBG_PRINT ("Nand Flash APP Format OK \r\n");
	}
	else
	{
		DBG_PRINT ("Nand Flash APP Format Fail \r\n");
		DBG_PRINT("Nand Flash APP Format Fail Return is %x\r\n", dwRet);
	}
	
	NandBootDisableWrite();	//disable write
	#endif
	
	return dwRet;

}

INT32S NAND_APP_Uninitial(void)
{
	if(gAPPinitFlag == 1)
	{
		#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
		NandBootEnableWrite();
		NandBootFlush();
		NandBootDisableWrite();
		#endif
		
		gAPPinitFlag = 0;
	}
	DBG_PRINT("======NAND_APP_Uninitial success====\r\n");
	
	return 0;
}

void NAND_APP_GetDrvInfo(struct DrvInfo* info)
{
	if(gAPPinitFlag == 1)
	{
		NandAppGetPartInfo(real_part_id_map[part_id], &nandAppPartInfo[part_id]);
		info->nSectors = nandAppPartInfo[part_id].part_size;
		info->nBytesPerSector = 512;
	}
	else
	{
		info->nSectors = 0;
		info->nBytesPerSector = 512;
	}
	
	DBG_PRINT("======NAND App Area nSectors = 0x%x====\r\n",info->nSectors);
}

INT32S NAND_APP_ReadSector(INT32U blkno , INT32U blkcnt ,  INT32U buf)
{
    INT16U  ret;
	
    NVRAM_OS_LOCK();
	ret = NandBootReadSector(blkno + nandAppPartInfo[part_id].startSector, blkcnt , buf); 
    NVRAM_OS_UNLOCK();
    return ret;
}

/************************************************************************/
/*		NAND_WriteSector
func:	往物理介质上写入数据
input:	
output:	0		成功
		其他值	失败

note:	增加了对电脑是否可以格式化的控制
		当定义了 _PC_UNABLE_FORMAT_UDISK 时，禁止了对逻辑盘 逻辑0 扇的写入动作
		PC 不能对U盘进行格式化  
                                                                     */
/************************************************************************/
INT32S NAND_APP_WriteSector(INT32U blkno , INT32U blkcnt ,  INT32U buf)
{
    INT16U  ret = 0;
	
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
    NVRAM_OS_LOCK();
    NandBootEnableWrite();
    ret = NandBootWriteSector(blkno + nandAppPartInfo[part_id].startSector, blkcnt, buf);	
    NandBootDisableWrite();
    NVRAM_OS_UNLOCK();
    #endif
    
	return ret;
}

INT32S NAND_APP_Flush(void)
{
	return 0;
}

struct Drv_FileSystem FS_NAND_App_driver = {
	"NANDApp" ,
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
	DEVICE_READ_ALLOW | DEVICE_WRITE_ALLOW, 
	#else
	DEVICE_READ_ALLOW,
	#endif
	NAND_APP_Initial ,
	NAND_APP_Uninitial ,
	NAND_APP_GetDrvInfo ,
	NAND_APP_ReadSector ,
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
	NAND_APP_WriteSector ,
	#else
	NULL,
	#endif
	NAND_APP_Flush ,
};

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(defined NAND_APP_EN) && (NAND_APP_EN == 1)                    //
//================================================================//