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
INT16S fs_lfnrmdir(INT8S * dirpath)
{
	f_node_ptr			fnp;
	struct f_node		tempfnp;
	f_node_ptr			fnp1 = &tempfnp;
	
	BOOLEAN				found;
	INT8S *				path_max;
#if MALLOC_USE == 1
	INT8S *				Dir;
	INT8S *				Name;
	tFatFind*			psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S				*Dir = gFS_path1;
	INT8S				*Name = gFS_name1;
	tFatFind			*psFind = &gFS_sFindData;
#else
	STATIC INT8S 	 	Dir[FD32_LFNPMAX];
	STATIC INT8S 	 	Name[FD32_LFNMAX];
	STATIC tFatFind 	sFindData;
	tFatFind* 			psFind = &sFindData;
#endif

	INT16S i;

	i = check_path_length(dirpath,FD32_LFNPMAX);
	if (i<0) 
		return i;		

#if MALLOC_USE == 1
	Dir = (INT8S *)FS_MEM_ALLOC(2 * i + sizeof(tFatFind));
	if (Dir == NULL)
		return DE_NOMEM;
	
	Name = (INT8S *)&Dir[i];
	psFind = (tFatFind*)&Dir[2 * i];
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
	pre_truename_dir((CHAR *) path_max); //cut the \ in the end of path   
	
    split_path_LFN((CHAR *) path_max, (CHAR *) Dir, (CHAR *) Name);

	if ((fnp = descend_path((CHAR *) Dir, psFind)) == NULL)
	{	
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return gFS_errno; 	 // 2005-2-4 Yongliang change
	}

	/* Check that we're not trying to remove the root!      */
	//if ((path[0] == '\\') && (path[1] == NULL))
	//{
	//  dir_close(fnp);
	//  return DE_ACCESS;
	//}

	/* Check that we don't have a duplicate name, so if we  */
	/* find one, it's an error.                             */
	if (fat_find(fnp, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) == 0)
	{
		*fnp1 = *fnp;

		dir_init_fnode(fnp1, getdstart(fnp1->f_dpb, &fnp1->f_dir));
	
		/* Check that the directory is empty. Only the  */
		/* "." and ".." are permissable.                */
		fnp->f_flags &= ~F_DMOD;

	  #if WITHEXFAT == 1
		if(!ISEXFAT(fnp1->f_dpb))
	  #endif
	  	{
			dir_read(fnp1);
			if (fnp1->f_dir.dir_name[0] != '.')
			{
				dir_close(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)Dir); 
			#endif
				return DE_NOTEMPTY; // 2004-11-19 Yongliang change error code from DE_ACCESS
			}
		
			fnp1->f_diroff++;
			dir_read(fnp1);
			if (fnp1->f_dir.dir_name[0] != '.')
			{
				dir_close(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)Dir); 
			#endif
				return DE_NOTEMPTY; // 2004-11-19 Yongliang change error code from DE_ACCESS
		    }
		
			/* Now search through the directory and make certain    */
			/* that there are no entries.                           */				
			fnp1->f_diroff++;
	  	}
	  
		found = FALSE;
		//Exfat 判断文件存在条件比较苛刻.<^-^>	

	 #if WITHEXFAT == 1
		if(ISEXFAT(fnp1->f_dpb))
		{
			
			psFind->NameHash=0xffff;
			psFind->DIRAttr=FD32_FRNONE | FD32_FAALL;
			if (lfn_readdir(fnp1, psFind) == 0)
			{
				found = TRUE;
			}
		}
		else
	  #endif
		while (dir_read(fnp1) == 1)
		{
			if (fnp1->f_dir.dir_name[0] == '\0')
				break;
			if (fnp1->f_dir.dir_name[0] != DELETED
				&& fnp1->f_dir.dir_attrib != D_LFN)
			{
				found = TRUE;
				break;
			}
			fnp1->f_diroff++;
		}
	
		/* If anything was found, exit with an error.   */
		if (found)
		{
			dir_close(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 
		#endif
			return DE_NOTEMPTY; // 2004-11-19 Yongliang change error code from DE_ACCESS
		}
		fnp->f_diroff--;
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
#endif  //#ifndef READ_ONLY
