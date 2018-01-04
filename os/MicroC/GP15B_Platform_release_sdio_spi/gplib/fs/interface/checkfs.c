#include "fsystem.h"
#include "globals.h"


//-------------------------------------
INT16S chekfs(INT8U disk)
{
	INT8U *FATBuff;
	struct dpb *dpbp;
	
	if (disk >= MAX_DISK_NUM)
		return -1;

	FS_OS_LOCK();
	
	dpbp = CDS[disk].cdsDpb;

	
	if ( (FileSysDrv[disk] != NULL) && 
		(FileSysDrv[disk]->Status == DEVICE_READ_ALLOW) && 		// Tristan: doing a logical AND would be better than equal comparison
		(dpbp->dpb_mounted == 1) )
	{
		FATBuff=(INT8U *)gp_malloc(512);
		if(FATBuff== NULL)
		{
			FS_OS_UNLOCK();
			return -1;
		}
		
		//Read FatTable1 data		
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt, (INT32U)FATBuff, 1, DSKREAD) )
		{
			gp_free(FATBuff);
			FS_OS_UNLOCK();
			return -1;
		}
		
		if(!(((getlong(FATBuff)==0x0FFFFFF8)&&(getlong(FATBuff+4)==0x0FFFFFFF))||(getlong(FATBuff)==0xFFFFFFF8)||(getlong(FATBuff)==0x7FFFFFF8)))
		{
			gp_free(FATBuff);
			FS_OS_UNLOCK();			
			return -1;
		}
		
		//Read FatTable2 data	
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize, (INT32U)FATBuff, 1, DSKREAD) )
		{
			gp_free(FATBuff);
			FS_OS_UNLOCK();			
			return -1;
		}
		
		if(!(((getlong(FATBuff)==0x0FFFFFF8)&&(getlong(FATBuff+4)==0x0FFFFFFF))||(getlong(FATBuff)==0xFFFFFFF8)||(getlong(FATBuff)==0x7FFFFFF8)))
		{
			gp_free(FATBuff);
			FS_OS_UNLOCK();			
			return -1;
		}
		
		gp_free(FATBuff);
		FS_OS_UNLOCK();	    
	    return 0;
	}
	

	FS_OS_UNLOCK();
	return -1;
}

//¼ÓÇ¿°æfs check
INT16S chekfsEx(INT8U disk)
{
	INT8U *FAT1Buff,*FAT2Buff;
	INT32U i=0,j=0;
	INT32U *Comp1,*Comp2;
	INT32U Size;
	struct dpb *dpbp;
	
	if(chekfs(disk))
	{
		return -1;	
	}
	
	
	if (disk >= MAX_DISK_NUM)
		return -1;

	FS_OS_LOCK();
	
	dpbp = CDS[disk].cdsDpb;

  #if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		FS_OS_UNLOCK();
		return 0; 	   
	} 		
  #endif

	
	if ( (FileSysDrv[disk] != NULL) && 
		(FileSysDrv[disk]->Status == DEVICE_READ_ALLOW) && 		// Tristan: doing a logical AND would be better than equal comparison
		(dpbp->dpb_mounted == 1) )
	{
		FAT1Buff=(INT8U *)gp_malloc(4096);
		if(FAT1Buff== NULL)
		{
			FS_OS_UNLOCK();
			return -1;
		}
		
		FAT2Buff=(INT8U *)gp_malloc(4096);
		if(FAT2Buff== NULL)
		{
			gp_free(FAT1Buff);
			FS_OS_UNLOCK();
			return -1;
		}
		
				
		Comp1=(INT32U *)FAT1Buff;
		Comp2=(INT32U *)FAT2Buff;

		
//compare FATTable 0 sector		
		//Read FatTable1 data		
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt, (INT32U)FAT1Buff, 1, DSKREAD) )
		{
			gp_free(FAT1Buff);
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();
			return -1;
		}
		
		
		//Read FatTable2 data	
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize, (INT32U)FAT2Buff, 1, DSKREAD) )
		{
			gp_free(FAT1Buff);
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();			
			return -1;
		}
		
		for(j=2;j<128;j++)
		{
			if(*(Comp2+j)!=*(Comp1+j))
			{
				gp_free(FAT1Buff);
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();			
				return -1;							
			}
		}
		
	
		
		Size=((dpbp->dpb_fatsize-1)/8)*8;
		for(i=1;i<Size;i+=8)
		{
			//Read FatTable1 data		
			if( 0 != dskxfer(disk, dpbp->dpb_fatstrt+i, (INT32U)FAT1Buff, 8, DSKREAD) )
			{
				gp_free(FAT1Buff);
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();
				return -1;
			}
			
			
			//Read FatTable2 data	
			if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize+i, (INT32U)FAT2Buff, 8, DSKREAD) )
			{
				gp_free(FAT1Buff);
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();			
				return -1;
			}
			
			for(j=0;j<1024;j++)
			{
				if(*(Comp2+j)!=*(Comp1+j))
				{
					gp_free(FAT1Buff);
					gp_free(FAT2Buff);
					FS_OS_UNLOCK();			
					return -1;				
					
				}
			}
		}
	
		Size=dpbp->dpb_fatsize%8;
		
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt+i, (INT32U)FAT1Buff, Size, DSKREAD) )
		{
			gp_free(FAT1Buff);
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();
			return -1;
		}
		
		
		//Read FatTable2 data	
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize+i, (INT32U)FAT2Buff, Size, DSKREAD) )
		{
			gp_free(FAT1Buff);
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();			
			return -1;
		}

		for(j=0;j<Size*128;j++)
		{
			if(*(Comp2+j)!=*(Comp1+j))
			{
				gp_free(FAT1Buff);
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();			
				return -1;				
				
			}
		}



		gp_free(FAT1Buff);
		gp_free(FAT2Buff);
	
		FS_OS_UNLOCK();	    
	    return 0;
	}
	
	gp_free(FAT1Buff);
	gp_free(FAT2Buff);
	FS_OS_UNLOCK();
	return -1;
}


//Table2 Copy to Table1
INT16S FATab2To1(INT8U disk)
{
	INT8U *FAT2Buff;
	INT32U i=0;
	INT32U Size;
	struct dpb *dpbp;
	
	if (disk >= MAX_DISK_NUM)
		return -1;
	

	FS_OS_LOCK();
	
	dpbp = CDS[disk].cdsDpb;

  #if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		FS_OS_UNLOCK();
		return 0;		 
	}		  
  #endif

	if ( (FileSysDrv[disk] != NULL) && 
		(FileSysDrv[disk]->Status == DEVICE_READ_ALLOW) && 		// Tristan: doing a logical AND would be better than equal comparison
		(dpbp->dpb_mounted == 1) )
	{
		
		FAT2Buff=(INT8U *)gp_malloc(4096);
		if(FAT2Buff== NULL)
		{
			FS_OS_UNLOCK();
			return -1;
		}
		
			
		
		Size=(dpbp->dpb_fatsize/8)*8;
		for(i=0;i<Size;i+=8)
		{
			//Read FatTable1 data		
			if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize+i, (INT32U)FAT2Buff, 8, DSKREAD) )
			{
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();
				return -1;
			}
			
			SetWirteBufferType(BFR_FAT);
			if( 0 != dskxfer(disk, dpbp->dpb_fatstrt+i, (INT32U)FAT2Buff, 8, DSKWRITE) )
			{
				gp_free(FAT2Buff);
				FS_OS_UNLOCK();
				return -1;
			}	
		}
	
	
		Size=dpbp->dpb_fatsize%8;
		
				
		if( 0 != dskxfer(disk, dpbp->dpb_fatstrt +dpbp->dpb_fatsize+i, (INT32U)FAT2Buff, Size, DSKREAD) )
		{
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();			
			return -1;
		}

		SetWirteBufferType(BFR_FAT);
		if(0 != dskxfer(disk, dpbp->dpb_fatstrt+i, (INT32U)FAT2Buff, Size, DSKWRITE))
		{
			gp_free(FAT2Buff);
			FS_OS_UNLOCK();
			return -1;
		}
		
		gp_free(FAT2Buff);
		FS_OS_UNLOCK();	    
	    return 0;
	}
	
	FS_OS_UNLOCK();
	return -1;
	
	
	
	
}

#if WITHEXFAT == 1
//Exfat only
void set_exfatlink(INT8U disk_id,INT8U flags)
{
    LinkFlag[disk_id] = flags;
}
#endif







