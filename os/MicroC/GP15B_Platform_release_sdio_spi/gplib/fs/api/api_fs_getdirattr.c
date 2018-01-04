#include "fsystem.h"
#include "globals.h"

INT16S fs_getdirattr(INT8S *dirpath, INT16U *attr) 
{
	f_node_ptr			fnp;
	INT8S*				path_max;
#if MALLOC_USE == 1
	INT8S*				Dir;
	INT8S*				Name;
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
	
	*attr = 0;

	i = check_path_length(dirpath,FD32_LFNPMAX);
	if (i < 0) 
		return i;		

#if MALLOC_USE == 1
	Dir = (INT8S *) FS_MEM_ALLOC(2 * i + sizeof(tFatFind));
	if (Dir == NULL)
		return DE_NOMEM;
	
	Name = (INT8S*)&Dir[i];
	psFind = (tFatFind*)&Dir[2 * i];
#endif

	path_max = psFind->LongName;
	
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(dirpath, (UTF8 *) path_max) )
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{	
	if (DE_UNITABERR == ChartToUTF8(dirpath, (UTF8 *) path_max) )
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

	/* Check that we don't have a duplicate name, so if we  */
	/* find one, it's an error.                             */
	if (fat_find(fnp, (CHAR *) Name, FD32_FRNONE | FD32_FAALL, psFind) == 0)
	{
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
	  	{
			*attr = fnp->extmain_dir.FileAttributes;
		}
		else
	  #endif
	  	{
			*attr = fnp->f_dir.dir_attrib;
	  	}
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE(Dir); 
	#endif
			
		return SUCCESS;
	}
	else
	{
		/* No such file, return the error               */
		dir_close(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *) Dir); 
	#endif
		return DE_FILENOTFND;
	}
}