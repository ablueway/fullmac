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
/* Allocates one entry of the specified open directory and fills it    */
/* with the directory entry D.                                         */
/* On success, returns the byte offset of the allocated entry.         */
/* On failure, returns a negative error code.                          */
/* Called by allocate_lfn_dir_entry (if not LFN entries are required), */
/* and, ifndef FATLFN, by fat_creat and fat_rename.                    */
/**************************************************************************/
INT16S allocate_sfn_dir_entry(f_node_ptr fnp, tDirEntry *D, CHAR *FileName)
{
	INT16S Res, EntryOffset;

	/* Get the name in FCB format, wildcards not allowed */
	if (fd32_build_fcb_name((CHAR *) D->Name, FileName)) 
		return FD32_EFORMAT;
    
	/* Search for a free directory entry, extending the dir if needed */
	EntryOffset = find_empty_dir_entries(fnp, 1); 
	/* Write the new directory entry into the free entry */
	fnp->f_flags |= F_DMOD;

	Res = dir_lfnwrite(fnp, D);
	if (Res < 0) return Res;
		return 0; 
}

/**************************************************************************/
#endif // LFN_FLAG
#endif	// #ifndef READ_ONLY

