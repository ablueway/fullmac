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
/* Converts a UTF-8 character to Unicode scalar value (same as UTF-32).  */
/* On success, returns the number of BYTEs taken by the character.       */
/* On failure, returns FD32_EUTF8.                                       */
/*                                                                       */
/* The conversion is done according to the following rules:              */
/*                                                                       */
/*           Scalar                               UTF-8                  */
/* 00000000 00000000 0xxxxxxx <-> 0xxxxxxx                               */
/* 00000000 00000yyy yyxxxxxx <-> 110yyyyy  10xxxxxx                     */
/* 00000000 zzzzyyyy yyxxxxxx <-> 1110zzzz  10yyyyyy  10xxxxxx           */
/* 000uuuuu zzzzyyyy yyxxxxxx <-> 11110uuu  10uuzzzz  10yyyyyy  10xxxxxx */
/*                                                                       */
/* NOTE: For optimization reasons, it is assumed that this function is   */
/* not called when the UTF-8 character is not multi-byte. In this case   */
/* the caller should process the single-byte character directly.         */
/**************************************************************************/
INT16S fd32_utf8to32(const UTF8 *s, UTF32 *Ch)
{
  /* "if 0" means that it's assumed that the UTF-8 character is multi-byte */
  #if 0
  if (!(*s & 0x80))
  {
    *Ch = *s;
    return 1;
  }
  #endif
  if ((*s & 0xE0) == 0xC0)
  {
    //*Ch = (*s++ & 0x8F) << 6;
    *Ch = (*s++ & 0x1F) << 6;//by matao 20051031
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch += *s++ & 0x3F;
    return 2;
  }
  if ((*s & 0xF0) == 0xE0)
  {
    *Ch = (*s++ & 0x0F) << 12;
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch += (*s++ & 0x3F) << 6;
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch += *s++ & 0x3F;
    return 3;
  }
  if ((*s & 0xF8) == 0xF0)
  {
    *Ch = (UTF32)(*s++ & 0x07) << 18;
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch = (UTF32)(*s++ & 0x3F) << 12;
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch += (*s++ & 0x3F) << 6;
    if ((*s & 0xC0) != 0x80) return FD32_EUTF8;
    *Ch += *s++ & 0x3F;
    return 4;
  }
  return FD32_EUTF8;
}

/**************************************************************************/
#endif // LFN_FLAG

