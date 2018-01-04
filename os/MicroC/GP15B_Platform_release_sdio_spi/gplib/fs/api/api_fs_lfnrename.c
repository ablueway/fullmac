/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "open.c" - Open, create and close a file (or even a directory  *
 *                    as a file) in any directory, allocating and freeing *
 *                    handles                                             *
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
INT16S fs_lfnrename(INT8S * path1, INT8S * path2)
{
	REG f_node_ptr		fnp1;
	REG f_node_ptr		fnp2;
	INT16S				ret;
	INT16S				res;

	INT8S *				path_max;
#if MALLOC_USE == 1
	INT8S *				Name;
	INT8S *				Path;
	tFatFind*			psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S *				Name = gFS_name1;
	INT8S *				Path = gFS_path1;
	tFatFind*			psFind = &gFS_sFindData;
#else
	STATIC INT8S			Name[FD32_LFNMAX];
	STATIC INT8S			Path[FD32_LFNPMAX];
	STATIC tFatFind		sFindData;
	tFatFind*			psFind = &sFindData;
#endif

	INT16S				old_len, new_len;

	old_len = check_path_length(path1,FD32_LFNPMAX);
	if (old_len <0) 
		return old_len;				

	new_len = check_path_length(path2,FD32_LFNPMAX);
	if (new_len <0) 
		return new_len;		

#if MALLOC_USE == 1
	Path = (INT8S *)FS_MEM_ALLOC(max(old_len, new_len));
	if (Path == NULL)
		return DE_NOMEM;

	Name = (INT8S *)FS_MEM_ALLOC(max(old_len, new_len) + sizeof(tFatFind));
	if (Name == NULL)
	{
		FS_MEM_FREE((INT16U *)Path); 
		return DE_NOMEM;
	}
	psFind = (tFatFind*)&Name[max(old_len, new_len)];
#endif

	path_max = psFind->LongName;
	//old file name
		//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(path1, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{
	if (DE_UNITABERR == ChartToUTF8(path1, (UTF8 *) path_max))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return DE_UNITABERR;
	}
	}
	pre_truename(path_max);     //change '/' to'\'
	split_path_LFN((CHAR *) path_max, (CHAR *) Path, (CHAR *) Name);
	
	if ((fnp1 = descend_path((CHAR *) Path, psFind)) == NULL)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return gFS_errno; 
	}
	
	if (fat_find(fnp1, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) != 0)
	{
		dir_close(fnp1);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return DE_FILENOTFND;
	}

	//new file name
		//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(path2, (UTF8 *) path_max) )
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{
	if (DE_UNITABERR == ChartToUTF8(path2, (UTF8 *) path_max) )
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif	
		return DE_UNITABERR;
	}
	}
	pre_truename(path_max);     //change '/' to'\'
	split_path_LFN((CHAR *) path_max, (CHAR *) Path, (CHAR *) Name);

	if (check_lfn((CHAR *) Name) != 0) 
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return DE_INVLDPARM;
	}

	if ((fnp2 = descend_path((CHAR *) Path, psFind)) == NULL)
	{
		dir_close(fnp1);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return gFS_errno; 
	}

	if (fat_find(fnp2, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) == 0)
	{
		dir_close(fnp1);
		dir_close(fnp2);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return DE_FILEEXISTS;
	}
	
	//creat new name file
	res = fat_creat(fnp2, Name, O_RDWR);
	if (res) 	 
	{
		dir_close(fnp1); 
		dir_close(fnp2); 
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return res;
	}

	fnp1->f_diroff--;
	if ((ret = remove_lfn_entries(fnp1)) < 0)
	{
		dir_close(fnp2); 
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Path); 
		FS_MEM_FREE((INT16U *)Name); 
	#endif
		return ret;
	}

#if WITHEXFAT == 1
	if(ISEXFAT(fnp2->f_dpb))
	{
		fnp2->extmain_dir.CreatemsIncrement=fnp1->extmain_dir.CreatemsIncrement;
		fnp2->extmain_dir.CreateTimestamp=fnp1->extmain_dir.CreateTimestamp;
		fnp2->extmain_dir.FileAttributes=fnp1->extmain_dir.FileAttributes;
		
		fnp2->ext_dir->ValidDataLength=fnp1->ext_dir->ValidDataLength;
		fnp2->ext_dir->SecondaryFlags=fnp1->ext_dir->SecondaryFlags;
		fnp2->ext_dir->DataLength=fnp1->ext_dir->DataLength;
		fnp2->ext_dir->FirstCluster=fnp1->ext_dir->FirstCluster;
		
		fnp2->f_flags  = F_DMOD | F_DDIR|F_ExMDir;
	}
	else
#endif
	{
		/* Set the fnode to the desired mode                            */
		fnp2->f_dir.dir_size = fnp1->f_dir.dir_size;
		fnp2->f_dir.dir_start = fnp1->f_dir.dir_start;
#if WITHFAT32 == 1
		fnp2->f_dir.dir_start_high = fnp1->f_dir.dir_start_high;
#endif
		fnp2->f_dir.dir_attrib = fnp1->f_dir.dir_attrib;
		fnp2->f_dir.dir_time = fnp1->f_dir.dir_time;
		fnp2->f_dir.dir_date = fnp1->f_dir.dir_date;
		// 2005-2-1 Yongliang add begin
		fnp2->f_dir.dir_crtimems = fnp1->f_dir.dir_crtimems;
		fnp2->f_dir.dir_crtime = fnp1->f_dir.dir_crtime;
		fnp2->f_dir.dir_crdate = fnp1->f_dir.dir_crdate;
		// 2005-2-1 Yongliang add end
		/* The directory has been modified, so set the bit before       */
		/* closing it, allowing it to be updated.                       */
		fnp1->f_flags = fnp2->f_flags = F_DMOD | F_DDIR;

		/* Ok, so we can delete this one. Save the file info.           */
		*(fnp1->f_dir.dir_name) = DELETED;
	}

	dir_close(fnp1);
	dir_close(fnp2);
	
	/* SUCCESSful completion, return it                             */
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Path); 
	FS_MEM_FREE((INT16U *)Name); 
#endif
	return SUCCESS;
}

#endif
#endif	//#ifndef READ_ONLY
