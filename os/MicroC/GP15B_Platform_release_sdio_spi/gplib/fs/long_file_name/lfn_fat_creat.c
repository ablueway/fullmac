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
/* Creates a zero length file, allocating a new directory entry, in */
/* the directory specified by the file structure Fp, and keeps the  */
/* file open in the file structure Ff.                              */
/* Returns 0 on success, or a negative error code on failure.       */
/* Called by fat_open.                                              */
/**************************************************************************/
INT16S fat_creat(f_node_ptr fnp, INT8S *Name, INT8S Attr)
{
	INT16S       Res;
	tDirEntry	D;
	
	/* Initialize the directory entry */
	memset((void*)&D, 0, sizeof(tDirEntry));

  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		init_ExFatdirentry((struct dirent *)&D, Attr, FREE);
	}
	else
  #endif	
  	{
		init_direntry((struct dirent *)&D, Attr, FREE);
  	}

	 
	/* Switch the access mode to READWRITE for the following operation */
#ifdef FATLFN
  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
  		Res = allocate_exfat_dir_entries(fnp, &D, (CHAR *) Name);
  	else
  #endif
		Res = allocate_lfn_dir_entries(fnp, &D, (CHAR *) Name);
#else
	Res = allocate_sfn_dir_entry(fnp, &D, (CHAR *) Name);
#endif
	if (Res < 0) return Res;
	memcpy((void*)&fnp->f_dir, (void*)&D, sizeof(tDirEntry));
#if WITHEXFAT == 1
	fnp->ext_dir=( ExFATExtDIR * )&fnp->f_dir;
#endif
		
	return 0;
}

#endif // LFN_FLAG
#endif //#ifndef READ_ONLY

