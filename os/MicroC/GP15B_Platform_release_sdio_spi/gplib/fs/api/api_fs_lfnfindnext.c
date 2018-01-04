#include "fsystem.h"
#include "globals.h"


#if LFN_FLAG == 1

#define CHAR_BYTES 2

/************************************************************************/
/*                                                                      */
/************************************************************************/
BOOLEAN fcmp_lfnwild(const INT8S *match, const INT8S *dir)
{
	CHAR	*pdir = (CHAR *) dir;
	CHAR	*pmatch = (CHAR *) match;
	CHAR	set[3];
	CHAR	*bk_pdir = NULL;
	CHAR	*bk_pmatch = NULL; 
	
	if (!strcmp(pmatch, "*.*"))  
		return  TRUE;

	 set[2] = '\0';
	 while (*pdir)
	 {
	   if (((fs_toupper((INT8U) *pdir)) == (fs_toupper((INT8U) *pmatch)))  ||  *pmatch == '?')
	   {
		   if (((INT8U) *pdir) < 0x80) {
				 pdir++;
				 pmatch++;
		   }
		   else if (*pmatch == '?'){
				 pdir += CHAR_BYTES;
				 pmatch ++;
		   }
		   else if (!memcmp(pdir, pmatch, 1))
		   {
			   pdir ++;
			   pmatch ++;
		   }
		   else {
				 return FALSE;
		   }
	   }
	   else
	   {
			 if (*pmatch == 0)// && *bk_pmatch == 0)
			 {
				 if (bk_pmatch == 0) return FALSE;
				 pdir = bk_pdir;
				 pmatch = bk_pmatch;
				 continue;
			 }
				 //return FALSE; //add by matao 2005-2-19
 			 if (*pmatch++ == '*')
			 {
				 while (*pmatch == '*' || *pmatch == '?') pmatch++;
				 if (*pmatch != '\0')
 			    {
					 if (*pmatch <='z' && *pmatch >= 'a')
					 {
						 set[0] = *pmatch;
						 set[1] = *pmatch  - 'a'  +  'A';
					 }
					 else  if (*pmatch  <= 'Z' && *pmatch >= 'A')
					 {
					 	 set[0] = *pmatch  -  'A'  +  'a';
						 set[1] = *pmatch;
					 }
					 else
					 {
						 set[0] = *pmatch;
						 set[1] = '\0';
					 }
					 pdir = strpbrk(pdir, set);
					 if (pdir  != NULL)
					 {
						 bk_pdir = pdir  +  1;
						 bk_pmatch = pmatch  -  1;
					 }
				 }
				 else  return  TRUE;
			 }
			 else
			 {

				 if (bk_pdir  != NULL)
				 {
				 pdir = bk_pdir;
				 pmatch = bk_pmatch;
				 continue;
				 }
				 return  FALSE;
			 }
			 if (pdir == NULL)  
			 	return  FALSE;
	   }
	 }
	 if (*pmatch == '\0')
		 return  TRUE;
	 else
		 while (*pmatch)
		 if (*pmatch == '*')
		 {
			 pmatch++;
			 if (*pmatch == 0)  return  TRUE;
		 }
		 else
			 return  FALSE;
		 return  FALSE;
}

/************************************************************************/
/* 寻找下一个目录项
input:	
		f_info 结构体
output:
			
history:	
	[1/15/2006]
		输入的 f_info 结构体中就已经有存放文件名的空间了，目前的程序
		有进行了空间的申请，在 UNSP 小资源的情况下，导致申请不到内存。
		改为直接使用 f_info 结构体中的文件名空间
                                                                     */
/************************************************************************/
INT16S fs_lfnfindnext(struct f_info *f_info)
{
	REG f_node_ptr fnp;
	REG dmatch *dmp = &sda_tmp_dm;
#if MALLOC_USE == 1
	tFatFind*	psFind;
#elif USE_GLOBAL_VAR == 1
	tFatFind	*psFind = &gFS_sFindData;
#else
	tFatFind	sFindData;
	tFatFind*	psFind = &sFindData;
#endif
	INT8S *		Dir = f_info->f_name;	// 直接使用文件名空间，不再需要进行空间的申请

	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		return DE_MAX_FILE_NUM;
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
	
	/* Select the default to help non-drive specified path          */
	/* searches...                                                  */
	fnp->f_dpb = get_dpb(dmp->dm_drive);
	if (media_check(fnp->f_dpb) < 0)
	{
		release_f_node(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE((INT16U *)psFind);			
	#endif
		return DE_MAX_FILE_NUM;
	}

	dir_init_fnode(fnp, dmp->dm_dircluster);
	
	/* Search through the directory to find the entry, but do a     */
	/* seek first.                                                  */
	fnp->f_diroff = dmp->dm_entry;
	fnp->f_cluster = dmp->dm_cluster;                     /* 2012-08-20,add by duxiatang */
	fnp->f_cluster_offset = dmp->dm_cluster_offset;
	/* Loop through the directory  \                                 */
  #if WITHEXFAT == 1
  	if(ISEXFAT(fnp->f_dpb))
  	{
		psFind->NameHash = 0xFFFF;
		psFind->DIRAttr = 0x00FF; 
  	}
  #endif

	while (lfn_readdir(fnp, psFind) == 0)
	{
		dmp->dm_entry = fnp->f_diroff;
		dmp->dm_cluster = fnp->f_cluster;                  /* 2012-08-20,add by duxiatang */
		dmp->dm_cluster_offset = fnp->f_cluster_offset;
		memcpy((void*)&fnp->f_dir, (void*)&psFind->SfnEntry, sizeof(tDirEntry));
		
		if(gUnicodePage == UNI_UNICODE)
		{
			fd32_utf8to16((UTF8 *)&psFind->LongName, (UTF16 *)Dir);
		}
		else
		{
			UTF8ToChart(Dir, &psFind->LongName);
		}
		if (fcmp_lfnwild(dmp->dm_name_pat, psFind->LongName))
		{
			/*
			   MSD Command.com uses FCB FN 11 & 12 with attrib set to 0x16.
			   Bits 0x21 seem to get set some where in MSD so Rd and Arc
			   files are returned. 
			   RdOnly + Archive bits are ignored
			*/
		
			/* Test the attribute as the final step */
			if ( (fnp ->f_dir.dir_attrib & dmp->dm_attr_srch & D_ALL)
				|| (dmp->dm_attr_srch == D_ALL) 
				|| (dmp->dm_attr_srch == fnp ->f_dir.dir_attrib)
				|| (((INT8U)dmp->dm_attr_srch == (INT8U)D_FILE) && (!(fnp->f_dir.dir_attrib&D_DIR)) ) )
		    {
				/* If found, transfer it to the dmatch structure                */
				dmp->dm_dircluster = fnp->f_dirstart;
				
			#if ADD_ENTRY == 1
				f_info->entry = dmp->dm_currentfileentry;
			#endif

			#if WITHEXFAT == 1
				if(ISEXFAT(fnp->f_dpb))
				{
					f_info->f_time = (INT16U)(fnp->extmain_dir.CreateTimestamp);
					f_info->f_date = (INT16U)(fnp->extmain_dir.CreateTimestamp>>16);
					f_info->f_size = fnp->ext_dir->ValidDataLength;
					f_info->f_attrib = fnp->f_dir.dir_attrib;
					fmemcpy((UTF8*)psFind->ShortName,0,12);
				}
				else
			#endif
				{
					f_info->f_time = fnp->f_dir.dir_time;
					f_info->f_date = fnp->f_dir.dir_date;
					f_info->f_size = fnp->f_dir.dir_size;
					f_info->f_attrib = fnp->f_dir.dir_attrib;
					
					//UTF8ToChart(f_info->f_short_name, psFind->ShortName);
					utf8_to_oemcp((UTF8*)psFind->ShortName, -1, f_info->f_short_name, 12);
				}
				/* return the result                                            */
				release_f_node(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)psFind);
			#endif
				return SUCCESS;
			}
		}
	}

	release_f_node(fnp);
#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)psFind);
#endif	
	return DE_MAX_FILE_NUM;
}

INT16S fs_get_fnode_pos(f_pos *fpos)
{
	fpos->f_dsk = sda_tmp_dm.dm_drive;
	fpos->f_entry = sda_tmp_dm.dm_dircluster;
	fpos->f_offset = sda_tmp_dm.dm_entry - 1;
	return 0;
}

#endif
