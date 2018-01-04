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
INT8U fs_toupper(INT8U c)
{
    if (c >= 'a' && c <= 'z') return(c-'a'+'A');
    else return(c);
}

INT8U fs_tolower(INT8U c) 	
{
	if (c >= 'A' && c <= 'Z')
		return (c + 'a' - 'A');
	else
		return (c);
}

INT8S * fs_strlwr(INT8S * upin)
{
	INT8S * up;
	
	up = upin;
	while (*up) {		/* For all characters */
		*up = fs_tolower(*up);
		up++;
	}
	return upin;		/* Return input pointer */
}

INT8S * fs_strupr(INT8S * lowin)
{
	INT8S * low;
	
	low = lowin;
	while (*low) {		/* For all characters */
		*low = fs_toupper(*low);
		low++;
	}
	return lowin;		/* Return input pointer */
}



/**************************************************************************/
#endif // LFN_FLAG


