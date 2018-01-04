/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "lfn.c" - Convert long file names in the standard DOS          *
 *                   directory entry format and generate short name       *
 *                   aliases from long file names                         *
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
/* Calculate the 8-bit checksum for the long file name from its */
/* corresponding short name.                                    */
/* Called by split_lfn and find (find.c).                       */
/**************************************************************************/
INT8S lfn_checksum(tDirEntry *D)
{
  INT16S Sum = 0, i;
  
	for (i = 0; i < 11; i++)
	{
		Sum = (((Sum & 1) << 7) | ((Sum & 0xFE) >> 1)) + D->Name[i];
	}

	return Sum & 0xFF; 	 //modify by zhangyan for unSP
}

/**************************************************************************/
/* Gets a origin short file name for 0xe5 is change to 0x05. */
/**************************************************************************/
INT16S fd32_get_origin_short_name(CHAR *Dest, CHAR *Source)
{
	INT16S i;
	
	if (*Source == 0x05)
		*Dest++ = (CHAR) 0xE5;
	else
		*Dest++ = (CHAR) *Source;
	Source++;
	for (i = 1; i < 11; i++)
	{
		*Dest++ = (CHAR) *Source++;
	}
	*Dest = 0;

	return 0;
}
#if WITHEXFAT ==1 
/**************************************************************************/
/* ExFAT Hash */
/**************************************************************************/
INT16U NameHash(UTF16 *FileName)
{
	INT16U Hash=0,index=0,Temp;
	INT8U *P=(INT8U *)FileName;
	while(FileName[index/2])
	{
		Temp=FileName[index/2];
		if((Temp>='a')&&(Temp<='z'))
		{
			Temp=Temp-'a'+'A';
			FileName[index/2]=Temp;
		}
	
		Hash=((Hash&1)?0x8000:0)+(Hash>>1)+(INT16U)P[index];
		index++;
		Hash=((Hash&1)?0x8000:0)+(Hash>>1)+(INT16U)P[index];
		index++;
	}
	return Hash;
}

#endif

#endif // LFN_FLAG

