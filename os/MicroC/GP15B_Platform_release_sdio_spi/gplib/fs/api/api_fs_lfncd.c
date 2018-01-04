#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
INT16S fs_lfncd(CHAR *PathName)
{
	INT16S			i;
	f_node_ptr		fnp;
	
	INT8S			*Dir;
#if MALLOC_USE == 1
	INT8S			*path_max;
	tFatFind		*psFind;
#elif USE_GLOBAL_VAR == 1
	INT8S			*path_max = gFS_path1;
	tFatFind		*psFind = &gFS_sFindData;
#else
	STATIC INT8S		path_max[MAX_CDSPATH];
	STATIC tFatFind	sFindData;
	tFatFind		*psFind = &sFindData;
#endif
	struct cds		*cdsp;	
	INT8S 			*path_max_tmp;
	INT16S 			disk;
	INT16S 			ret = SUCCESS;
		
	// 首先判断路径名是否超长
	i = check_path_length((INT8S *) PathName, FD32_LFNPMAX);
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
		if (DE_UNITABERR == UnicodeToUTF8((INT8S *) PathName, (UTF8 *) path_max))
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{
	if (DE_UNITABERR == ChartToUTF8((INT8S *) PathName, (UTF8 *) path_max))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 	
	#endif
		return DE_UNITABERR;
	}
	}
	
	//把字符'/'转换成'\'，并把盘符转换成大写的
	pre_truename(path_max);
	
	//切换磁盘到路径制定的磁盘
	path_max_tmp = path_max;
	if (path_max[1] == ':')
	{
		if (path_max[0] - 'A' != default_drive)
		{
			disk = path_max[0];
			if (disk >= 'a' && disk <= 'z')
				disk -= 'a' - 'A';
		
			if (fs_changedisk(disk-'A')!=0)
			{
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)path_max); 	
			#endif
				return DE_INVLDDRV;
			}
		}
		path_max_tmp +=2;     
	}
	
	//获取磁盘的结构体及判断磁盘当前是否有效
	cdsp = get_cds(default_drive); 	 
	if (cdsp == NULL)				//guili 2006.5.16
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 	
	#endif	
		return DE_INVLDDRV;			//not check return value
	}						
	if ((media_check(cdsp->cdsDpb) < 0))
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 
	#endif
		return DE_INVLDDRV;
	}
	
	/* now test for its existance. If it doesn't, return an error.  */
	if ((fnp = descend_path((CHAR *) path_max_tmp, psFind)) == NULL)
	{	
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 
	#endif
		return gFS_errno;
	}

	if (!(fnp->f_dir.dir_attrib & D_DIR))
	{
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)path_max); 
	#endif
		return DE_PATHNOTFND; 	
  	}
	/* problem: RBIL table 01643 does not give a FAT32 field for the
	 CDS start cluster. But we are not using this field ourselves */
	cdsp->cdsStrtClst = (CLUSTER)fnp->f_dirstart;  

	dir_close(fnp);

#if WITH_CDS_PATHSTR == 1
	Dir = psFind->LongName;
	UTF8ToChart(Dir, path_max_tmp);
	if (Dir[0] == '\\') 
	{
		strcpy((CHAR *) CDS[default_drive].cdsCurrentPath+2,(CHAR *) Dir);
	}
	else if (Dir[0])
	{
		INT16S temp;
		
		temp = strlen((CHAR *) CDS[default_drive].cdsCurrentPath)-1;
		if (CDS[default_drive].cdsCurrentPath[temp]!='\\')
		{
			strcat((CHAR *) CDS[default_drive].cdsCurrentPath,"\\");
		}
		//guili 2006.5.29 start
		else
		{
			if ((INT8U)CDS[default_drive].cdsCurrentPath[temp-1] >= 0x80)
			{
				strcat((CHAR *) CDS[default_drive].cdsCurrentPath,"\\");
			}
		}		
		//guili 2006.5.29 end
	
		strcat((CHAR *) CDS[default_drive].cdsCurrentPath,(CHAR *) Dir);
	}
	ret = true_current_path((CHAR *) CDS[default_drive].cdsCurrentPath+2);
#endif

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)path_max); 
#endif
	return ret;
}

#endif
