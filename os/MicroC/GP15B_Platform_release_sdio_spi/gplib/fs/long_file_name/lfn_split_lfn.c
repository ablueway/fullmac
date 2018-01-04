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
/* Splits the long file name in the string LongName to fit in up to 20 */
/* LFN slots, using the short name of the dir entry D to compute the   */
/* short name checksum.                                                */
/* On success, returns 0 and fills the Slot array with LFN entries and */
/* NumSlots with the number of entries occupied.                       */
/* On failure, returns a negative error code.                          */
/* Called by allocate_lfn_dir_entries.                                 */
/**************************************************************************/
INT16S split_lfn(tLfnEntry *Slot, tDirEntry *D, CHAR *LongName, INT16S *NumSlots)
{
	INT16S   NamePos  = 0;
	INT16S   SlotPos  = 0;
	INT16S   Order    = 0;
	INT8S	Checksum = lfn_checksum(D);
#if MALLOC_USE == 1
	UTF16*			LfnUtf16;
#elif USE_GLOBAL_VAR == 1
	UTF16*			LfnUtf16 = (UTF16*)gFS_sFindData.LongName;
#else
	STATIC UTF16	LfnUtf16[FD32_LFNPMAX];
#endif
	
#if MALLOC_USE == 1
	LfnUtf16 = FS_MEM_ALLOC(FD32_LFNPMAX);
	if (LfnUtf16==NULL)
		return FD32_ENOMEM;
#endif

	/* Long file names are stored in UTF-16 */
	if (fd32_utf8to16((UTF8 *) LongName, (UTF16 *) LfnUtf16))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE(LfnUtf16); 
	#endif
		return FD32_EUTF8;
	}
  
	/* Initialize the first slot */
	Slot[0].Order    = 1;
	Slot[0].Attr     = FD32_ALNGNAM;
	Slot[0].Reserved = 0;
	Slot[0].Checksum = Checksum;
	Slot[0].FstClus  = 0;
	
	while (LfnUtf16[NamePos])
	{
		if (SlotPos == 13)
		{
			SlotPos = 0;
			Order++;
			Slot[Order].Order    = Order + 1; /* 1-based numeration */
			Slot[Order].Attr     = FD32_ALNGNAM;
			Slot[Order].Reserved = 0;
			Slot[Order].Checksum = Checksum;
			Slot[Order].FstClus  = 0;
		}
		copy_char_in_lfn_slot(&Slot[Order], SlotPos++, LfnUtf16[NamePos]);
		if (++NamePos == FD32_LFNMAX)
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE(LfnUtf16); 
		#endif
			return FD32_EFORMAT;
		}
	}
	/* Mark the slot as last */
	Slot[Order].Order |= 0x40;
	/* Insert an Unicode NULL terminator, only if it fits into the slots */
	copy_char_in_lfn_slot(&Slot[Order], SlotPos++, 0x0000);
	/* Pad the remaining characters of the slot with FFFFh */
	while (SlotPos < 13) 
		copy_char_in_lfn_slot(&Slot[Order], SlotPos++, 0xFFFF);
	
	*NumSlots = Order + 1;
#if MALLOC_USE == 1
	FS_MEM_FREE(LfnUtf16); 	
#endif
	return 0;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif

