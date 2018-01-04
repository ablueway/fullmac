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
/*changeLog:
 *	wanghuidi,2005.01.17,  2005.01.20
 *  first truname(add CDS to get pathmax)
 *  then ChartToUTF8
 *  then split_path_LFN
 *  then check_lfn first(to find invalid filename),
 */
INT16S fs_lfnmkdir(CHAR *dirpath)
{
	INT16S				i;		
	REG f_node_ptr		fnp;
	INT8S *				path_max;
#if MALLOC_USE == 1
	INT8S *				Dir;
	INT8S *				Name;
	tFatFind*			psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S *				Dir = gFS_path1;
	INT8S *				Name = gFS_name1;
	tFatFind*			psFind = &gFS_sFindData;
#else
	STATIC INT8S			Dir[FD32_LFNPMAX];
	STATIC INT8S			Name[FD32_LFNMAX];
	STATIC tFatFind		sFindData;
	tFatFind* 			psFind = &sFindData;
#endif

	INT16S				res;
	REG INT16S			idx;
	struct buffer		*bp;
	struct dpb			*dpbp;
	struct dirent		DirEntBuffer;			/* Temporary directory entry            */
	CLUSTER				free_fat, parent;
	
	i = check_path_length((INT8S *) dirpath, FD32_LFNPMAX);
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
		if (DE_UNITABERR == UnicodeToUTF8((INT8S *) dirpath, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{
    if (DE_UNITABERR == ChartToUTF8((INT8S *) dirpath, (UTF8 *) path_max))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 	
	#endif	
		return DE_UNITABERR;
	}
	}
	
    pre_truename(path_max);                 //change '/' to'\',and change disk id toupper  
    pre_truename_dir((CHAR *) path_max);             //cut the \ at the end of path   
    split_path_LFN((CHAR *) path_max, (CHAR *) Dir, (CHAR *) Name);

	//2006/4/7 for dir name='.xxx' is error,but filename='.xxx' is ok
	if (check_lfn_dir((INT8U *) Name) != 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 	
	#endif
		return DE_INVLDPARM;
	}

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
        dir_close(fnp);
	#if MALLOC_USE == 1
	    FS_MEM_FREE((INT16U *)Dir);
	#endif
        return DE_FILEEXISTS; //needs more details
    }
  
    parent = fnp->f_dirstart;
    res = fat_creat(fnp, Name, D_DIR);
    if (res!=0) 	
    {
        release_f_node(fnp);
	#if MALLOC_USE == 1
	    FS_MEM_FREE((INT16U *)Dir);
	#endif
       return res;
    }

    /* get an empty cluster, so that we make it into a directory.          */
    /* TE this has to be done (and failed) BEFORE the dir entry is changed */
    free_fat = find_fat_free(fnp);

    /* No empty clusters, disk is FULL! Translate into a useful error message. */
    if (free_fat == LONG_LAST_CLUSTER)
    {
        remove_lfn_entries(fnp);
        *(fnp->f_dir.dir_name) = DELETED;
        fnp->f_flags |= F_DMOD;
        dir_close(fnp);
	#if MALLOC_USE == 1
	    FS_MEM_FREE((INT16U *)Dir);
	#endif
        return DE_HNDLDSKFULL;
    }

    /* Set the fnode to the desired mode                         */
    fnp->f_mode = WRONLY;                                        
    
  #if WITHEXFAT == 1
    if(ISEXFAT(fnp->f_dpb))
		init_exfatdir(fnp, D_DIR, free_fat);
	else
  #endif                                                               
  	{
  		init_direntry(&fnp->f_dir, D_DIR, free_fat);                                          
    	fnp->f_flags = F_DMOD | F_DDIR;                              
  	}                                                             
    fnp->f_offset = 0;                                          
                                                                 
    /* Mark the cluster in the FAT as used                       */
    dpbp = fnp->f_dpb;                                           
    link_fat(dpbp, free_fat, LONG_LAST_CLUSTER,NULL);               
                                                                 
    /* Craft the new directory. Note that if we're in a new      */
    /* directory just under the root, ".." pointer is 0.         */
    /* as we are overwriting it completely, don't read first     */
    bp = getblockOver(clus2phys(free_fat, dpbp), dpbp->dpb_unit);
    if (bp == NULL)
    {
        dir_close(fnp);
	#if MALLOC_USE == 1
	    FS_MEM_FREE((INT16U *)Dir);
	#endif
        return DE_BLKINVLD;
    }

	#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		fmemset(bp->b_buffer, 0, BUFFERSIZE);
	}
	else	
	#endif
	{
	    /* Create the "." entry                                 */
	    DirEntBuffer.dir_name[0] = '.';
	    memset(DirEntBuffer.dir_name + 1, ' ', FNAME_SIZE + FEXT_SIZE - 1);
	    init_direntry(&DirEntBuffer, D_DIR, free_fat);
	    
	    /* And put it out                                       */
	    putdirent(&DirEntBuffer, bp->b_buffer);
	    
	    /* create the ".." entry                                */
	    DirEntBuffer.dir_name[1] = '.';
#if WITHFAT32 == 1
		if (ISFAT32(dpbp) && parent == dpbp->dpb_xrootclst)
			parent = 0;
#endif
	    setdstart(dpbp, &DirEntBuffer, parent);

	    /* and put it out                                       */
	    putdirent(&DirEntBuffer, &bp->b_buffer[DIRENT_SIZE>>WORD_PACK_VALUE]);

	    /* fill the rest of the block with zeros                */
	    fmemset(&bp->b_buffer[DIRENT_SIZE * 2 >>WORD_PACK_VALUE], 0, BUFFERSIZE - DIRENT_SIZE * (2 >>WORD_PACK_VALUE));

	    /* Mark the block to be written out                     */
	    bp->b_flag |= BFR_DIRTY | BFR_VALID;
	}
    /* clear out the rest of the blocks in the cluster      */
    for (idx = 1; idx <= dpbp->dpb_clsmask; idx++)
    {
        /* as we are overwriting it completely, don't read first */
        bp = getblockOver(clus2phys(getdstart(dpbp, &fnp->f_dir), dpbp) + idx,
                          dpbp->dpb_unit);
        if (bp == NULL)
        {
            dir_close(fnp);
		#if MALLOC_USE == 1
	        FS_MEM_FREE((INT16U *)Dir);
		#endif
	        return DE_BLKINVLD;
        }
        fmemset(bp->b_buffer, 0, BUFFERSIZE);
        bp->b_flag |= BFR_DIRTY | BFR_VALID | BFR_UNCACHE; /* need not be cached */
    }

    /* flush the drive buffers so that all info is written  */
    /* hazard: no error checking!                           */
    flush_buffers(dpbp->dpb_unit);
    
    /* Close the directory so that the entry is updated     */
    fnp->f_flags |= F_DMOD;
    dir_close(fnp);

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Dir);
#endif
    return SUCCESS;
}

#endif
#endif //#ifndef READ_ONLY
