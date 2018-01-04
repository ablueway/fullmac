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

#if LFN_FLAG == 1
/*changeLog:
 *	wanghuidi,2005.01.18, change the sequence of characters process to below
 *  first truname(add CDS to get pathmax)
 *  then ChartToUTF8
 *  then split_path_LFN
 *  then check_lfn first(to find invalid filename),
 */
INT32S fs_lfnopen(CHAR *path, INT16U flags, INT16U attrib)
{
	INT16S 			i;
	REG f_node_ptr 	fnp;
	
	INT8S *			path_max;	
#if MALLOC_USE == 1
	INT8S *			Dir;
	INT8S *			Name;
	tFatFind*		psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S			*Dir = gFS_path1;
	INT8S			*Name = gFS_name1;
	tFatFind		*psFind = &gFS_sFindData;
#else
	STATIC INT8S		Dir[FD32_LFNPMAX];
	STATIC INT8S		Name[FD32_LFNMAX];
	STATIC tFatFind	sFindData;
	tFatFind* 		psFind = &sFindData;
#endif
	INT16S			res; 
	INT16S			status = S_OPENED;

	/* First test the flags to see if the user has passed a valid   */
	/* file mode...                                                 */
	if ((flags & O_ACCMODE) > 2)
		return DE_INVLDACC;	  

	i = check_path_length((INT8S *) path, FD32_LFNPMAX);
	if (i<0)
		return i;	// 文件名超长，直接返回

#if MALLOC_USE == 1		//add by zhangyan
	Dir = (INT8S *)FS_MEM_ALLOC(2 * i + sizeof(tFatFind));	
	if (Dir ==NULL){
		return DE_NOMEM;
	}
	Name = (INT8S *)&Dir[i];
	psFind = (tFatFind*)&Dir[2 * i];
#endif

	path_max = psFind->LongName;
		//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8((INT8S *) path, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{
		if (DE_UNITABERR == ChartToUTF8((INT8S *) path, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 
		#endif
			return DE_UNITABERR;
		}
	}
	pre_truename(path_max);     //change '/' to'\'
	split_path_LFN((CHAR *) path_max, (CHAR *) Dir, (CHAR *) Name);

	if (check_lfn((CHAR *) Name) != 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return DE_INVLDPARM;
	}

	if ((fnp = descend_path((CHAR *) Dir, psFind)) == NULL)
	{ 	
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 		
	#endif
		return gFS_errno;
	}

	/* Check that we don't have a duplicate name, so if we  */
	/* find one, truncate it (O_CREAT).                     */
	if (fat_find(fnp, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) == 0)
	{
		fnp -> f_diroff -= 1;
		
		if (flags & O_TRUNC)
		{
		#ifndef READ_ONLY
			/* The only permissable attribute is archive,   */
			/* check for any other bit set. If it is, give  */
			/* an access error.                             */
			if ((fnp->f_dir.dir_attrib & (D_RDONLY | D_DIR | D_VOLID))
				|| (fnp->f_dir.dir_attrib & ~D_ARCHIVE & ~attrib))
			{
				dir_close(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)Dir); 
			#endif
				return DE_ACCESS;
			}

			/* Release the existing files FAT and set the   */
			/* length to zero, effectively truncating the   */
			/* file to zero.                                */
			status = S_REPLACED;
		#else
				return DE_ACCESS;
		#endif
		}
	    else if (flags & O_OPEN)
	    {
			/* force r/o open for FCB if the file is read-only */
			if ((flags & O_FCB) && (fnp->f_dir.dir_attrib & D_RDONLY))
				flags = (flags & ~3) | O_RDONLY;

			if (fnp->f_dir.dir_attrib & D_DIR && attrib != D_OPEN_DIR_FOR_STAT)//matao add
			{
				dir_close(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)Dir);
			#endif
				return DE_FILEEXISTS;
			}
		}
		else
		{
			dir_close(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 
		#endif
			return DE_FILEEXISTS;
		}
	}
	else if (flags & O_CREAT)
	{
	#ifndef READ_ONLY	
		res = fat_creat(fnp, Name, D_ARCHIVE);
		//  Name being unused  
		if (res) 	 
		{
			release_f_node(fnp); 
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 	
		#endif
			return res;
		}
		status = S_CREATED;
	#else	
		return DE_ACCESS;	
	#endif
	}
	else
	{
    	/* open: If we can't find the file, just return a not    */
    	/* found error.                                          */
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	#endif
		return DE_FILENOTFND;
	}

	/* Set the fnode to the desired mode                    */
	/* Updating the directory entry first.                  */
	fnp->f_mode = flags & O_ACCMODE;

	if (status != S_OPENED)
	{
	#ifndef READ_ONLY
		CLUSTER d_start;
		struct dpb* dpbp; 		
		
		dpbp = fnp->f_dpb;
	  #if WITHEXFAT == 1
		fnp->ext_dir=(ExFATExtDIR *)&fnp->f_dir;
	  #endif

		d_start = getdstart(dpbp, &fnp->f_dir);
		
	    if (d_start!=FREE)
		{
		  #if WITHEXFAT == 1
			if(ISEXFAT(dpbp))
				exfat_wipe_out_clusters(fnp);
			else	
		  #endif		
				wipe_out_clusters(dpbp, d_start);
		}	
		
	  #if WITHEXFAT == 1
		if(ISEXFAT(dpbp))
			init_exfatdir(fnp, fnp->f_dir.dir_attrib | attrib, FREE);
		else
	  #endif
		{
			init_direntry(&fnp->f_dir, fnp->f_dir.dir_attrib | attrib, FREE); 	
			fnp->f_flags = F_DMOD | F_DDIR;
		}
		if (!dir_write(fnp))
		{
			release_f_node(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Dir); 
		#endif
			return DE_ACCESS;
		}
		
	#else
		return DE_ACCESS;	
	#endif 
	}

#if _DIR_UPDATA == 1
	/* backup dir address */
	fnp->f_dir_address = fnp->f_offset;
	fnp->f_dir_cluster = fnp->f_cluster;
#endif

	/* Now change to file                                   */
	fnp->f_offset = 0;

	if (status != S_OPENED)
	{
	#ifndef READ_ONLY	
		fnp->f_cluster = FREE;
		setdstart(fnp->f_dpb, &fnp->f_dir, FREE);
		fnp->f_cluster_offset = 0;
	#else
		return DE_ACCESS;	
	#endif
	}

	fnp->f_flags = 0;
	
#ifndef READ_ONLY	
	if (status != S_OPENED)
		fnp->f_flags = F_DMOD;
#endif	

	merge_file_changes(fnp, status == S_OPENED); /* /// Added - Ron Cemer */
	/* /// Moved from above.  - Ron Cemer */
	fnp->f_cluster = getdstart(fnp->f_dpb, &fnp->f_dir);
	fnp->f_cluster_offset = 0;
	
	save_far_f_node(fnp);

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Dir); 		 //add by zhangyan
#endif

#if _SEEK_SPEED_UP == 1
	// 只读打开，并且 是 D_NORMAL
	if ( (O_RDONLY == (flags&O_ACCMODE)) && (attrib == D_NORMAL) && !(ISFAT12(fnp->f_dpb))||(ISEXFAT(fnp->f_dpb))) 
	{
		check_flink_sequence(fnp);	// just for file O_RDONLY		 	 
	}
#endif
	
	// reset file common_flag
	fnp->common_flag = 0x00;
	
	return xlt_fnp(fnp) | ((INT32S)status << 16);
}

#endif
