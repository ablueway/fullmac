#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
INT16S fs_lfnfindfirst(INT8S *name, struct f_info *f_info, INT16U attr)
{
	REG f_node_ptr fnp;
	REG dmatch *dmp = &sda_tmp_dm;
	INT8S *path_max;
  #if MALLOC_USE == 1
	INT8S *Dir;
	INT8S *Name;
	tFatFind *psFind;
  #elif USE_GLOBAL_VAR == 1
	INT8S		*Dir = gFS_path1;
	INT8S		*Name = gFS_name1;
	tFatFind	*psFind = &gFS_sFindData;
  #else
	STATIC INT8S	Dir[FD32_LFNPMAX];
	STATIC INT8S	Name[FD32_LFNMAX];
	STATIC tFatFind	sFindData;
	tFatFind *psFind = &sFindData;
  #endif
	INT16S i;
	
	i = check_path_length(name, FD32_LFNPMAX);
	if (i < 0) {
		return i;
	}

  #if MALLOC_USE == 1
	Dir = (INT8S *) FS_MEM_ALLOC(2 * i + sizeof(tFatFind));	
	if (Dir == NULL) {
		return DE_NOMEM;
	}
	
	Name = (INT8S *) &Dir[i];	
	psFind = (tFatFind *) &Dir[2 * i];
  #endif
	
	path_max = psFind->LongName;
		//把路径转换成utf8类型
	if(gUnicodePage == UNI_UNICODE)
	{
		if (DE_UNITABERR == UnicodeToUTF8(name, (UTF8 *) path_max)) 
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)path_max); 	
		#endif
			return DE_UNITABERR;
		}
	}
	else
	{	
		if (DE_UNITABERR == ChartToUTF8(name, (UTF8 *) path_max)) 
		{
	  #if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *) Dir);
	  #endif
		return DE_UNITABERR;
	}
	}
	pre_truename(path_max);     //change '/' to'\'
	split_path_LFN((CHAR *) path_max, (CHAR *) Dir, (CHAR *) Name);
	
	if ((fnp = descend_path((CHAR *) Dir, psFind)) == NULL) {
	  #if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)Dir); 
	  #endif
		return gFS_errno;
	}

	// Now initialize the dirmatch structure.
	if ((Dir[0] == '\0') || (Dir[0] == '\\')) {
		dmp->dm_drive = default_drive; 
	} else {
		if (Dir[1] == ':') {
			dmp->dm_drive = Dir[0] - 'A'; 
		}
	}
	dmp->dm_attr_srch = attr & (D_FILE | D_RDONLY | D_HIDDEN | D_SYSTEM | D_DIR | D_ARCHIVE); 
	
	// Copy the raw pattern from our data segment to the DTA.
	fmemcpy(dmp->dm_name_pat, Name, strlen((CHAR *) Name)+1);  		

	dmp->dm_entry = 0;
//	dmp->dm_cluster = 0;
	dmp->dm_cluster_offset = 0;                            /* 2012-08-20,add by duxiatang */
	dmp->dm_cluster = dmp->dm_dircluster = fnp->f_dirstart;
	dir_close(fnp);
  #if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *) Dir); 
  #endif
	return fs_lfnfindnext(f_info);
}

#endif