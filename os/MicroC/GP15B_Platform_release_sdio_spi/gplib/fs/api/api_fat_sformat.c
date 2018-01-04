#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _SMALL_DISK_FORMAT == 1

/***************************************************************************/
extern struct DSKSZTOSECPERCLUS const DskTableFAT16[];
extern struct DSKSZTOSECPERCLUS const DskTableFAT32[]; 

//if disk size > 512M, FAT32 is better than FAT16.
/***************************************************************************/
BOOLEAN FAT16_sFormat(INT16S dsk, INT32U totalsectors, INT32U realsectors, INT8U *buf)
{
	INT32U TempValue;
	INT16U nfsect;
	INT16U nsector, FATStartSector, RootEndSector;
#ifdef	unSP
	INT16U *bp = (INT16U *)buf;
#else
	INT8U *bp = (INT8U *)buf;
#endif
	INT16U i, j;
	INT16U FATEndSector;
	INT32U sector_offset = 0;
	INT32U sector_number = totalsectors - 16;

	if (realsectors == 0)
		return FALSE;		

	for (i=0; i<8; i++){
		if (sector_number <= DskTableFAT16[i].DiskSize) {
			nsector = DskTableFAT16[i].SecPerClusVal;
			if (nsector == 0){
				return FALSE;				 
			}
			break;
		}
	}

	fmemset(bp, 0, 512>>WORD_PACK_VALUE);
	
	TempValue = sector_number / nsector;
	
	nfsect = ( INT16U)((TempValue+2+255)>>8);

#ifdef	unSP
	bp[0] =  0x3ceb;
	bp[1] =  0x90+('M'<<8);
	bp[2] =  'S'+('W'<<8);
	bp[3] =  'I'+('N'<<8);
	bp[4] =  '4'+('.'<<8);
	bp[5] =  '1'+(0<<8);
	bp[6] =  0x02+(nsector<<8); 		 //DBR_nbyte_Addr 	 + 	 DBR_nsector_Addr	 
#else
	bp[0] = 0xeb, bp[1] = 0x3c, bp[2] = 0x90;	// 0xEB 3C 90
	strcpy((CHAR *) bp+3, "MSWIN4.1");				// MSWIN4.1
	bp[12] = 0x02;								// 0x0200=512 bytes per sector
	bp[13] = nsector;
#endif

	putword(&bp[DBR_nreserved_Addr>>WORD_PACK_VALUE], 0x0001);

#ifdef	unSP
	bp[8] = 0x02; 		 //DBR_nfat_Addr 	 = 0x02
	bp[9] = 0x0002; 	 //DBR_ndirent_Addr= 0x0200
	bp[10] = 0xf800; 	 // DBR_mdesc_Addr = 0xf8	 
#else
	putword(&bp[16], 0x02);
	putword(&bp[18], 0x0002);
	putword(&bp[20], 0xf800);
#endif

	if (sector_number > 65535 )		
	{
		putword(&bp[DBR_nsize_Addr], 0x0000);
		putlong(&bp[DBR_huge_Addr>>WORD_PACK_VALUE], sector_number);
	}
	else
	{
	#ifdef unSP	
		bp[9] |= (sector_number & 0xFF)<<8; 	 //DBR_nsize_Addr
		bp[10] |= (sector_number >> 8) & 0xFF; 	 //DBR_nsize_Addr
	#else
		bp[19] = sector_number & 0xFF;
		bp[20] = (sector_number >> 8) & 0xFF;	
	#endif
		putlong(&bp[DBR_huge_Addr>>WORD_PACK_VALUE], 0x00000000);		 
	}
	 
	putword(&bp[DBR_nfsect_Addr>>WORD_PACK_VALUE], nfsect);
	putlong(&bp[DBR_hidden_Addr>>WORD_PACK_VALUE], sector_offset);

#ifdef	unSP
	bp[19] =  0x29 + (0x14<<8);
	bp[20] =  0x10+(0x04<<8);
	bp[21] =  0x20+('N'<<8);
	bp[22] =  'O'+(' '<<8);
	bp[23] =  'N'+('A'<<8);
	bp[24] =  'M'+('E'<<8);
	bp[25] =  ' '+(' '<<8);
	bp[26] =  ' '+(' '<<8);	 
	bp[27] =  'F'+('A'<<8);
	bp[28] =  'T'+('1'<<8);
	bp[29] =  '6';	 
#else
	bp[38] = 0x29;
	putlong(&bp[DBR_FAT16_VOL_ID], 0x20041014);
	memcpy(&bp[DBR_FAT16_VOL_LAB], "NO NAME    ", 11);	
	memcpy(&bp[DBR_FAT16_FilSysType], "FAT16", 5);
#endif

	bp[36>>WORD_PACK_VALUE] = 0x80;  
		
	putword(&bp[DBR_end_Addr>>WORD_PACK_VALUE], 0xaa55);

	if ( 0 != dskxfer(dsk, sector_offset, (INT32U)bp, 1, DSKWRITE) )
	{
		return FALSE;
	}
	
	FATStartSector = 1+sector_offset; 
	RootEndSector = FATStartSector + (nfsect<<1)+(0x0200*32+(512-1))/512;


	FATEndSector = FATStartSector + nfsect;
	j = (realsectors - RootEndSector) / nsector + 2;
	memset(bp, 0, 512>>WORD_PACK_VALUE);
	for (i=FATStartSector; i<FATStartSector + j / (256 >> WORD_PACK_VALUE); i++)
	{		
		if ( (i == FATStartSector) || (i == FATStartSector+nfsect) ) 
		{
		#ifdef unSP
			bp[0] = 0xfff8; 
	 		bp[1] = 0xffff;	
	 	#else
			memset(bp, 0xff, 4);
			bp[0] = 0xf8;
		#endif
		}
		else
		{
		#ifdef unSP
			bp[0] = 0; 
	 		bp[1] = 0;	
	 	#else
			memset(bp, 0, 4);
		#endif
		}
		
		if ( 0 != dskxfer(dsk, i, (INT32U)bp, 1, DSKWRITE) )
		{
			return FALSE;			
		}
		if ( 0 != dskxfer(dsk, i + nfsect, (INT32U)bp, 1, DSKWRITE) )
		{
			return FALSE;			
		}
	}
#ifdef unSP
	memset(bp, 0, j % 256);			// Tristan: need to be modified for 32 bit system
	memset(bp + j % 256, 0xfff7, 256 - j % 256);
#else
	fs_memset_word(bp, 0, j % 256);
	fs_memset_word(bp + ((j % 256) << 1), 0xfff7, 256 - j % 256);
#endif
	if ( (i == FATStartSector) || (i == FATStartSector+nfsect) ) 
	{
	#ifdef unSP
		bp[0] = 0xfff8; 
 		bp[1] = 0xffff;	
 	#else
		memset(bp, 0xff, 4);
		bp[0] = 0xf8;
	#endif
	}
	if ( 0 != dskxfer(dsk, i, (INT32U)bp, 1, DSKWRITE) )
	{
		return FALSE;			
	}
	if ( 0 != dskxfer(dsk, i + nfsect, (INT32U)bp, 1, DSKWRITE) )
	{
		return FALSE;			
	}
	i++;
#ifdef unSP
	memset(bp, 0xfff7, 512>>WORD_PACK_VALUE);
#else
	fs_memset_word(bp, 0xfff7, 256);
#endif
	for (; i<FATEndSector; i++)
	{			
		if ( 0 != dskxfer(dsk, i, (INT32U)bp, 1, DSKWRITE) )
		{
			return FALSE;			
		}
		if ( 0 != dskxfer(dsk, i + nfsect, (INT32U)bp, 1, DSKWRITE) )
		{
			return FALSE;			
		}
	}
	memset(bp, 0, 512>>WORD_PACK_VALUE);
	for (i+=nfsect; i<RootEndSector; i++)
	{			
		if ( 0 != dskxfer(dsk, i, (INT32U)bp, 1, DSKWRITE) )
		{
			return FALSE;			
		}
	}

	return TRUE; 
}


/***************************************************************************/
INT16S fat_sformat (INT16S dsk, INT32U totalsectors, INT32U realsectors) 
{
	INT8U *bp;
	BOOLEAN val = FALSE; 
	INT16S ret;

	if (dsk>= MAX_DISK_NUM)
		return DE_INVLDACC;
	
	if (!(FileSysDrv[dsk]->Status))
		return DE_INVLDDRV;
	
	if (!flush1(firstbuf))
		return DE_INVLDBUF;
	
	firstbuf->b_flag &= ~BFR_VALID;
	
	bp = firstbuf->b_buffer;
	
	clear_buffers(dsk); 
		
	val = FAT16_sFormat(dsk,totalsectors, realsectors, bp); 			

	flush_buffers(dsk); 
	
	if (val==TRUE)
	{
		ret = fs_mount(dsk);
	
		if (ret!=SUCCESS)
		{
			return ret;
		}
	}
	
	if (val==FALSE)
	{
		return DE_INVLDACC;
	}
	
	return SUCCESS;
}
#endif
#endif //READ_ONLY

