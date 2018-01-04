/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "readdir.c" - Services to find a specified directory entry     *
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

#if LFN_FLAG == 1
/**************************************************************************/
/* Fetches a long file name before a short name entry, if present, from the */
/* specified Buffer of directory entries to a 16-bit character string.      */
/* Returns the number of directory entries used by the long name, or 0 if   */
/* no long name is present before the short name entry.                     */
/* Called by fat_readdir and find.                                          */
/**************************************************************************/

#ifdef unSP		//zhangyan added
INT16S fetch_lfn(tDirEntry *Buffer, INT16S BufferPos, INT16S *Name)
{
	tDirEntry	*pFakeLfnSlot  = &Buffer[BufferPos];
	INT8S		Checksum;
	INT16S		Order    = 0;
	INT16S		NamePos  = 0;
	INT16S		i,k;
	INT8S  		Aux[13];
	
	fd32_get_origin_short_name((CHAR *) &Aux, (CHAR *) &Buffer[BufferPos]); 	//2006.6.21
	Checksum = lfn_checksum((tDirEntry *) &Aux);
	 
	do
	{
		if (--pFakeLfnSlot < Buffer) pFakeLfnSlot += LFN_FETCH_SLOTS;
		if (pFakeLfnSlot->Attr != FD32_ALNGNAM) return 0;
		if (++Order != (pFakeLfnSlot->Name[0] & 0x1F)) return 0; 	 //Order check
		if (Checksum != pFakeLfnSlot->CrtTimeTenth) return 0;
		/* Ok, the LFN slot is valid, attach it to the INT32S name */
	
		Name[NamePos++] = (pFakeLfnSlot->Name[2]<<8)|pFakeLfnSlot->Name[1];
		Name[NamePos++] = (pFakeLfnSlot->Name[4]<<8)|pFakeLfnSlot->Name[3];
		Name[NamePos++] = (pFakeLfnSlot->Name[6]<<8)|pFakeLfnSlot->Name[5];
		Name[NamePos++] = (pFakeLfnSlot->Name[8]<<8)|pFakeLfnSlot->Name[7];
		Name[NamePos++] = (pFakeLfnSlot->Name[10]<<8)|pFakeLfnSlot->Name[9];
		
		Name[NamePos++] = pFakeLfnSlot->CrtTime;
		Name[NamePos++] = pFakeLfnSlot->CrtDate;
		Name[NamePos++] = pFakeLfnSlot->LstAccDate;
		Name[NamePos++] = pFakeLfnSlot->FstClusHI;
		Name[NamePos++] = pFakeLfnSlot->WrtTime;
		Name[NamePos++] = pFakeLfnSlot->WrtDate;
		
		Name[NamePos++] = *((INT16U *)&pFakeLfnSlot->FileSize);
		Name[NamePos++] = *(((INT16U *)&pFakeLfnSlot->FileSize)+1);
	} while (!(pFakeLfnSlot->Name[0] & 0x40)); 	 //Last Order
	 
	for (i=0x00, k=NamePos-1; i<13; i++)
	{
		if ( Name[k--]) { continue; }
		break;
	}	
	
	if (i>=13)
	{
		Name[NamePos] = 0x00;	  
	}
	
	return Order;
}
#else

/************************************************************************/
/* 
	history:
		逻辑设计有BUG， 中间没有判断NULL，最后又进行了补NULL的动作

		在极限条件下，会有内存写穿的情形发生!!!!
  
                                                                     */
/************************************************************************/
//从长目录项里面获取unicode格式的长文件名
INT16S fetch_lfn(tDirEntry *Buffer, INT16S BufferPos, INT16S *Name)
{
	tLfnEntry	*LfnSlot  = (tLfnEntry *) &Buffer[BufferPos];
	INT8S		Checksum;
	INT16S		Order    = 0;
	INT16S		NamePos  = 0;
	INT16S		i,k;
	INT8S  		Aux[13];
	
	fd32_get_origin_short_name((CHAR *) &Aux, (CHAR *) &Buffer[BufferPos]); 		//2006.6.21
	Checksum = lfn_checksum((tDirEntry *) &Aux);					 			//2006.6.21
	
	do
	{
		if (--LfnSlot < (tLfnEntry *) Buffer)
			LfnSlot += LFN_FETCH_SLOTS;	

		if (LfnSlot->Attr != FD32_ALNGNAM) 
			return 0;
		
		if (++Order != (LfnSlot->Order & 0x1F))
			return 0;
		
		if (Checksum != LfnSlot->Checksum)
			return 0;
		
		/* Ok, the LFN slot is valid, attach it to the long name */
		for (k = 0; k < 5; k++)
			Name[NamePos++] =  LfnSlot->Name0_4[k];		
		for (k = 0; k < 6; k++)
			Name[NamePos++] =  LfnSlot->Name5_10[k];		
		for (k = 0; k < 2; k++)
			Name[NamePos++] =  LfnSlot->Name11_12[k];
		
	} while (!(LfnSlot->Order & 0x40));

	for (i=0x00, k=NamePos-1; i<13; i++)
	{
		if ( Name[k--]) 
		{
			continue;
		}
		break;
	}	

	if (i>=13)
	{
		Name[NamePos] = 0x0000;	  		
	}	

	return Order;
}
#endif

/**************************************************************************/
#endif // LFN_FLAG

