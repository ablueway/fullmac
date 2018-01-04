/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "creat.c" - File creation and deletion services                *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 1
/**************************************************************************/
/* Allocates 32-bytes directory entries of the specified open directory */
/* to store a long file name, using D as directory entry for the short  */
/* name alias.                                                          */
/* On success, returns the byte offset of the short name entry.         */
/* On failure, returns a negative error code.                           */
/* Called fat_creat and fat_rename.                                     */
/**************************************************************************/
INT16S allocate_lfn_dir_entries(f_node_ptr fnp, tDirEntry *D, CHAR *FileName)
{ 
	CHAR		ShortName[18]; 	 //larger buffer, modify by zhangyan 051031
#if MALLOC_USE == 1
	tLfnEntry*	Slot;
#elif USE_GLOBAL_VAR == 1
	tLfnEntry*	Slot = (tLfnEntry*)gFS_slots;
#else
	STATIC tLfnEntry	Slot[20];
#endif
#ifdef unSP
	tDirEntry	DirEnt;
#endif
	INT16S       EntryOffset;
	INT16S       LfnEntries;
	INT16S       Res;
	INT16S       k;

	/* gen_short_fname already checks if the LFN is valid */
	/* if (!lfn_is_valid(FileName)) return FD32_EFORMAT;  */
	/* TODO: gen_short_fname cross reference without fat_ prefix */
	Res = gen_short_fname(fnp, FileName, ShortName);
	if (Res < 0) return Res;
	
	/* If Res == 0 no long name entries are required */
	if (Res == 0) return allocate_sfn_dir_entry(fnp, D, FileName);
	
	/* Generate a 32-byte Directory Entry for the short name alias */
	memcpy((void*)D->Name, (void*)ShortName, 11);
	
#if MALLOC_USE == 1
	Slot = (tLfnEntry*)FS_MEM_ALLOC(20*sizeof(tLfnEntry));
#endif
	
	/* Split the long name into 32-byte slots */
	Res = split_lfn(Slot, D, FileName, &LfnEntries);
	if (Res)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Slot);
	#endif
		return Res;
	}

	/* Search for NumSlots + 1 (LFN entries plus short name entry) */
	/* free directory entries, expanding the dir if needed.        */
	EntryOffset = find_empty_dir_entries(fnp, LfnEntries + 1);
//	if (EntryOffset < 0) 
//		return EntryOffset;
	
	/* Write the new directory entries into the free entries */
	fnp->f_flags |= F_DMOD; 
	
	for (k = LfnEntries - 1; k >= 0; k--)
	{
	#ifdef unSP
		putLfnEnt(&Slot[k], &DirEnt);
	    Res = dir_lfnwrite(fnp, &DirEnt);
	#else
	    Res = dir_lfnwrite(fnp, (tDirEntry *)&Slot[k]);
	#endif
	    if (Res < 0)
	    {
	#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Slot); 		 //add by zhangyan
	#endif
			return Res;
		}
	 	fnp->f_diroff += 1;
	}
	//EntryOffset = F->TargetPos;//by wq 2004.7.16
	Res = dir_lfnwrite(fnp, D);
	if (Res < 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Slot); 		 //add by zhangyan
	#endif
		return Res;
	}
//  return EntryOffset;	//m by zy 05-01-07
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Slot); 		 //add by zhangyan
#endif
	return 0;
}

/**************************************************************************/
#endif // LFN_FLAG

#if WITHEXFAT == 1

INT16S allocate_exfat_dir_entries(f_node_ptr fnp, tDirEntry *D, CHAR *FileName)
{ 

#if MALLOC_USE == 1
	tLfnEntry*	Slot;
#elif USE_GLOBAL_VAR == 1
	tLfnEntry*	Slot = (tLfnEntry*)gFS_slots;
#else
	STATIC tLfnEntry	Slot[LFN_FETCH_SLOTS];
#endif

#if MALLOC_USE == 1
		UTF16*			LfnUtf16;
#elif USE_GLOBAL_VAR == 1
		UTF16*			LfnUtf16 = (UTF16*)gFS_sFindData.LongName;
#else
		UTF16			LfnUtf16[FD32_LFNPMAX];
#endif
	INT16S       EntryOffset,Res,k=0,RetW;
	INT8U		 Namelong=0;
	INT16U		 exnamehash;
	
	ExFATMainDIR *MainDiR=(ExFATMainDIR *)D;
	ExFATExtDIR *ExtDTIR;
	ExFATNameDIR *NameDIR;

#if MALLOC_USE == 1
	Slot = (tLfnEntry*)FS_MEM_ALLOC(20*sizeof(tLfnEntry));
#endif
	memset((void *)Slot,0,LFN_FETCH_SLOTS*sizeof(tLfnEntry));
	ExtDTIR=(ExFATExtDIR *)&Slot[1];
	
	//begin
	memset(LfnUtf16,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));
	fd32_utf8to16((UTF8 *) FileName, (UTF16 *) LfnUtf16);
	//count namelong
	while(LfnUtf16[Namelong])
	{
		Namelong++;	
	}
	exnamehash= NameHash((UTF16 *) LfnUtf16);

	memset(LfnUtf16,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));
	fd32_utf8to16((UTF8 *) FileName, (UTF16 *) LfnUtf16);
	
	ExtDTIR->EntryType=0xC0;
	ExtDTIR->SecondaryFlags=0x01;
	ExtDTIR->NameLength=Namelong;
	ExtDTIR->NameHash=exnamehash;
	//文件名目录项个数
	Res=(Namelong+14)/15;
	MainDiR->SecondaryCount=Res+1;
	k=0;
	
	while(1)
	{
		NameDIR=(ExFATNameDIR *)&Slot[k+2];
		NameDIR->EntryType=0xC1;
		NameDIR->SecondaryFlags=0x00;
		memcpy((INT8U *)NameDIR->FileName,(INT8U *)LfnUtf16+30*k,30);
		k++;
		if(k>=Res)
		{
			break;
		}			
	}
	memcpy((void *)&Slot[0],MainDiR,32);

	
	/* free directory entries, expanding the dir if needed.  */
	EntryOffset = find_empty_dir_entries(fnp, Res+2);	
	/* Write the new directory entries into the free entries */
	fnp->f_flags |= F_DMOD; 
	EntryOffset=fnp->f_diroff;
	for (k = 0; k <(Res+2); k++)
	{
	    RetW = dir_lfnwrite(fnp, (tDirEntry *)&Slot[k]);
	    if (RetW < 0)
	    {
		  #if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Slot); 		 //add by zhangyan
		  #endif
			return RetW;
		}
	 	fnp->f_diroff += 1;
	}
	fnp->f_diroff = EntryOffset+1;				//统一指向扩展目录
	memcpy((void *)MainDiR,(void *)&Slot[1],32);
	memcpy((void *)&fnp->extmain_dir,(void *)&Slot[0],32);
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Slot); 		 //add by zhangyan
#endif
	return 0;
}

#endif
#endif //#ifndef READ_ONLY

