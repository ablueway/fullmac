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
/* Generate a valid 8.3 file name for a long file name, and makes sure */
/* the generated name is unique in the specified directory appending a */
/* "~Counter" if necessary.                                            */
/* Returns 0 if the passed long name is already a valid 8.3 file name  */
/* (including upper case), thus no extra directory entry is required.  */
/* Returns a positive number on successful generation of a short name  */
/* alias for a long file name which is not a valid 8.3 name.           */
/* Returns a negative error code on failure.                           */
/* Called by allocate_lfn_dir_entries (direntry.c).                    */
/**************************************************************************/
INT16S gen_short_fname(f_node_ptr fnp, CHAR *LongName, CHAR *ShortName)
{
  INT8S     Aux[11];
  CHAR		szCounter[6];
  INT16U    Counter, szCounterLen;
  INT16S    i, k, Res;
  CHAR     	*s;
  tDirEntry E;
  
	CHAR 	*DotPos = NULL; /* Position of the last embedded dot in Source */
	CHAR 	*s1;
	INT16U	name_len,ext_name_len;

  Res = fd32_gen_short_fname(ShortName, LongName, FD32_GENSFN_FORMAT_FCB);
  if (Res <= 0) return Res;
  /* TODO: Check case change! */
  if (!(Res & FD32_GENSFN_WAS_INVALID)) return 1;

#if 1  // Zurong 2011.3.1 修改小于等于8.3格式的文件名，不产生短文件名  	
	for (s1 = LongName + 1; *s1; s1++)
		if ((*s1 == '.') && (*(s1-1) != '.') && (*(s1+1) != '.') && *(s1+1))
			DotPos = s1;
			  	
	name_len = DotPos - LongName;
	ext_name_len = strlen(DotPos)-1;
	if((name_len<=8)&&(ext_name_len<=3))
	{
		return 1;
	}
#endif
  /* Now append "~Counter" to the short name, incrementing "Counter" */
  /* until the file name is unique in that Path                      */
  for (Counter = 1; Counter < 65535; Counter++)
  {
    memcpy(Aux, ShortName, 11);
    itoa(Counter, (INT8S *) szCounter, 10);
    szCounterLen = strlen(szCounter);
    //for (k = 0; k < 7 - szCounterLen; k += oemcp_skipchar(&Aux[k]));
// 07.12.24 modify
	for (k = 0; k < 7 - szCounterLen; )
	{
		i = oemcp_skipchar(&Aux[k]);
		if(i == 0)	
			break;
		k += i;
	}
// 07.12.24 modify end

	 /* Append the "~Counter" to the name */
    /* TODO: The "~Counter" shouldn't be right justified if name is shorter than 8! */
    Aux[k++] = '~';
    for (s = szCounter; *s; Aux[k++] = *s++);

    /* Search for the generated name */
	fnp->f_diroff = 0;        //add by matao 
	//start search from direntry 
    for (;;)
    {
      if ((Res = dir_lfnread(fnp, &E)) < 0)
      {
        memcpy(ShortName, Aux, 11); 	 // add by zy 2005-1-7
        return 1; 	 // add by zy 2005-1-7
      }
      if ((Res == 0) || (E.Name[0] == 0))
      {
        memcpy(ShortName, Aux, 11);
        return 1;
      }
      if (fd32_compare_fcb_names((INT8S*)E.Name, (INT8S*)Aux) == 0) break;
	  fnp->f_diroff ++; //add by matao
	  //search next direntry
    }
  }
  return FD32_EACCES;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif //#ifndef READ_ONLY

