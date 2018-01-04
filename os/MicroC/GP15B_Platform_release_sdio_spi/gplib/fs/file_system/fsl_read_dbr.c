#include "fsystem.h"
#include "globals.h"


#ifdef _rdbr_debug_
#include "stdio.h"
#define rdbr_printf(x)   printf(x) 
#define rdbr_printf1(x,y)   printf(x,y) 
#define rdbr_printf2(x,y,z)   printf(x,y,z) 
#else
#define rdbr_printf(x)   
#define rdbr_printf1(x,y)   
#define rdbr_printf2(x,y,z)   
#endif

#define C_BS_FilSysType_Offset 				(54>>WORD_PACK_VALUE)		//8 bytes
#define C_DBR_End_Offset					(510>>WORD_PACK_VALUE)
#define C_DBR_EndNum						0xaa55


INT16S Read_DBR(INT16S dsk, bpb * bpbp)
{
	INT8U *bp;
	INT32S i,j;
	INT32U up_bound;  
	BOOLEAN Flag = FALSE;
	BOOLEAN isMBR = 0;
	INT32U hidden_cnt = 0; 
	BOOLEAN bFAT32 = FALSE;
	#if WITHEXFAT
	BOOLEAN bExFAT = FALSE;
	#endif
	INT16U sector_count = 1;	
	INT16U shftcnt, nsector;
	struct DrvInfo info;
	INT32U sign;
	INT8U exitflag;
	bp = firstbuf ->b_buffer;
	if (!flush1(firstbuf)) {
		return FALSE;
	}
	
	firstbuf->b_flag &= ~BFR_VALID;
	
	if ((FileSysDrv[dsk]==NULL) || (FileSysDrv[dsk]->Status==0)) {
		return FALSE;
	}
		
	FileSysDrv[dsk]->Drv_GetDrvInfo(&info); 
	up_bound = info.nSectors;
	if ((up_bound==0xffffffff) || (up_bound>0xFF)) {
		up_bound = 0xFF;
	}
	sector_count = info.nBytesPerSector >> 9;
	if(sector_count<1 || sector_count>=16) {
		return FALSE;
	}

	for (i = 0; i < up_bound; i += sector_count) {
		if (0 != dskxfer(dsk, (INT32U) i, (INT32U) bp, 1, DSKREAD)) {
			return FALSE;
		}
#if 0
		if (getword(&(bp[C_DBR_End_Offset])) == C_DBR_EndNum) {	//get end code
			if (i == 0)	{			// first sector
				if ((getlong(&(bp[0x48>>WORD_PACK_VALUE])) & 0x00ffffff) == 0x00646f50) {			// is ipod
					hidden_cnt = getlong(&(bp[0x1d6>>WORD_PACK_VALUE])); 	     //get MBR hidden offset 		  	
					
					hidden_cnt *= sector_count;
					i = hidden_cnt;
					up_bound = hidden_cnt + sector_count;
					
					i -= sector_count;
					continue;
				} else if ( ((getbyte(&(bp[0x1c2>>WORD_PACK_VALUE])) & 0x00ff) < 0x10) &&  // patition type
					(getlong(&(bp[0x1ca>>WORD_PACK_VALUE])) > 0x0) &&   // total sector in this partition
					((getlong(&(bp[0x00])) & 0x00ff00ff) != 0x009000eb) )  //is MBR
				{ 	
					hidden_cnt = getlong(&(bp[0x1c6>>WORD_PACK_VALUE])); 	     //get MBR hidden offset 		  						
					hidden_cnt *= sector_count;
					i = hidden_cnt;
					up_bound = hidden_cnt + sector_count;
					
					rdbr_printf1("hidden sector: 0x%x\n", i);
					i -= sector_count;
					continue;
				}
			}			
			
		  #ifdef unSP
			sign = getlong(&bp[C_BS_FilSysType_Offset+1]) & 0x00FFFFFF; 				
		  #else
			sign = getlong(&bp[C_BS_FilSysType_Offset+2]) & 0x00FFFFFF;
		  #endif

			if (sign == 0x00323154) {		//cmp fat string
				Flag = TRUE;
				rdbr_printf("---->FAT12\n");
				break;
			} else if (sign == 0x00363154) {
				Flag = TRUE;
				rdbr_printf("---->FAT16\n");
				break;
			} else if (sign == 0x00323354) {
                Flag = TRUE; 
                rdbr_printf("---->FAT16\n"); 
                break; 
            }
            sign = getlong(&bp[0x54 >> WORD_PACK_VALUE]) & 0x00FFFFFF;
			if (sign == 0x00323354) {
				Flag = TRUE;
				rdbr_printf("---->FAT32\n");
				bFAT32 = TRUE;
				break;
			}
		}
#else
	if(getword(&(bp[C_DBR_End_Offset])) == C_DBR_EndNum)
	{
		if((isMBR == 0) && (i == 0))
		{
			INT8U boot_indicator, partition_type;
			INT32U total_sectors;
			INT32U start_sector;
			
			if ((getlong(&(bp[0x48>>WORD_PACK_VALUE])) & 0x00ffffff) == 0x00646f50)				// is ipod
			{
				partition_type = getbyte(&(bp[0x1d2>>WORD_PACK_VALUE]));
				boot_indicator = getbyte(&(bp[0x1ce>>WORD_PACK_VALUE]));
				start_sector = getlong(&(bp[0x1d6>>WORD_PACK_VALUE]));
				total_sectors = getlong(&(bp[0x1da>>WORD_PACK_VALUE]));
				if(start_sector == 0)
				{
					partition_type = getbyte(&(bp[0x1c2>>WORD_PACK_VALUE]));
					boot_indicator = getbyte(&(bp[0x1be>>WORD_PACK_VALUE]));
					start_sector = getlong(&(bp[0x1c6>>WORD_PACK_VALUE]));
					total_sectors = getlong(&(bp[0x1ca>>WORD_PACK_VALUE]));
				}
			}
			else
			{
				partition_type = getbyte(&(bp[0x1c2>>WORD_PACK_VALUE]));
				boot_indicator = getbyte(&(bp[0x1be>>WORD_PACK_VALUE]));
				start_sector = getlong(&(bp[0x1c6>>WORD_PACK_VALUE]));
				total_sectors = getlong(&(bp[0x1ca>>WORD_PACK_VALUE]));
			}
			
			if( (boot_indicator == 0 || boot_indicator == 0x80) &&				// boot indicator is 0x00 or 0x80
				(partition_type < 0x10) &&  									// patition type	
				((start_sector > 0x0) && (start_sector < info.nSectors)) &&		// start sector
				((total_sectors > 0x0) && (total_sectors < info.nSectors))		// total sector in this partition
			)
			{ 	
				// is it MBR?? no, must check DBR
				isMBR = 1;	// goto check DBR(recheck MBR)
				
				hidden_cnt = start_sector * sector_count;
				i = hidden_cnt - sector_count;
				up_bound = hidden_cnt + sector_count;
				continue;
			}
		}
		
		// is DBR??
		if( ((getlong(&(bp[0x00])) & 0x00ff00ff) == 0x009000eb) ||
			(bp[0] == 0xe9) )
		{
		#ifdef unSP
			sign = getlong(&bp[C_BS_FilSysType_Offset+1]) & 0x00FFFFFF; 				
		#else
			sign = getlong(&bp[C_BS_FilSysType_Offset+2]) & 0x00FFFFFF;
		#endif

			if (sign == 0x00323154) {		//cmp fat string
				Flag = TRUE;
				rdbr_printf("---->FAT12\n");
				break;
			} else if (sign == 0x00363154) {
				Flag = TRUE;
				rdbr_printf("---->FAT16\n");
				break;
			} else if (sign == 0x00323354) {
				Flag = TRUE; 
				rdbr_printf("---->FAT16\n"); 
				break; 
			}
			  else if (sign == 0x00323150){  //Huajun Add Mattl find
				Flag = TRUE; 
				rdbr_printf("---->FAT16\n"); 
				break; 
			}
			  
			sign = getlong(&bp[0x54 >> WORD_PACK_VALUE]) & 0x00FFFFFF;
			if (sign == 0x00323354) {
				Flag = TRUE;
				rdbr_printf("---->FAT32\n");
				bFAT32 = TRUE;
				break;
			}
			
			#if WITHEXFAT
			sign = getlong(&bp[0x03 >> WORD_PACK_VALUE]) & 0xFFFFFFFF;
			if(sign==0x41465845)
			{
				Flag = TRUE;
				rdbr_printf("---->ExFAT\n");				
				bExFAT=TRUE;
				break;
			}
			#endif
	
			if(isMBR == 1)
			{
				i = -sector_count;
				isMBR = 2;	// no MBR
			}
		}
	}
	else
	{
		if(isMBR == 1)
		{
			i = -sector_count;
			if ((up_bound==0xffffffff) || (up_bound>0xFF)) {	//huajun @ 2013-09-24 解决当隐藏区比较大时，fs 有问题时，要mount很长时间
				up_bound = 0xFF;
			}

			isMBR = 2;	// no MBR
		}
	}
#endif

	}

	if (Flag == TRUE) {
	  #if WITHEXFAT 
	  if(bExFAT==FALSE)
	  {
	  #endif
	  #ifdef unSP	
		bpbp->bpb_nbyte = getWordfromWord(bp, 11); //get bytes per sector		
		bpbp->bpb_nsector = GetBytefromWord(bp, 13); //get sectors per cluster   13		
		bpbp->bpb_nreserved  = getWordfromWord(bp, 14); 	 //get Reserved Sectors 	 14 										
		bpbp->bpb_nfat = GetBytefromWord(bp, 16);//get FAT number 16
		bpbp->bpb_ndirent = getWordfromWord(bp, 17); // get root entry cnt 17
		bpbp->bpb_nsize = getWordfromWord(bp, 19); //get total sector cnt 19		
		bpbp->bpb_nfsect = getWordfromWord(bp, 22);//get fat sector cnt 22
	 	bpbp->bpb_hidden = i;
		bpbp->bpb_huge = getLongfromWord(bp, 32);//get huge size in sectors 										
	  #else
		bpbp->bpb_nbyte = getword(&(bp[0x0B])); 		// get bytes per sector
		bpbp->bpb_nsector = bp[0x0D]; 					// get sectors per cluster
		bpbp->bpb_nreserved	= getword(&(bp[0x0E]));		// get Reserved Sectors
		bpbp->bpb_nfat = bp[0x10];						// get FAT number
		bpbp->bpb_ndirent = getword(&(bp[0x11])); 		// get root entry count
		bpbp->bpb_nsize = getword(&(bp[0x13])); 		// get total sector count
	    bpbp->bpb_nfsect = getword(&(bp[0x16]));		// get fat sector count
		bpbp->bpb_hidden = i;							// get hidden sector cnt 28
		bpbp->bpb_huge = getlong(&(bp[0x20]));			// get huge size in sectors		
	  #endif
	
	  #if WITHFAT32 == 1
		if (bFAT32) {
		  #ifdef unSP	
			bpbp->bpb_xnfsect = getLongfromWord(bp, 0x24);//get FAT32 FAT sectors
			bpbp->bpb_xrootclst = getLongfromWord(bp, 0x2c);//get root cluster
			bpbp->bpb_xfsinfosec = getWordfromWord(bp, 0x30);//get fs info size
			bpbp->bpb_xflags = getWordfromWord(bp, 0x28);//get fat32 ext flag
			bpbp->bpb_xbackupsec = getWordfromWord(bp, 0x32);//get fat32 backup boot sector number
		  #else
			bpbp->bpb_xnfsect = getlong(&(bp[0x24]));	// get FAT32 FAT sectors
			bpbp->bpb_xflags = getword(&(bp[0x28]));	// get fat32 ext flag
			bpbp->bpb_xrootclst = getlong(&(bp[0x2C]));	// get root cluster
			bpbp->bpb_xfsinfosec = getword(&(bp[0x30]));// get fs info size
			bpbp->bpb_xbackupsec = getword(&(bp[0x32]));// get fat32 backup boot sector number
		  #endif
		} else { 
			bpbp->bpb_xnfsect = 0;
			bpbp->bpb_xrootclst = 0;
			bpbp->bpb_xfsinfosec = 0;
			bpbp->bpb_xflags = 0;
			bpbp->bpb_xbackupsec = 0;
		}
	  #endif
	  #if WITHEXFAT 
        bpbp->bpb_exmapbit=0;		  //cluster start sector
		//bpbp->bpb_exvolflags=0;
		bpbp->bpb_expertage=0;
  		bpbp->bpb_exFAT1Adr=0;
	   	bpbp->bpb_exFATLong=0;
		bpbp->bpb_exclucount=0;
		bpbp->bpb_exfsize=0;
      }
	  else
	  {
		    bpbp->bpb_nsize = getword(&(bp[0x13]));		  // get total sector count
			bpbp->bpb_nreserved = getword(&(bp[0x0E]));	  // get Reserved Sectors
			bpbp->bpb_ndirent = getword(&(bp[0x11])); 	  // get root entry count
			bpbp->bpb_huge = getlong(&(bp[0x20]));		  // get huge size in sectors	  
			bpbp->bpb_nfsect = getword(&(bp[0x16]));	  // get fat sector count
			bpbp->bpb_xfsinfosec = getword(&(bp[0x30]));  // get fs info size
			bpbp->bpb_xbackupsec = getword(&(bp[0x32]));  // get fat32 backup boot sector number


            bpbp->bpb_hidden = i; 	
			bpbp->bpb_exfsize=getlong(&(bp[0x48]));			
	  		bpbp->bpb_exFAT1Adr=getlong(&(bp[0x50]));
		   	bpbp->bpb_exFATLong=getlong(&(bp[0x54]));
		   	bpbp->bpb_xnfsect = getlong(&(bp[0x54]));     // get FAT32 FAT sectors		
			bpbp->bpb_exmapbit=getlong(&(bp[0x58]));		  //cluster start sector
			bpbp->bpb_exclucount=getlong(&(bp[0x5C]));
			bpbp->bpb_xrootclst = getlong(&(bp[0x60]));   // get root cluster
			bpbp->bpb_xflags = getword(&(bp[0x6A]));	  // get ext flag
			bpbp->bpb_nbyte = 1<<getword(&(bp[0x6C]));		  // get bytes per sector
			bpbp->bpb_nsector = 1<<bp[0x6D]; 				  // get sectors per cluster
			bpbp->bpb_nfat = bp[0x6E];					  // get FAT number					  
			bpbp->bpb_expertage=bp[0x70];




			//ExFAT


			//ExFAT get maptable ,up case.

			bpbp->bpb_exmtable=0;
			bpbp->bpb_exmlength=0;	  
						  
			bpbp->bpb_exuptable=0;
			bpbp->bpb_exuplength=0;

			exitflag=0;
			
			up_bound=(bpbp->bpb_exmapbit)+(bpbp->bpb_hidden)+((bpbp->bpb_xrootclst-2)<<bp[0x6D]);
			for(i=0;i<(bpbp->bpb_nsector);i++)
			{
				if (0 != dskxfer(dsk, (INT32U) i+up_bound, (INT32U) bp, 1, DSKREAD)) {
					return FALSE;
				}	
				
				for(j=0;j<16;j++)
				{
					if(bp[j*32]==0x81)		//maptable
					{
						bpbp->bpb_exmtable=getlong(&(bp[j*32+0x14]));
						bpbp->bpb_exmtable=bpbp->bpb_exmapbit+bpbp->bpb_hidden+((bpbp->bpb_exmtable-2)*bpbp->bpb_nsector);//sector
						bpbp->bpb_exmlength=getlong(&(bp[j*32+0x18]));
						exitflag|=0x01;
					}
					
					if(bp[j*32]==0x82)		//up case
					{
						bpbp->bpb_exuptable=getlong(&(bp[j*32+0x14]));
						bpbp->bpb_exuptable=bpbp->bpb_exmapbit+bpbp->bpb_hidden+((bpbp->bpb_exuptable-2)*bpbp->bpb_nsector);//sector
						bpbp->bpb_exuplength=getlong(&(bp[j*32+0x18]));
						exitflag|=0x02;
					}	

					if(exitflag==0x03)
					{
						break;
					}
				}
				
				if(exitflag==0x03)
				{
					break;
				}			
			}	
			//备用方案，防止目录出问题，导致不能使用maptable.这里使用大多数情况的默认地址,如果还是木有用，就木有办法了
			if((bpbp->bpb_exmlength==0))
			{
				bpbp->bpb_exmtable=(bpbp->bpb_exmapbit)+(bpbp->bpb_hidden);//sector
				bpbp->bpb_exmlength=(bpbp->bpb_exclucount-2)+7/8;
			}

			if((bpbp->bpb_exuptable==0))
			{
				bpbp->bpb_exuplength=0;
			}	
	  }
	  #endif
	  
		// For sector size more than 512
		nsector = bpbp->bpb_nbyte >> 9;
		for (shftcnt=0; (nsector>>shftcnt)>1; shftcnt++) ;
		if (shftcnt) {
			bpbp->bpb_nbyte = 512;
			bpbp->bpb_nsector <<= shftcnt; 	
			bpbp->bpb_nreserved <<= shftcnt;												
			bpbp->bpb_nsize <<= shftcnt;
			bpbp->bpb_nfsect <<= shftcnt;
		 	bpbp->bpb_huge <<= shftcnt;
		 	
		 	if (bFAT32) {
				bpbp->bpb_xnfsect <<= shftcnt;
				bpbp->bpb_xfsinfosec <<= shftcnt;
				bpbp->bpb_xbackupsec <<= shftcnt;
			}
		}
	}

	return(Flag);
} 
