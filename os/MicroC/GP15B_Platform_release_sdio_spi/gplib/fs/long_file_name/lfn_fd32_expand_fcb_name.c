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
/* Gets a UTF-8 short file name from an FCB file name.                    */
/* 从短目录项里面获取短文件名，并转换为utf8的格式                         */
/**************************************************************************/
INT16S fd32_expand_fcb_name(INT8S *Dest, INT8S *Source)
{
	INT8S *NameEnd;
	INT8S *ExtEnd;
	INT8S  Aux[13];
	INT8S *s = Source;
	INT8S *a = Aux;
	
	/* Count padding spaces at the end of the name and the extension */
	for (NameEnd = Source + 7;  *NameEnd == ' '; NameEnd--);
	for (ExtEnd  = Source + 10; *ExtEnd  == ' '; ExtEnd--);
	
	/* Put the name, a dot and the extension in Aux */
	if (*s == 0x05) 
		*a++ = (INT8S) 0xE5, s++;
	for (; s <= NameEnd; *a++ = (INT8S) *s++);
	if (Source + 8 <= ExtEnd) 
		*a++ = '.';
	for (s = Source + 8; s <= ExtEnd; *a++ = (INT8S) *s++);
	*a = 0;
	
	/* And finally convert Aux from the OEM code page to UTF-8 */
	return oemcp_to_utf8(Aux, (UTF8 *) Dest);
}

/**************************************************************************/
#endif // LFN_FLAG



