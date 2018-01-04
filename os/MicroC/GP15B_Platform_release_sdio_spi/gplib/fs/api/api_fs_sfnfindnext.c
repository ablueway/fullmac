/************************************************************************/
/* 
	change to father dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
INT16S CmpFileName(CHAR *src, CHAR *tag)
{
	INT16U i = 0;
	
	for (i = 0; i < 3; i++)
	{
		if(tag[i] == '?')
			continue;
			
		if (fs_toupper(src[i]) != fs_toupper(tag[i]))
			return -1;
	}
	return 0;
}

INT16S CheckExtName(CHAR *src, CHAR *tag)
{
	INT16U i;
	CHAR tmp[3];
	INT16S ret = -1;
	
	if(tag[0] == '*')
	{
		return 0;
	}
	
	for(i = 0; i < 3; i++)
	{
		if(src[i] == 0)
		{
			src[i] = ' ';
		}
	}
	
	i = 0;
	memset(tmp, ' ', 3);
	while(1)
	{
		if(*tag == 0)
		{			
			ret = CmpFileName(src, tmp);
			break;
		}
		if(*tag == '|')
		{
			ret = CmpFileName(src, tmp);
			if(ret == 0)
			{
				break;	
			}
			i = 0;
			tag++;
			memset(tmp, ' ', 3);		
		}
		tmp[i] = *tag;
		tag++;
		if(i < 2)
		{
			i++;
		}
	}
	return ret;
}

f_ppos sfn_findnext(STDiskFindControl *pstDiskFindControl, INT8S dsk, CHAR *extname, struct sfn_info *sfn_info, INT16S attr,INT32S (*filter)(INT8S *str))
{
	REG f_node_ptr	fnp;
	INT16S	i;
	CHAR ExtName[4];
#if MALLOC_USE == 1
	tFatFind*	psFind;
#elif USE_GLOBAL_VAR == 1
	tFatFind	*psFind = &gFS_sFindData;
#else
	tFatFind	sFindData;
	tFatFind*	psFind = &sFindData;
#endif	
	INT8S		Dir[256];
	if (dsk != pstDiskFindControl->cur_pos.f_dsk)
	{
		gFS_errno = DE_DEVICE; 
		return NULL;
	}
	
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 
		return NULL;
	}
#if MALLOC_USE == 1
	psFind = (tFatFind*)FS_MEM_ALLOC(sizeof(tFatFind));
	if (NULL == psFind) 
	{
		release_f_node(fnp);
		return DE_NOMEM;
	}
#endif		
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;
	
	fnp->f_dpb = get_dpb(pstDiskFindControl->cur_pos.f_dsk);
	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV; 
		release_f_node(fnp);
		return NULL;
	}
	
	dir_init_fnode(fnp, pstDiskFindControl->cur_pos.f_entry);
	/* Search through the directory to find the entry, but do a     */
	/* seek first.                                                  */
  	fnp->f_diroff = pstDiskFindControl->cur_pos.f_offset;
  	
  	// 080321 add for disk find speedup
  	if(pstDiskFindControl->sfn_cluster) 
  	{
  		fnp->f_cluster = pstDiskFindControl->sfn_cluster;
  		fnp->f_cluster_offset = pstDiskFindControl->sfn_cluster_offset;
  	}
  	// 080321 add end
	ExtName[3] = 0;
	if(filter==NULL)
	{
		while (0 == sfn_readdir(fnp))
		{
		#if 0
			if( (fnp->f_dir.dir_name[0] == 'B') && 
				(fnp->f_dir.dir_name[1] == 'I') && 
				(fnp->f_dir.dir_name[2] == 'T') && 
				(fnp->f_dir.dir_name[3] == 'D') && 
				(fnp->f_dir.dir_name[4] == 'C') && 
				(fnp->f_dir.dir_name[5] == 'O') && 
				(fnp->f_dir.dir_name[6] == 'D') )
			{
				fnp->f_dir.dir_name[0] = 'A';
			}
		#endif

		#if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb))
			{
				memcpy((void *)ExtName,(void *)&fnp->ShortName[8],3);	
			}	
			else	
		#endif
			{
				memcpy((void *)ExtName,(void *)&fnp->f_dir.dir_name[8],3);
			}
			//判断文件名是否符合
			if ( !CheckExtName((CHAR *) ExtName, extname) )
			{
				//判断属性是否符合
				if ( ( fnp->f_dir.dir_attrib & attr & D_ALL )
					 || ( attr == D_ALL ) 
					 || ( attr == fnp ->f_dir.dir_attrib )
					 || ( (attr == (INT8U)D_FILE) && (!(fnp->f_dir.dir_attrib&D_DIR)) )
					 || ( (attr == (INT8U)D_FILE_1) 
					   && ((fnp->f_dir.dir_attrib == D_NORMAL) || !(fnp->f_dir.dir_attrib & (D_HIDDEN | D_SYSTEM | D_VOLID | D_DIR))) )
				  )
		        {
					/* If found, transfer it to the dmatch structure                */
					pstDiskFindControl->cur_pos.f_entry = fnp->f_dirstart;

				#if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb))
					{
						sfn_info->f_time = (INT16U)fnp->extmain_dir.CreateTimestamp;
						sfn_info->f_date = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
						sfn_info->f_size = fnp->ext_dir->ValidDataLength;
						sfn_info->f_attrib = fnp->f_dir.dir_attrib;					
					}
					else
				#endif	
					{
						sfn_info->f_time = fnp->f_dir.dir_time;
						sfn_info->f_date = fnp->f_dir.dir_date;
						sfn_info->f_size = fnp->f_dir.dir_size;
						sfn_info->f_attrib = fnp->f_dir.dir_attrib;
					}	



				
					for (i = 0; i < 8; i++)
					{
					  #if WITHEXFAT == 1
						if(ISEXFAT(fnp->f_dpb))
						{
							if (fnp->ShortName[i] == 0)
								break;
							sfn_info->f_name[i] = fnp->ShortName[i];						
						}
					  	else
					  #endif
					  	{
							if ( (fnp->f_dir.dir_name[i] == 0) || (fnp->f_dir.dir_name[i] == ' ') )
								break;
							sfn_info->f_name[i] = fnp->f_dir.dir_name[i];
					  	}
					}
					sfn_info->f_name[i] = 0;
					
					for (i = 0; i < 3; i++)
					{
					  #if WITHEXFAT == 1
						if(ISEXFAT(fnp->f_dpb))
						{
							if ( (fnp->ShortName[8+i] == 0) || (fnp->ShortName[8+i] == ' ') )
								break;
							sfn_info->f_extname[i] = fnp->ShortName[8+i];					
						}
					  	else
					  #endif
						{
							if ( (fnp->f_dir.dir_name[8+i] == 0) || (fnp->f_dir.dir_name[8+i] == ' ') )
								break;
							sfn_info->f_extname[i] = fnp->f_dir.dir_name[8+i];
						}	
					}
					sfn_info->f_extname[i] = 0;
					
					pstDiskFindControl->cur_pos.f_offset = fnp->f_diroff - 1;
					sfn_info->f_pos = pstDiskFindControl->cur_pos;		// Tristan: is this correct?
					pstDiskFindControl->cur_pos.f_offset++;
					
					// 080321 add for disk find speedup
				  	pstDiskFindControl->sfn_cluster = fnp->f_cluster;
				  	pstDiskFindControl->sfn_cluster_offset = fnp->f_cluster_offset;
				  	// 080321 add end
					
					release_f_node(fnp);
					return(&sfn_info->f_pos);
		        }
			}
		}	
	}
	else
	{
		while (lfn_readdir(fnp, psFind) == 0)
		{
		#if 0
			if( (fnp->f_dir.dir_name[0] == 'B') && 
				(fnp->f_dir.dir_name[1] == 'I') && 
				(fnp->f_dir.dir_name[2] == 'T') && 
				(fnp->f_dir.dir_name[3] == 'D') && 
				(fnp->f_dir.dir_name[4] == 'C') && 
				(fnp->f_dir.dir_name[5] == 'O') && 
				(fnp->f_dir.dir_name[6] == 'D') )
			{
				fnp->f_dir.dir_name[0] = 'A';
			}
		#endif

		#if WITHEXFAT == 1
			if(ISEXFAT(fnp->f_dpb))
			{
				memcpy((void *)ExtName,(void *)&fnp->ShortName[8],3);	
			}	
			else	
		#endif
			{
				memcpy((void *)ExtName,(void *)&fnp->f_dir.dir_name[8],3);
			}
			//判断文件名是否符合
			if ( !CheckExtName((CHAR *) ExtName, extname) )
			{	
				UTF8ToChart(Dir, &psFind->LongName);										// long name				
				
				//判断属性是否符合
				if ( ( fnp->f_dir.dir_attrib & attr & D_ALL )
					 || ( attr == D_ALL ) 
					 || ( attr == fnp ->f_dir.dir_attrib )
					 || ( (attr == (INT8U)D_FILE) && (!(fnp->f_dir.dir_attrib&D_DIR)) )
					 || ( (attr == (INT8U)D_FILE_1) 
					   && ((fnp->f_dir.dir_attrib == D_NORMAL) || !(fnp->f_dir.dir_attrib & (D_HIDDEN | D_SYSTEM | D_VOLID | D_DIR))) )
				  )
		        {
					if(filter((INT8S*)Dir))	// return 0, filter in; else filter out.
					{
						/* If found, transfer it to the dmatch structure                */
						pstDiskFindControl->cur_pos.f_entry = fnp->f_dirstart;

					#if WITHEXFAT == 1
						if(ISEXFAT(fnp->f_dpb))
						{
							sfn_info->f_time = (INT16U)fnp->extmain_dir.CreateTimestamp;
							sfn_info->f_date = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
							sfn_info->f_size = fnp->ext_dir->ValidDataLength;
							sfn_info->f_attrib = fnp->f_dir.dir_attrib;					
						}
						else
					#endif	
						{
							sfn_info->f_time = fnp->f_dir.dir_time;
							sfn_info->f_date = fnp->f_dir.dir_date;
							sfn_info->f_size = fnp->f_dir.dir_size;
							sfn_info->f_attrib = fnp->f_dir.dir_attrib;
						}	

						for (i = 0; i < 8; i++)
						{
						  #if WITHEXFAT == 1
							if(ISEXFAT(fnp->f_dpb))
							{
								if (fnp->ShortName[i] == 0)
									break;
								sfn_info->f_name[i] = fnp->ShortName[i];						
							}
						  	else
						  #endif
						  	{
								if ( (fnp->f_dir.dir_name[i] == 0) || (fnp->f_dir.dir_name[i] == ' ') )
									break;
								sfn_info->f_name[i] = fnp->f_dir.dir_name[i];
						  	}
						}
						sfn_info->f_name[i] = 0;
						
						for (i = 0; i < 3; i++)
						{
						  #if WITHEXFAT == 1
							if(ISEXFAT(fnp->f_dpb))
							{
								if ( (fnp->ShortName[8+i] == 0) || (fnp->ShortName[8+i] == ' ') )
									break;
								sfn_info->f_extname[i] = fnp->ShortName[8+i];					
							}
						  	else
						  #endif
							{
								if ( (fnp->f_dir.dir_name[8+i] == 0) || (fnp->f_dir.dir_name[8+i] == ' ') )
									break;
								sfn_info->f_extname[i] = fnp->f_dir.dir_name[8+i];
							}	
						}
						sfn_info->f_extname[i] = 0;
						
						pstDiskFindControl->cur_pos.f_offset = fnp->f_diroff - 1;
						sfn_info->f_pos = pstDiskFindControl->cur_pos;		// Tristan: is this correct?
						pstDiskFindControl->cur_pos.f_offset++;
						
						// 080321 add for disk find speedup
					  	pstDiskFindControl->sfn_cluster = fnp->f_cluster;
					  	pstDiskFindControl->sfn_cluster_offset = fnp->f_cluster_offset;
					  	// 080321 add end
						
						release_f_node(fnp);
						return(&sfn_info->f_pos);
		        	}
				}
			}
		}		
	}
	gFS_errno = DE_INVLDDRV; 
	release_f_node(fnp);
	return NULL;
}
#endif
