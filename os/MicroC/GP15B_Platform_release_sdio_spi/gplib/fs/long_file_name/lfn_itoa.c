/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "lfn.c" - Convert long file names in the standard DOS          *
 *                   directory entry format and generate short name       *
 *                   aliases from long file names                         *
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
INT8S *itoa(INT16S value, INT8S *string, INT16S radix)
{
  INT8S tmp[33];
  INT8S *tp = tmp;
  INT16S i;
  INT16U v;
  INT16S sign;
  INT8S *sp;

  if (radix > 36 || radix <= 1) return NULL;

  sign = (radix == 10 && value < 0);
  if (sign) v = -value;
       else v = (INT16U)value;

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10) *tp++ = i+'0';
           else *tp++ = i + 'a' - 10;
  }

  //if (string == 0) string = (char *) mem_get((tp-tmp)+sign+1);		//by wq 2004.7.13
  sp = string;

  if (sign) *sp++ = '-';
  while (tp > tmp) *sp++ = *--tp;
  *sp = 0;
  return string;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif

