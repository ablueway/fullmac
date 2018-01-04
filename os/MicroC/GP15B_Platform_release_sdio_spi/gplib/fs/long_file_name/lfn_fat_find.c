/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "readdir.c" - Services to find a specified directory entry     *
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

#if LFN_FLAG == 1
/**************************************************************************/
/* Searches an open directory for files matching the file specification and   */
/* the search flags.                                                          */
/* On success, returns 0 and fills the passed find data structure (optional). */
/* Returns a negative error code on failure.                                  */

/*
	history:

		lanzhu @ [1/20/2006]
		暂时关闭了文件名大小写的问题，还需要再考虑一下!!!!


		lanzhu @ [1/19/2006]
		原来的函数在进行字符串比较时，使用了utf8_stricmp时，将字符串转换为了大写字符!!
		
		改动一：直接使用 strcmp 函数
		改动二：只进行 长文件名的比较，不进行短文件名的比较

*/
INT16S fat_find(f_node_ptr fnp, CHAR *FileSpec, INT32S Flags, tFatFind *Ff)
{
	INT16S	Res;
	INT8S	AllowableAttr = (INT8S) Flags;
	INT8S	RequiredAttr  = (INT8S) (Flags >> 8);
	


#if WITHEXFAT == 1
	Ff->DIRAttr=Flags;
	memset(Ff->LongName,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));
	fd32_utf8to16((UTF8 *) FileSpec, (UTF16 *) Ff->LongName);
	Ff->NameHash= NameHash((UTF16 *) Ff->LongName);
	memset(Ff->LongName,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));
#endif
	
	while ((Res = lfn_readdir(fnp, Ff)) == 0) 
	{
		if (((AllowableAttr | Ff->SfnEntry.Attr) == AllowableAttr)
			&& ((RequiredAttr & Ff->SfnEntry.Attr) == RequiredAttr)||(ISEXFAT(fnp->f_dpb)))
		{
		#ifdef FATLFN
			if (utf8_stricmp((UTF8 *) Ff->LongName, (UTF8 *) FileSpec) == 0)
			{
				memcpy((void*)&fnp->f_dir, (void*)&Ff->SfnEntry, sizeof(tDirEntry)); 	 //m by zhangyan
				#if WITHEXFAT == 1
				fnp->ext_dir=(ExFATExtDIR *)&fnp->f_dir;
			    #endif
			    return 0;
			}
		#endif
			
			if (utf8_stricmp((UTF8 *) Ff->ShortName, (UTF8 *) FileSpec) == 0)					
			{					
				memcpy((void*)&fnp->f_dir, (void*)&Ff->SfnEntry, sizeof(tDirEntry)); 	 //m by zhangyan					
				#if WITHEXFAT == 1
				fnp->ext_dir=(ExFATExtDIR *)&fnp->f_dir;
			    #endif
				return 0;				
			}
		}
	}
	return Res = DE_PATHNOTFND;
}

/**************************************************************************/
#endif // LFN_FLAG

