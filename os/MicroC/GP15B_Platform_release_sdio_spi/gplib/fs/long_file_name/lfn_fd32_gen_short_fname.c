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
/* Generates a valid 8.3 file name from a valid long file name. */
/* The generated short name has not a numeric tail.             */
/* Returns 0 on success, or a negative error code on failure.   */
/**************************************************************************/
INT16S fd32_gen_short_fname(CHAR *Dest, CHAR *Source, INT32S Flags)
{
	INT8S  	ShortName[11];// = "           ";
#if MALLOC_USE == 1
	CHAR	*Purified; /* A INT32S name without invalid 8.3 characters */
#elif USE_GLOBAL_VAR == 1
	CHAR	*Purified = (CHAR *) gFS_path1;
#else
	STATIC CHAR	Purified[FD32_LFNPMAX]; /* A INT32S name without invalid 8.3 characters */
#endif
	CHAR 	*DotPos = NULL; /* Position of the last embedded dot in Source */
	CHAR 	*s;
	INT16S	Res = 0;

	/* Find the last embedded dot, if present */
	if (!(*Source)) return FD32_EFORMAT;
	for (s = Source + 1; *s; s++)
		if ((*s == '.') && (*(s-1) != '.') && (*(s+1) != '.') && *(s+1))
			DotPos = s;

	//guili 2006.5.25 start
	if (check_ifis_lfn((UTF8 *) Source, DotPos ? DotPos - Source : -1, 8) )
		Res |= FD32_GENSFN_WAS_INVALID;					//表示要生成长文件名
	if (DotPos) if (check_ifis_lfn((UTF8 *) DotPos + 1, -1,3) )
		Res |= FD32_GENSFN_WAS_INVALID;
	//guili 2006.5.25  end

#if MALLOC_USE == 1
	Purified = (CHAR *) FS_MEM_ALLOC(FD32_LFNPMAX);
#endif

	/* Convert all characters of Source that are invalid for short names */
	for (s = Purified; *Source;)
	{
		if (!(*Source & 0x80))
		{
			if ((*Source >= 'a') && (*Source <= 'z'))
			{
				*s++ = *Source + 'A' - 'a';
				Res |= FD32_GENSFN_CASE_CHANGED;
			}
			else if (*Source < 0x20)
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
			#endif
				return FD32_EFORMAT;
			}
			else switch (*Source)
			{
				/* Spaces and periods must be removed */
				case ' ': 
					Res |= FD32_GENSFN_WAS_INVALID;		//07.12.24 add 
					break;
				
				case '.': 
					if (Source == DotPos) 
						DotPos = s, *s++ = '.';
					else 
						Res |= FD32_GENSFN_WAS_INVALID;
					break;
					
				/* +, ; = [ ] are valid for LFN but not for short names */
				case '+': case ',': case ';': case '=': case '[': case ']':
					*s++ = '_'; 
					Res |= FD32_GENSFN_WAS_INVALID; 
					break;
				
				/* Check for invalid LFN characters */
				case '"': case '*' : case '/': case ':': case '<': case '>':
				case '?': case '\\': case '|':
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
				#endif
					return FD32_EFORMAT;
				}
				
				/* Return any other single-byte character unchanged */
				default : 
					*s++ = *Source;
			}
			Source++;
		}
		else /* Process extended characters */
		{
			UTF32 Ch, upCh;
			INT16S Skip;
			
			if ((Skip = fd32_utf8to32((UTF8 *) Source, &Ch)) < 0)
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
			#endif
				return FD32_EUTF8;
			}
			Ch &= 0x0000ffff; 				 //by wq 2004.9.29
			Source += Skip;
			upCh = unicode_toupper(Ch);
			if (upCh != Ch) Res |= FD32_GENSFN_CASE_CHANGED;
			if (upCh < 0x80) Res |= FD32_GENSFN_WAS_INVALID;
			s += fd32_utf32to8(upCh, (UTF8 *) s);
			Res |= FD32_GENSFN_CASE_CHANGED; 		 //by wq 2004.9.29
		}
	}
	*s = 0;

	/* Convert the Purified name to the OEM code page in FCB format */
	/* TODO: Must report WAS_INVALID if an extended char maps to ASCII! */
	memset(ShortName, ' ', 11);
	if (utf8_to_oemcp((UTF8 *) Purified, DotPos ? DotPos - Purified : -1, (INT8S *) ShortName, 8))
		Res |= FD32_GENSFN_WAS_INVALID;
	
	if (DotPos) 
	{
		if (utf8_to_oemcp((UTF8 *) DotPos + 1, -1, (INT8S *) &ShortName[8], 3))
			Res |= FD32_GENSFN_WAS_INVALID;
	}
	
	if (ShortName[0] == ' ')
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
	#endif
		return FD32_EFORMAT;
	}
	
	if (ShortName[0] ==(CHAR) 0xE5) 
		Dest[0] = (CHAR) 0x05;

	/* Return the generated short name in the specified format */
	switch (Flags & FD32_GENSFN_FORMAT_MASK)
	{
		case FD32_GENSFN_FORMAT_FCB: 
			memcpy(Dest, ShortName, 11);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
		#endif
			return Res;
		
		case FD32_GENSFN_FORMAT_NORMAL: 
			fd32_expand_fcb_name((INT8S *) Dest, ShortName);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
		#endif
			return Res;
		
		default:
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *) Purified); 		 //add by zhangyan
		#endif
			return FD32_EINVAL;
		}
	}
}

/**************************************************************************/
#endif // LFN_FLAG
#endif //#ifndef READ_ONLY


