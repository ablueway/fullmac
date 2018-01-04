#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S checkfattype (INT8U disk) 	 
{
	INT16S ret = -1;
	struct dpb *dpbp;
	
	if (disk >= MAX_DISK_NUM)
		return -1;	

	FS_OS_LOCK();
	
	dpbp = CDS[disk].cdsDpb;

	if ( (FileSysDrv[disk] != NULL) && 
		(FileSysDrv[disk]->Status == DEVICE_READ_ALLOW) && 		// Tristan: doing a logical AND would be better than equal comparison
		(dpbp->dpb_mounted == 1) )
	{
	#if WITHFAT32 == 1
	    if (ISFAT32(dpbp))
	    {
	        ret = FAT32_Type;
	    }
	#endif
	    if (ISFAT16(dpbp))
	    {
	        ret = FAT16_Type;
	    }
	#if WITHFAT12 == 1
	    if (ISFAT12(dpbp))
	    {
	        ret = FAT12_Type;
	    }
	#endif
	#if WITHEXFAT == 1
		if (ISEXFAT(dpbp))
		{
			ret = EXFAT_Type;
		}	
	#endif
	}

	FS_OS_UNLOCK();
	return ret;
}


static INT32U WirteBufferType = 0;

void SetWirteBufferType(INT32U Type)
{
    WirteBufferType = Type;
}

INT32U GetWirteBufferType(void)
{
    INT32U WirteBufferTypeBackup = WirteBufferType;
    WirteBufferType = 0;
    return WirteBufferTypeBackup;
}

void CleanWirteBufferTyp(void)
{
    WirteBufferType = 0;
}
