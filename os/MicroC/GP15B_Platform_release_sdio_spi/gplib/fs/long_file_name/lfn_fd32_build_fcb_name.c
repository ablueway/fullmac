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

#ifndef READ_ONLY
#if LFN_FLAG == 1
/**************************************************************************/
/* Given a file name Source in UTF-8, checks if it's valid 				  */
/* and returns in Dest the file name in FCB format.        				  */
/* On success, returns 0 if no wildcards are present, or a 				  */
/* positive number if '?' wildcards are present in Dest.   				  */
/* On failure, returns a negative error code.             			 	  */
/**************************************************************************/
INT16S fd32_build_fcb_name(CHAR *Dest, CHAR *Source)
{
	INT16S   WildCards = 0;
#if MALLOC_USE == 1
	INT8S * 	 	SourceU;
#elif USE_GLOBAL_VAR == 1
	INT8S *		SourceU = gFS_path1;
#else
	STATIC INT8S	SourceU[FD32_LFNPMAX];
#endif
	INT16S   Res;
	INT16S   k;

	/* Name and extension have to be padded with spaces */
	memset(Dest, 0x20, 11);
	
	/* Build ".          " and "..         " if Source is "." or ".." */
	if ((strcmp(Source, ".") == 0) || (strcmp(Source, "..") == 0))
	{
		for (; *Source; Source++, Dest++)
			*Dest = *Source;	
		return 0;
	}
  
#if MALLOC_USE == 1
	SourceU =  (INT8S *)FS_MEM_ALLOC(FD32_LFNPMAX);
#endif

	if ((Res = utf8_strupr((UTF8 *) SourceU, (const UTF8 *) Source)) < 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)SourceU); 		 //add by zhangyan
	#endif
		return FD32_EUTF8;
	}
	
	for (k = 0; (SourceU[k] != '.') && SourceU[k]; k++);
	utf8_to_oemcp((UTF8 *) SourceU, SourceU[k] ? k : -1, (INT8S *) Dest, 8);
	if (SourceU[k]) 
		utf8_to_oemcp((UTF8 *) &SourceU[k + 1], -1, (INT8S *) &Dest[8], 3);

	if (Dest[0] == ' ')
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)SourceU); 		 //add by zhangyan
	#endif
		return FD32_EFORMAT;
	}
	if (Dest[0] ==(CHAR) 0xE5) 
		Dest[0] = (CHAR) 0x05;
		
	for (k = 0; k < 11;)
	{
		if (Dest[k] < 0x20)		// If Dest is singed char, a value larger than 0x7F is a negative value
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)SourceU); 		 //add by zhangyan
		#endif
			return FD32_EFORMAT;
		}
		switch (Dest[k])
		{
			case '"': 
			case '+': 
			case ',': 
			case '.':
			case '/': 
			case ':':
			case ';':
			case '<': 
			case '=':
			case '>': 
			case '[': 
			case '\\':
			case ']':
			case '|':
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)SourceU); 		 //add by zhangyan
			#endif
				return FD32_EFORMAT;
			}
			case '?': 
				WildCards = 1;
				k++;
				break;
				
			case '*': WildCards = 1;
				if (k < 8) 
					for (; k < 8; k++) 
						Dest[k] = '?';
				else 
					for (; k < 11; k++) 
						Dest[k] = '?';
				break;
				
			default : 
				k += 1; // zza oemcp_skipchar((INT8S *) &Dest[k]);
		}
	}
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)SourceU); 
#endif
	return WildCards;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif //#ifndef READ_ONLY

