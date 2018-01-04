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
INT16S fs_lfndelete(INT8S * dirpath)
{
	REG f_node_ptr fnp;
	INT8U i;
	
	INT8S * path_max;
#if MALLOC_USE == 1
	INT8S * Dir;
	INT8S * Name;
	tFatFind* psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S * Dir = gFS_path1;
	INT8S * Name = gFS_name1;
	tFatFind* psFind = &gFS_sFindData;
#else
	STATIC INT8S		Dir[FD32_LFNPMAX];
	STATIC INT8S		Name[FD32_LFNMAX];
	STATIC tFatFind	sFindData;
	tFatFind* psFind = &sFindData;
#endif
	INT16S j;
	
	j = check_path_length(dirpath,FD32_LFNPMAX);
	if (j < 0) 
		return j;

#if MALLOC_USE == 1
	Dir = (INT8S *)FS_MEM_ALLOC(2 * j + sizeof(tFatFind));
	if (Dir == NULL)
		return DE_NOMEM;
	
	Name = (INT8S *)&Dir[j];
	psFind = (tFatFind*)&Dir[2 * j];
#endif

	path_max = psFind->LongName;
	
	
		//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(dirpath, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{	
	if (DE_UNITABERR == ChartToUTF8(dirpath, (UTF8 *) path_max))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return DE_UNITABERR;
	}
	}

  	pre_truename(path_max);     //change '/' to'\'
    split_path_LFN((CHAR *) path_max, (CHAR *) Dir, (CHAR *) Name);
 
	if ((fnp = descend_path((CHAR *) Dir, psFind)) == NULL)
	{	
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return gFS_errno; 	 // 2005-2-4 Yongliang change
	}

	if (fat_find(fnp, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) == 0)
	{
		/* Do not delete directories or r/o files       */
		/* lfn entries and volume labels are only found */
		/* by find_fname() if attrib is set to a        */
		/* special value                                */	  
		// 2004-11-19 Yongliang add begin
		if (fnp->f_dir.dir_attrib & D_DIR)
		{
			dir_close(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 
		#endif
			return DE_ISDIR;
		}
		// 2004-11-18 Yongliang suggests reporting two kinds of errors here:
		// 	 It is a directory
		// 	 It is read-only
		fnp->f_diroff--;
		// 2005-2-2 Yongliang add begin: Avoid deleting using file
		for (i = 0; i < MAX_FILE_NUM; i ++) 	 {
			if (f_nodes_array[i].f_count != 0 && fnp != &(f_nodes_array[i]))
			{
				if (f_nodes_array[i].f_dirstart == fnp ->f_dirstart
					&& f_nodes_array[i].f_diroff == fnp ->f_diroff
					&& f_nodes_array[i].f_dpb == fnp->f_dpb )
				{
					dir_close (fnp);				
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)Dir); 
				#endif
					return DE_ACCESS;
				}
			}
		}
		// 2005-2-2 Yongliang add end
		#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
		#endif
		return delete_dir_entry(fnp);
	}
	else
	{
		/* No such file, return the error               */
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return DE_FILENOTFND;
	}
}

#endif

INT32S fs_lfndel_step(void)
{
    return unlink_step_run();
}

void fs_lfndel_step_start(void)
{
    unlink_step_reg();
}

void fs_lfndel_step_end(void)
{
    unlink_step_end();
}

#endif	//#ifndef READ_ONLY
