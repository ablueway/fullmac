/**************************************************************************
 * FreeDOS 32 Unicode support library                                     *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This file is part of FreeDOS 32                                        *
 *                                                                        *
 * FreeDOS 32 is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * FreeDOS 32 is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with FreeDOS 32; see the file GPL.txt; if not, write to          *
 * the Free Software Foundation, Inc.                                     *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/**************************************************************************/
/* Compares two Unicode UTF-8 strings, disregarding case.                */
/* Returns 0 if the strings match, a positive value if they don't match, */
/* or a FD32_EUTF8 if an invalid UTF-8 sequence is found.                */
/**************************************************************************/
INT16S utf8_stricmp(const UTF8 *s1, const UTF8 *s2)
{
	for (;;)
	{
		if (!(*s2 & 0x80))
		{
			if (fs_toupper(*s1) != fs_toupper(*s2))
				return 1; 					
			if (*s1 == 0)
				return 0;
			s1++;
			s2++;
		}
		else
		{
			if (*s1 != *s2)
				return 1; 				  
			if (*s1 == 0)
				return 0;		  
			s1++;
			s2++;
		}
	}
}

/**************************************************************************/
#endif // LFN_FLAG

