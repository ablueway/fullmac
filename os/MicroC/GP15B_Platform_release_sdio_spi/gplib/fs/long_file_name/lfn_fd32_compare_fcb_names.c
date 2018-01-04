/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "names.c" - Generate short (8.3) name alias for long file      *
 *                     names and manage FCB-format names.                 *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 File System Layer (the SOFTWARE).   *
 *                                                                        *
 * The SOFTWARE is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or (at  *
 * your option) any later version.                                        *
 *                                                                        *
 * The SOFTWARE is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the SOFTWARE; see the file GPL.txt;                         *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/**************************************************************************/
/* Compares two file names in FCB format. Name2 may contain '?' wildcards.*/
/* Returns zero if the names match, or nonzero if they don't match.       */
/* TODO: Works only on single byte character sets!                        */
/**************************************************************************/
INT16S fd32_compare_fcb_names(INT8S *Name1, INT8S *Name2)
{
	INT16S k;
	
	for (k = 0; k < 11; k++)
	{
		if (Name2[k] == '?') continue;
		if (Name1[k] != Name2[k]) return -1;
	}
	return 0;
}

/**************************************************************************/
#endif // LFN_FLAG

