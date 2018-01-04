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
/* Searches an open directory for NumEntries free consecutive entries. */
/* On success, returns the not negative byte offset of the first free  */
/* entry. Returns a negative error code on failure.                    */
/* Called by allocate_sfn_dir_entry and allocate_lfn_dir_entries.      */
/**************************************************************************/
INT16S find_empty_dir_entries(f_node_ptr fnp, INT16S NumEntries)
{
	INT16S		Found = 0; /* Number of consecutive free entries found */
	INT16S		EntryOffset = 0;
	INT16S		Res;
	tDirEntry	E;
	
	/* Scans the whole directory for free entries */
	fnp->f_diroff = 0;
	Res = dir_lfnread(fnp, &E);	
	if (Res < 0) 
		return Res;
	
	while (Res > 0)
	{
		if (((E.Name[0] == (INT8S)FREEENT)&&(!ISEXFAT(fnp->f_dpb))) || ( E.Name[0] == (INT8S)ENDOFDIR )||(((E.Name[0]&0x80) == (INT8S)0x00)&&(ISEXFAT(fnp->f_dpb))))
		{
			if (Found > 0) 
			{
				Found++;
			}
			else
			{
				Found = 1;
				EntryOffset = fnp->f_diroff; 		
			}
		}
		else 
		{
			Found = 0;
		}


		
		if (Found == NumEntries)
		{
			fnp->f_diroff = EntryOffset; 		
			return EntryOffset;
		}
		
		fnp->f_diroff++; 		
		Res = dir_lfnread(fnp, &E);
		
		if (Res < 0) 
			return Res;
	}
	
	/* At this point the directory file is at Eof. If possible, we need to */
	/* extend the directory writing a whole cluster filled with zeroes,    */
	/* next we return back to the beginning of that cluster.               */
	/* A FAT12/FAT16 root directory cannot be extended */
	/* A directory cannot have more than 65536 entries */
	/* The following write could fail if disk is full */
	return EntryOffset;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif



