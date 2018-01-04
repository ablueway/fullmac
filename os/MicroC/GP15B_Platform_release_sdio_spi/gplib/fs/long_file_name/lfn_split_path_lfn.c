/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "open.c" - Open, create and close a file (or even a directory  *
 *                    as a file) in any directory, allocating and freeing *
 *                    handles                                             *
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
/* Splits a full valid path name (path + file name) into its path and  */
/* file name components.                                               */
/* Called by fat_open, fat_unlink (creat.c), fsvol_lfn_findfirst       */
/* (find.c), fsvol_dos_findfirst (find.c) and fsvol_rename (rename.c). */
/* TODO: Fix list of callers and name... */
/**************************************************************************/
void split_path_LFN(CHAR *FullPath, CHAR *Path, CHAR *Name)
{
	CHAR *plast, *pnow;

	Path[0] = '\0';
	Name[0] = '\0';

	// 寻找文本串中最后一个'\\'的位置，并保存到plast中
	pnow = FullPath;
	plast = NULL;

	for ( ; ; plast = pnow, pnow++) {
		pnow = strchr(pnow, '\\');
		
		if (pnow == (CHAR *) 0) {
			break;
		}
	}

	if (plast == NULL) {
		strcpy(Name, FullPath);
		return;		
	}

	memcpy(Path, FullPath, plast-FullPath+1);
	Path[plast-FullPath+1] = '\0';
		
	// 判断是否需要将目录串的最后一个'\\'去除
	if (plast > FullPath) {
		Path[plast-FullPath] = '\0';
	}

	// 进行文件名的复制
	strcpy(Name, plast+1);		

	return;
}

#endif 	// LFN_FLAG