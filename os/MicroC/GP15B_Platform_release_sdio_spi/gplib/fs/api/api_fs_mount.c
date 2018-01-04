#include "fsystem.h"
#include "globals.h"

INT16S fs_mount(INT8U dsk)
{
	INT16S ret;
	bpb  DBR_Data;
	
	ret = DE_INVLDFUNC;
	
	if (dsk >= MAX_DISK_NUM)
		return DE_INVLDACC;
	
	if ( FileSysDrv[dsk] == NULL )
		return DE_INVLDACC;
	
	ret = FileSysDrv[dsk]->Drv_Initial();	
	if (ret!=0)
	{
		FileSysDrv[dsk]->Status = 0;		
		return DE_INVLDDRV;
	}
	
	FileSysDrv[dsk]->Status = DEVICE_READ_ALLOW;
	
	invalid_buffers(dsk);

	if (!Read_DBR(dsk,&DBR_Data)) 
	{
		DPB[dsk].dpb_mounted = 0;
		return -1;
	}
	else
	{
		DPB[dsk].dpb_mounted = 1;
	}
	
	if (DPB[dsk].dpb_mounted)
	{
	#if WITHFAT32 == 1 || WITHEXFAT==1
		if (DBR_Data.bpb_nfsect == 0) 
			ret = bpb_to_dpb(&DBR_Data, CDS[dsk].cdsDpb,1); 
		else
			ret = bpb_to_dpb(&DBR_Data, CDS[dsk].cdsDpb,0); 
	#else
		ret = bpb_to_dpb(&DBR_Data, CDS[dsk].cdsDpb);
	#endif
	}
	
	if (ret!=0)
	{
		DPB[dsk].dpb_mounted = 0;
	}
	
	return ret;
}

// zurong add for get volume data offset 
INT32U GetVolumeDataStart(INT8U dsk)
{
	INT32U  data_offset;
	REG struct dpb *dpbp;
	
	dpbp = CDS[dsk].cdsDpb;
	
	data_offset =
  #if WITHFAT32 == 1 || WITHEXFAT == 1 
	(ISFAT32(dpbp)||ISEXFAT(dpbp)) ? dpbp->dpb_xdata :
  #endif
	dpbp->dpb_data;
	
	return data_offset;
}
