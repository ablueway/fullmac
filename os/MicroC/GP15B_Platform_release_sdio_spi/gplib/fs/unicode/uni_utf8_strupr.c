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

#ifndef READ_ONLY
#if LFN_FLAG == 1
/**************************************************************************/
/* Converts a UTF-8 string to upper case.                          */
/* Returns 0 if case has not been changed (already in upper case), */
/* a positive number if case has been changed or FD32_EUTF8 if an  */
/* invalid UTF-8 sequence is found.                                */
/**************************************************************************/
INT16S utf8_strupr(UTF8 *Dest, const UTF8 *Source)
{
  UTF32 Ch, upCh;
  INT16S   CaseChanged = 0; /* Case not changed by default */
  INT16S   Res;

  for (;;)
  {
    /* Try to do it faster on single-byte characters */
    if (!(*Source & 0x80))
    {
      if ((*Source >= 'a') && (*Source <= 'z'))
      {
        CaseChanged = 1;
        *Dest++ = *Source++ + 'A' - 'a';
      }
      else if ((*Dest++ = *Source++) == 0x00) return CaseChanged;
    }
    else /* Process extended characters */
    {
      if ((Res = fd32_utf8to32(Source, &Ch)) < 0) return FD32_EUTF8;
      Source += Res;
      upCh = unicode_toupper(Ch);
      if (upCh != Ch) CaseChanged = 1;
      Dest += fd32_utf32to8(upCh, Dest);
    }
  }
}

#endif // LFN_FLAG
#endif
