#include "fsystem.h"
#include "globals.h"


#ifndef READ_ONLY
#if LFN_FLAG == 1
/*
	 2004-11-26 Yongliang add this file
	 fs_deleteall (INT8S * filename): empty the specified folder
	 return value:
		 0 					 success
		 DE_PATHNOTFND 		 path not found
		 DE_ACCESS 			 access denided or meets a invalid entry
		 DE_BLKINVLD 			 low level i/o error
		
*/

INT16S fs_lfndeleteall_slow (f_node_ptr);
#if WITHEXFAT == 1
	#define FAST_DELETE_CACHE_LEVEL 50
#else
	#define FAST_DELETE_CACHE_LEVEL 16
#endif
#define Push_cache()	cluster_cache[idx] = fnp ->f_dirstart;\
						 offset_cache[idx] = fnp ->f_diroff;\
						 idx ++;
#define Pop_cache()		idx --;\
						 fnp ->f_dirstart = cluster_cache[idx];\
						 fnp ->f_diroff = offset_cache[idx];
#define Is_cache_full()		(idx == FAST_DELETE_CACHE_LEVEL)
#define Is_cache_empty()	(!idx)



//For Deleteall  used



void dir_close_Ex(f_node_ptr fnp)
{
	if (!fnp || !(fnp->f_flags&F_DDIR) || !(fnp->f_dpb->dpb_mounted)) {
		return;
	}

  #if WITHFAT32 == 1
	if ((fnp->f_dpb->dpb_fsinfoflag & FSINFODIRTYFLAG)&&(!ISEXFAT(fnp->f_dpb))) {
		write_fsinfo(fnp->f_dpb);
	}
  #endif

#ifndef READ_ONLY
	dir_write(fnp);							// Write out the entry

#endif
	//flush_buffers(fnp->f_dpb->dpb_unit);	// Clear buffers after release
	
	release_f_node(fnp);					// And release this instance of the fnode

}


#ifndef READ_ONLY
INT16S remove_lfn_entries_Ex(f_node_ptr fnp)
{
	INT16U original_diroff = fnp->f_diroff;
#if WITHEXFAT == 1
	INT8U BlackEntries[32],DirNum=0,DirCount=0;
	if(ISEXFAT(fnp->f_dpb))
	{	
		memcpy((void *)BlackEntries,(void *)fnp->ext_dir,32);
		fnp->f_diroff-=2;
	}
#endif

	while (1) {
  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			if(DirCount>DirNum)
			{
				break;
			}
			fnp->f_diroff++;	
			if (dir_read(fnp) <= 0) {		
				dir_close_Ex(fnp);
				return DE_BLKINVLD;
			}
			if(DirCount==0)
			{
				if(fnp->f_dir.dir_name[0]==0x85)
				{
					DirNum=fnp->f_dir.dir_name[1];
				}
				else
				{		
					dir_close_Ex(fnp);
					return DE_BLKINVLD;
				}
			}
			else if(DirCount==1)
			{
				if(fnp->f_dir.dir_name[0]!=0xC0)
				{					
					dir_close_Ex(fnp);
					return DE_BLKINVLD;
				}
			}
			else
			{
				if(fnp->f_dir.dir_name[0]!=0xC1)
				{					
					dir_close_Ex(fnp);
					return DE_BLKINVLD;
				}
			}			
			DirCount++;
			fnp->f_dir.dir_name[0] &=0x7F;				
			
		}
		else
  #endif
		{
			if (fnp->f_diroff == 0) {
				break;
			}
			fnp->f_diroff--;
			if (dir_read(fnp) <= 0) {					
			fnp->f_diroff = original_diroff;		
				dir_close_Ex(fnp);
				return DE_BLKINVLD;
			}
			if (fnp->f_dir.dir_attrib != D_LFN) {
				break;
			}
			fnp->f_dir.dir_name[0] = DELETED;
		}
		
		fnp->f_flags |= F_DMOD;
	
		if (!dir_write(fnp)) {						
			return DE_BLKINVLD; 			
		}
	}

	
#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		memcpy((void *)fnp->ext_dir,(void *)BlackEntries,32);
		fnp->ext_dir->EntryType&=0x7F;
	}
	else
#endif
	{
		fnp->f_diroff = original_diroff;
		if (dir_read(fnp) <= 0) {
			dir_close_Ex(fnp);
			return DE_BLKINVLD;
		}
	}	
	return SUCCESS;
}
#endif

#ifndef READ_ONLY
INT16S delete_dir_entry_Ex(f_node_ptr fnp)
{
	INT16S rc;
	CLUSTER d_start;
	struct dpb *dpbp;
	INT16S dsk;
	
	// Ok, so we can delete. Start out by clobbering all FAT entries for this file (or, in English, truncate the FAT).
	if ((rc=remove_lfn_entries_Ex(fnp)) < 0) {
		return rc;
	}

	#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		fnp->f_dir.dir_name[0] = fnp->f_dir.dir_name[0]&0x7F;
	}
	else
	#endif
	{
		fnp->f_dir.dir_name[0] = DELETED;
	}
	fnp->f_flags |= F_DMOD; 				// Set the bit before closing it, allowing it to be updated
	dpbp = fnp->f_dpb;
	dsk = fnp->f_dpb->dpb_unit;
	d_start = getdstart(dpbp, &fnp->f_dir);

	dir_close_Ex(fnp);
	if (d_start != FREE) {
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			exfat_wipe_out_clusters(fnp);
		}
		else
	  #endif		
		{
			wipe_out_clusters(dpbp, d_start);
		}
	}
	//flush_buffers(dsk);

	return SUCCESS;
}
#endif






/************************************************************************/
/*	fs_lfndeleteall

Main Method: Deleting the left most filesystem tree node iterative.
	SUPPOSE: A node in the filesystem is eigher a file or a directory.
	If any file or directory in the directory is read only, then a error will be throwed.
	
input:	PathName 路径名字符串
output: 

history:	
		[1/22/2006]
			lanzhu 发现进行全部删除时，在文件较多时不能完全删除成功

		[1/17/2006]
			lanzhu 去掉对只读属性的判断	

                                                                      */
/************************************************************************/
INT16S fs_lfndeleteall (INT8S * PathName) 	 
{
	f_node_ptr fnp;
	CLUSTER cluster_cache[FAST_DELETE_CACHE_LEVEL];
	INT16U offset_cache[FAST_DELETE_CACHE_LEVEL];
	INT16S idx = 0, res;

	//INT16U k, sector_size, shft_cnt, rel_cluster;
	INT16U sector_size,shft_cnt;
	//INT32U j;
#if MALLOC_USE == 1
	INT8S * path_max;
	tFatFind* psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S		*path_max = gFS_path1;
	tFatFind	*psFind = &gFS_sFindData;
#else
	STATIC INT8S path_max[MAX_CDSPATH];
	STATIC tFatFind sFindData;
	tFatFind* psFind = &sFindData;
#endif

	INT16S i;
	i = check_path_length(PathName,FD32_LFNPMAX);
	if (i<0) 
		return i; 

#if MALLOC_USE == 1
	path_max = (INT8S *)FS_MEM_ALLOC(i+sizeof(tFatFind));
	if (path_max == NULL)
		return DE_NOMEM;
	
	psFind = (tFatFind*)&path_max[i];	 
#endif
	//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(PathName, (UTF8 *) path_max))
		{
		  #if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		  #endif
			return DE_UNITABERR;
		}
	}
	else
	{	
		if (DE_UNITABERR == ChartToUTF8(PathName, (UTF8 *) path_max))
		{
		  #if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 
		  #endif	
			return DE_UNITABERR;
		}
	}

	pre_truename(path_max);     //change '/' to'\'
	pre_truename_dir((CHAR *) path_max); //cut the \ in the end of path  
	
	// now test for its existance. If it doesn't, return an error.
	fnp = descend_path((CHAR *) path_max, psFind);
	if (fnp == NULL)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 
	#endif
		return gFS_errno; 	 // 2005-2-4 Yongliang change
	}

	sector_size = fnp->f_dpb->dpb_secsize;
	shft_cnt = fnp->f_dpb->dpb_shftcnt;

	// set the specified directory as the bottom of the stack
	Push_cache ();

	// ok. now try to empty the folder
	while (!Is_cache_empty ()) 	 
	{
		res = dir_read (fnp);
		switch (res)
		{
		case DE_BLKINVLD: 	 // low level i/o error
			dir_close (fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 
		#endif
			return res;
		case DE_SEEK:
		case 0: 	 // meets the end of current directory
			Pop_cache ();
			if (idx) 	 
			{
				fnp ->f_cluster = fnp ->f_dirstart;
				dir_read (fnp);
				delete_dir_entry (fnp);				
			}
			break;
		case 1: 	 // meets a file or a directory

		  #if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb))
			{
				if(fnp ->f_dir.dir_name[0] == 0x85)
				{
					memcpy((void *)&fnp->extmain_dir,(void *)&fnp->f_dir,32);
				}
				
				if (fnp ->f_dir.dir_name[0] == '\0' || (fnp ->f_dir.dir_name[0]&0x80) == 0x00)	 {
						break;	 // ignore deleted entries
				}
				if (fnp ->f_dir.dir_name[0] == 0xC1)
				{
					break;	 // ignore lfn entry
				}
			}
			else
		  #endif	
			{
				if (fnp ->f_dir.dir_name[0] == '\0' || fnp ->f_dir.dir_name[0] == DELETED) 	 {
					break; 	 // ignore deleted entries
				}
				if (fnp ->f_dir.dir_attrib == D_LFN)
				{
					break; 	 // ignore lfn entry
				}
				if (fnp->f_dir.dir_attrib & ~D_ALL) 	 
				{
					dir_close (fnp);
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)path_max); 
				#endif
					return DE_ACCESS; 	 // meets a invalid entry
				}
			}

		#if WITHEXFAT == 1
			if (((fnp ->f_dir.dir_attrib & D_DIR)&&(!ISEXFAT(fnp->f_dpb)))||((fnp ->extmain_dir.FileAttributes & D_DIR)&&(ISEXFAT(fnp->f_dpb)))) 	 
		#else
			if (fnp ->f_dir.dir_attrib & D_DIR)
		#endif
			{
				if(!ISEXFAT(fnp->f_dpb))
				{
					if (!memcmp (fnp ->f_dir.dir_name, ".          ", 11)
						|| !memcmp (fnp ->f_dir.dir_name, "..         ", 11))
					{
						break; 	 // ignore '.' and '..' directory
					}
				}
				
			  #if WITHEXFAT == 1
				if(ISEXFAT(fnp->f_dpb))
				{
					fnp ->f_diroff ++;
					res = dir_read (fnp);
					if(res == DE_BLKINVLD)
					{
						dir_close (fnp);
						return res;
					}
					else if(fnp->f_dir.dir_name[0] !=0xC0)
					{
						break;
					}
					fnp->ext_dir = (ExFATExtDIR *)&fnp->f_dir;
				}
			  #endif
			  
				if (Is_cache_full ()) 	 
				{ 	 
				  #if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb))
					{
						dir_close (fnp);
					  #if MALLOC_USE == 1
						FS_MEM_FREE((INT16U *)path_max); 
					  #endif
						return res;
					}
				  #endif
					
					// to much levels of directories call slow version
					res = fs_lfndeleteall_slow (fnp);

					if (res) 	 
					{
						dir_close (fnp);
					#if MALLOC_USE == 1
						FS_MEM_FREE((INT16U *)path_max); 
					#endif
						return res;
					}
					fnp ->f_cluster = fnp ->f_dirstart;
					dir_read (fnp);
					delete_dir_entry (fnp); 	 // delete the directory it self
				}
				else 	 
				{
					Push_cache (); 	 // set the sub directory as current directory
				  #if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb))
					{
						fnp ->f_dirstart = fnp->ext_dir->FirstCluster;
					}
					else
				  #endif
				  	{
						fnp ->f_dirstart = ((CLUSTER)fnp ->f_dir.dir_start_high) << 16;
						fnp ->f_dirstart += fnp ->f_dir.dir_start;
					}
					dir_init_fnode(fnp, getdstart (fnp ->f_dpb, &fnp ->f_dir));
					if(!ISEXFAT(fnp->f_dpb))
					{
						fnp ->f_diroff = 2; 	 // ignore '.' and '..' directory
					}
					continue;
				}
			}
			else
			{
				
  			  #if WITHEXFAT == 1
				if(ISEXFAT(fnp->f_dpb))
				{
					fnp ->f_diroff ++;
					res = dir_read (fnp);
					if(res == DE_BLKINVLD)
					{
						dir_close (fnp);
						return res;
					}
					else if(fnp->f_dir.dir_name[0] !=0xC0)
					{
						break;
					}
					fnp->ext_dir = (ExFATExtDIR *)&fnp->f_dir;

					//fnp ->f_diroff--;
				}
			  #endif				
				
				// delete the current file
				//fnp ->f_cluster = fnp ->f_dirstart;
			  	fnp->f_offset = fnp->f_diroff *(INT32U)DIRENT_SIZE;
				if (map_cluster(fnp, XFR_READ) != SUCCESS)
					return DE_HNDLDSKFULL;	
			  /*
				{
					rel_cluster = (INT16U)((fnp->f_offset/sector_size) >> shft_cnt);
					j = fnp->f_dirstart;

					for (k=0x00; k<rel_cluster; k++)
					{
						j = next_cluster(fnp->f_dpb, (INT32U)j);
					}
					fnp->f_cluster = j;
				}
			  */
			  
				delete_dir_entry_Ex (fnp);		
			  	
			}
			break;
		}
		fnp ->f_diroff ++;
	}

	//dir_close(fnp);
	release_f_node(fnp);					// And release this instance of the fnode
	
	flush_buffers(fnp->f_dpb->dpb_unit);

	#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)path_max); 
	#endif
	return 0;
}


/*
	 Slow version of f_deleteall (), when CLUSTER cache array is full, then
	 this function will be called
*/
#define Entry_sub_directory()	\
					 level ++;\
					 fnp ->f_dirstart = (INT32U)fnp ->f_dir.dir_start_high << 16;\
					 fnp ->f_dirstart += fnp ->f_dir.dir_start;\
					 fnp ->f_diroff = 0;\
					 dir_init_fnode(fnp, getdstart (fnp ->f_dpb, &fnp ->f_dir));

#define Entry_parent_directory()	\
					 level --;\
					 fnp ->f_dirstart = parent_cluster;\
					 fnp ->f_diroff = 0;\
					 fnp ->f_cluster = fnp ->f_dirstart;

#define Store_parent_directory()	\
					 parent_cluster = (INT32U)fnp ->f_dir.dir_start_high << 16;\
					 parent_cluster += fnp ->f_dir.dir_start; 
				  	


INT16S fs_lfndeleteall_slow (f_node_ptr fn_ptr) 	 {
	 struct f_node fn, * fnp = &fn;
	 INT16S level = 0, res;
	 CLUSTER parent_cluster = 0;

	 memcpy (fnp, fn_ptr, sizeof (struct f_node)); // duplicate the f_node
	 // set the sub directory as current directory
	 Entry_sub_directory ();
	 while (level) 	 {
		 res = dir_read (fnp);
		 switch (res) 	 {
		 case DE_BLKINVLD: 	 // low level i/o error
			 dir_close (fnp);
			 return res;
		 case DE_SEEK:
		 case 0: 	 // meets the end of current directory
			 Entry_parent_directory ();
			 if (level) 	 {
				 do 	 {
					 fnp ->f_diroff ++;
					 dir_read (fnp);
				 }
				 while (fnp ->f_dir.dir_name[0] == '\0'
					 || fnp ->f_dir.dir_name[0] == DELETED
					 || !memcmp (fnp ->f_dir.dir_name, ".          ", 11)
					 || !memcmp (fnp ->f_dir.dir_name, "..         ", 11));
				 delete_dir_entry (fnp);
				 fnp ->f_diroff = 0; 	 // let the main loop to find the '..' directory
				 continue;
			 }
			 break;
		 case 1: 	 // meets a file or a directory
			 if (fnp ->f_dir.dir_name[0] == '\0' || fnp ->f_dir.dir_name[0] == DELETED) 	 {
				 break; 	 // ignore deleted entries
			 }
			 if (fnp ->f_dir.dir_attrib == D_LFN) 	 {
				 break; 	 // ignore lfn entry
			 }
			 if (fnp ->f_dir.dir_attrib & ~D_ALL) 	 {
				 dir_close (fnp);
				 return DE_ACCESS; 	 // meets a invalid entry
			 }
			 if (fnp ->f_dir.dir_attrib & D_RDONLY) 	 {
				 dir_close (fnp);
				 return DE_ACCESS; 	 // the sub directory or file is read only
			 }
			 else if (fnp ->f_dir.dir_attrib & D_DIR) 	 {
				 if (!memcmp (fnp ->f_dir.dir_name, ".          ", 11)) 	 {
					 break; 	 // ignore '.' directory
				 }
				 if (!memcmp (fnp ->f_dir.dir_name, "..         ", 11)) 	 {
					 Store_parent_directory (); 	 // store upper level directory info
					 break;
				 }
				 else 	 {
					 Entry_sub_directory (); 	 // set the sub directory as current directory
					 continue;
				 }
			 }
			 else 	 {
				 // delete the current file
				 fnp ->f_cluster = fnp ->f_dirstart;
				 delete_dir_entry (fnp);
			 }
			 break;
		 }
		 fnp ->f_diroff ++;
	 }
	 return 0;
}

#endif
#endif //#ifndef READ_ONLY
