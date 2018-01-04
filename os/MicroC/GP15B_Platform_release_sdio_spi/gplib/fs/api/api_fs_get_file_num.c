/************************************************************************/
/* 	
	zhangzha add。
	统计某种类型的文件的数量。
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"


//遍历整个磁盘的文件，统计出某种类型的文件的数量
int fs_StatFileNumByExtName(int dsk, INT8S * extname, INT32U *filenum)
{
	f_node_ptr fnp;
	struct f_node pre_f_node;
#if WITHEXFAT == 0
	tDirEntry  DirEntry;
#endif	
	INT16U level;

#if MALLOC_USE == 1
	INT16U *dir_off = FS_MEM_ALLOC(C_MAX_DIR_LEVEL);
  #if WITHEXFAT == 1
	INT32U *Clu_off = FS_MEM_ALLOC(C_MAX_DIR_LEVEL*2);
  #endif
#else
	INT16U dir_off[C_MAX_DIR_LEVEL];
	INT32U Clu_off[C_MAX_DIR_LEVEL];
#endif
	INT16S flag = 0;
	
	INT16S ret, i;
	INT32U count = 0;
	INT8S	localextname[3];	
#if WITHEXFAT == 1
	INT8S	fileextname[3];
#endif
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE(dir_off); 
	#endif	
		return DE_MAX_FILE_NUM;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;
	
	/* Select the default to help non-drive specified path          */
	/* searches...                                                  */
	fnp->f_dpb = get_dpb(dsk);
	if (media_check(fnp->f_dpb) < 0)
	{
		release_f_node(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE(dir_off); 
	#endif	
		return DE_MAX_FILE_NUM;
	}
	
	for (i = 0; i < 3; i++)
	{
		if (extname[i] == '\0')
		{
			for (; i < 3; i++)
				localextname[i] = ' ';
			break;
		}
		else
			localextname[i] = extname[i];
	}
		
	fs_strupr(localextname);
	
	//start from the first of root directory
	level = 0;
	fnp->f_diroff = dir_off[level] = 0;
	dir_init_fnode(fnp, level);
	
	while (1)
	{   	
#if WITHEXFAT == 1
		ret = sfn_readdir(fnp);
		if (ret == FD32_ENMFILE)
		{
			if (level == 0)
			{
				release_f_node(fnp);
			  #if MALLOC_USE == 1
				FS_MEM_FREE(dir_off); 
			  #endif				
				*filenum = count;
				return 0;
			}
			else
			//回到上一级目录
			{
				//这里置一个标志，并且设fnp->f_diroff为1，去读".."目录，以返回上一级目录
				if(!ISEXFAT(fnp->f_dpb))
				{
					flag = 1;
					fnp->f_diroff = 1;
					continue;
				}
			#if WITHEXFAT == 1
				else
				{
					//进入上一级目录
					level--;
					if (level == 0)
						dir_init_fnode(fnp, level);
					else
						//指向上一级目录的起始地址
						fnp->f_cluster = fnp->f_dirstart = Clu_off[level];
					//恢复上一级目录的偏移量
					fnp->f_diroff = dir_off[level];
				}
			#endif
				
			}
		}
		else
		{			
			//如果是目录，则进入该目录，修改f_dirstart到该目录指向的地址，
			//备份f_diroff，并置为0
			//上一级目录的地址最好做一个备份，否则如果当前目录有问题的话就没办法回去了^_^
			if ((fnp->f_dir.dir_attrib& D_DIR) == D_DIR)		
			{
				//"."
				if ( (level != 0) && (fnp->f_diroff == 1)&&(!ISEXFAT(fnp->f_dpb)))
				{
					if ( !memcmp((void*)fnp->f_dir.dir_name, ".          ", 11) )
					//扫描该目录的占用情况
					{
					
					}
					//非法的目录，回到上一级目录。如果是"."和".."文件，
					else
					{
						//进入上一级目录
						level--;
						//恢复上一级目录的f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				//".."
				else if ( (level != 0) && (fnp->f_diroff == 2)&&(!ISEXFAT(fnp->f_dpb)))
				{
					if (!memcmp((void*)fnp->f_dir.dir_name,  "..         ", 11) )
					//填充上一级目录的fnp，回到上一级目录
					{
						if (flag == 1)
						{
							flag = 0;
							//进入上一级目录
							level--;
							if (level == 0)
								dir_init_fnode(fnp, level);
							else
								//指向上一级目录的起始地址
								fnp->f_cluster = fnp->f_dirstart = (((INT32U) fnp->f_dir.dir_start_high<< 16) + (INT32U) fnp->f_dir.dir_start);
							//恢复上一级目录的偏移量
							fnp->f_diroff = dir_off[level];
						}
					}
					else
					//非法的目录，根据备份的fnp回到上一级目录。
					{
						//进入上一级目录
						level--;
						//恢复上一级目录的f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				else
				{
					flag = 0;
				#if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb)) {					
						if(level>=(C_MAX_DIR_LEVEL-1)) {						
							continue;
						}
					}
				#endif
					//保存上一级目录的偏移量
					dir_off[level] = fnp->f_diroff;
				#if WITHEXFAT == 1
					Clu_off[level] = fnp->f_dirstart;
				#endif
					//上一级目录的f_node做一个备份
					pre_f_node = *fnp;		// Tristan: Is this correct?
					//进入下一级目录
					level++;
					//下一级目录从头开始扫描
					fnp->f_diroff = 0;
					//指向下一级目录的起始地址
				
				#if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb))
					{
						fnp->f_cluster = fnp->f_dirstart = fnp->ext_dir->FirstCluster;
					}
					else
				#endif
					{
						fnp->f_cluster = fnp->f_dirstart = (((INT32U) fnp->f_dir.dir_start_high<< 16) + (INT32U) fnp->f_dir.dir_start);
					}
					continue;
				}
			}
			//如果是文件，则扫描该文件的fat表的占用情况，
			else
			{
				if ( !memcmp(localextname, "*", 1) )
				{
					count++;
				}
			  #if WITHEXFAT == 1
				else{
					if(ISEXFAT(fnp->f_dpb))
					{
						memcpy((void *)fileextname,(void*)(fnp->ShortName+8),3);
						for(i=0;i<3;i++){
							fileextname[i] = fs_toupper(fileextname[i]);
						}
					}
					else
					{
						memcpy((void *)fileextname,(void*)(fnp->f_dir.dir_name+8),3);
					}
					if(!memcmp((void*)fileextname, localextname, 3)){
						count++;
					}
				}
			  #else	
				else if ( !memcmp((void*)(fnp->f_dir.dir_name+8), localextname, 3) )
				{
					count++;
				}
			  #endif	
				
				
			}
			
		}


#else	
		ret = dir_lfnread(fnp, (tDirEntry *)&DirEntry);
		if (ret == DE_BLKINVLD)
		{
			release_f_node(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE(dir_off); 
		#endif
			
			*filenum = count;
			return FD32_ENMFILE;
		}	
			
		/* whole directory scanned end		 						*/
		if ( (ret == DE_HNDLDSKFULL) || (DirEntry.Name[0] == ENDOFDIR) )
		{
			if (level == 0)
			{
				release_f_node(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE(dir_off); 
			#endif
				
				*filenum = count;
				return 0;
			}
			else
			//回到上一级目录
			{
				//这里置一个标志，并且设fnp->f_diroff为1，去读".."目录，以返回上一级目录
				flag = 1;
				fnp->f_diroff = 1;
				continue;
			}
		}
		//遇到删除项
		else if ((INT8U)(DirEntry.Name[0]) == FREEENT)
		{
			fnp->f_diroff += 1;
			continue;
		}
		//长目录项
		else if (DirEntry.Attr == FD32_ALNGNAM) 

		{
			fnp->f_diroff++;
			continue;
		}		
		//短目录项
		else
		{			
			//如果是目录，则进入该目录，修改f_dirstart到该目录指向的地址，
			//备份f_diroff，并置为0
			//上一级目录的地址最好做一个备份，否则如果当前目录有问题的话就没办法回去了^_^
			if ((DirEntry.Attr & D_DIR) == D_DIR)		
			{
				//"."
				if ( (level != 0) && (fnp->f_diroff == 0) )
				{
					if ( !memcmp((void*)DirEntry.Name, ".          ", 11) )
					//扫描该目录的占用情况
					{

					}
					//非法的目录，回到上一级目录。如果是"."和".."文件，
					else
					{
						//进入上一级目录
						level--;
						//恢复上一级目录的f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				//".."
				else if ( (level != 0) && (fnp->f_diroff == 1) )
				{
					if (!memcmp((void*)DirEntry.Name,  "..         ", 11) )
					//填充上一级目录的fnp，回到上一级目录
					{
						if (flag == 1)
						{
							flag = 0;
							//进入上一级目录
							level--;
							if (level == 0)
								dir_init_fnode(fnp, level);
							else
								//指向上一级目录的起始地址
								fnp->f_cluster = fnp->f_dirstart = FIRSTCLUSTER(DirEntry);
							//恢复上一级目录的偏移量
							fnp->f_diroff = dir_off[level];
						}
					}
					else
					//非法的目录，根据备份的fnp回到上一级目录。
					{
						//进入上一级目录
						level--;
						//恢复上一级目录的f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				else
				{
					flag = 0;
					//保存上一级目录的偏移量
					dir_off[level] = fnp->f_diroff;
					//上一级目录的f_node做一个备份
					pre_f_node = *fnp;		// Tristan: Is this correct?
					//进入下一级目录
					level++;
					//下一级目录从头开始扫描
					fnp->f_diroff = 0;
					//指向下一级目录的起始地址
					fnp->f_cluster = fnp->f_dirstart = FIRSTCLUSTER(DirEntry);
					continue;
				}
			}
			//如果是文件，则扫描该文件的fat表的占用情况，
			else
			{
				if ( !memcmp(localextname, "*", 1) )
				{
					count++;
				}
				else if ( !memcmp((void*)(DirEntry.Name+8), localextname, 3) )
				{
					count++;
				}
			}
			fnp->f_diroff += 1;		
		}	
#endif		
	} 
}


