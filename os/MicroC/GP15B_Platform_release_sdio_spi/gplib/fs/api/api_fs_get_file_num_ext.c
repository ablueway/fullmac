#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

INT16S CompressFileNodeInfoBuffer(struct STFileNodeInfo *stFNodeInfo)
{
	INT16U					i;
	struct STFileNodeMark	*pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
	INT16U					maxsize = ( stFNodeInfo->FileBufferSize / sizeof(struct STFileNodeMark) ) >> WORD_PACK_VALUE;
		
	for (i = 1; i < maxsize; i++)
	{
		pstFileNodeMark[i] = pstFileNodeMark[i << 1];
	}
	return 0;
}

INT16S GetFileNumEx(struct STFileNodeInfo *stFNodeInfo, INT32U *nFolderNum, INT32U *nFileNum)
{
	f_ppos	fppos;
	f_pos	fpos1, fpos2;
	INT16U	i = 0;
	INT32U	count = 0;
	INT32U	nfolder = 0;
	INT32U	maxfile;
	INT16U	filebuffersize;	
	INT16U	folderbuffersize;
	INT8S	bCheckFolder;
	INT16U	nFileInFolder = 0;
	INT8S 	*pExtName;
	
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;	
	struct STFolderNodeInfo		*pstFolderNodeInfo;
	
	INT16U	MarkDistance = stFNodeInfo->MarkDistance;	
	STDiskFindControl	stDiskFindControl;
	STDiskFindControl	*pstDiskFindControl = &stDiskFindControl;
	
	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	MarkDistance = 1;
	
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
#ifdef unSP
	memset(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	memset(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
#else
	fs_memset_word(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	fs_memset_word(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
#endif
	
	pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	folderbuffersize = (stFNodeInfo->FolderBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFolderNodeInfo);
	if (pstFolderNodeInfo == NULL || folderbuffersize == 0)
		bCheckFolder = 0;
	else
		bCheckFolder = 1;
	
	// 080602 add for extend disk find funtion
  	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	// 080602 add end
	
	// 090812 add for the config of former version
	stFNodeInfo->attr = D_ALL;
	// 090812 add end
	//fppos = fs_getfirstfile_1(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr);
	fppos = fs_getfirstfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1,stFNodeInfo->filter);
	if (fppos == NULL)
	{
		*nFileNum = 0;
		gFS_errno = DE_FILENOTFND; 			//no file found
		FS_OS_UNLOCK();
		return -1;
	}
	
	if (bCheckFolder)
	{
		sfn_getcurdirpos(pstDiskFindControl, &fpos1);
	}
	
	do
	{
		//到指定个数的个文件了，把标记记入STFileNodeMark结构体
		if ( (count & (INT32U)(MarkDistance - 1) ) == 0)
		{
			pstFileNodeMark[i].level	=	pstDiskFindControl->level;
			pstFileNodeMark[i].entry	=	fppos->f_entry;
			pstFileNodeMark[i].offset	=	fppos->f_offset;
			if (pstDiskFindControl->level)
				memcpy(pstFileNodeMark[i].dirtrack, pstDiskFindControl->dir_offset, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE));
			
			i++;
		}
		
		if (bCheckFolder)
		{
			//判断目录是否有变化，如有变化，记录目录信息
			sfn_getcurdirpos(pstDiskFindControl, &fpos2);
//			if ( sfn_cmpdirpos(&fpos1, &fpos2) )
			if(pstDiskFindControl->dir_change_flag)
			{
				//目录已改变
				pstDiskFindControl->dir_change_flag = 0;
				if (nFileInFolder)
				{
				#if SUPPORT_GET_FOLDER_NAME == 1
					pstFolderNodeInfo->entry = fpos1.f_entry;
					pstFolderNodeInfo->offset = fpos1.f_offset;
				#endif
					pstFolderNodeInfo->filenum = nFileInFolder;
					
					fpos1 = fpos2;
					nFileInFolder = 0;
					
					nfolder++;
					if (nfolder >= folderbuffersize - 1)
					{
						//当folder数目超出user给定的buffer size时，不再继续统计目录
						bCheckFolder = 0;
					}
					pstFolderNodeInfo++;
				}
			}
			nFileInFolder++;
		}
		
		//大于最多支持的文件数
		if (++count >= maxfile)
		{
		#if 1
			INT16U temp = MarkDistance;
			
			temp <<= 1;
			if(temp >= C_MaxMarkDistance)
			{
				break;
			}
			i = (i >> 1) + (i & 1);
			maxfile = (filebuffersize - 1) * (INT32U) temp;
			MarkDistance = temp;
			CompressFileNodeInfoBuffer(stFNodeInfo);
		#else
			CompressFileNodeInfoBuffer(stFNodeInfo);
			
			MarkDistance <<= 1;
			if (MarkDistance >= C_MaxMarkDistance)
			{
				*nFileNum = 0;
				gFS_errno = DE_TOOMANYFILES;
				FS_OS_UNLOCK();
				return -1;
			}
			
			i = (i >> 1) + (i & 1);	//i >>= 1;
			maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
		#endif
		}
		
		//fppos = fs_getnextfile_1(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr);
		fppos = fs_getnextfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1, stFNodeInfo->filter);
	} while (fppos);
	
	if (bCheckFolder && nFileInFolder)
	{
		//记录最后一个目录信息
	#if SUPPORT_GET_FOLDER_NAME == 1
		pstFolderNodeInfo->entry = fpos1.f_entry;
		pstFolderNodeInfo->offset = fpos1.f_offset;
		stFNodeInfo->root_dir_flag = pstDiskFindControl->root_dir_flag;
	#endif					
		pstFolderNodeInfo->filenum = nFileInFolder;
		nfolder++;
	}
	
	stFNodeInfo->flag = 1;
	*nFileNum = stFNodeInfo->MaxFileNum = count;
	*nFolderNum = stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return 0;
}
/*-------- zurong add for search folder ---------------  */
INT16S GetFileNumInFolderEx(struct STFileNodeInfo *stFNodeInfo,INT32U *nFolderNum, INT32U *nFileNum)	// 获取当前folder个数以及file个数
{	
	INT16S ret;
	f_node_ptr	fnp;	
	
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{		
		return -1;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode 	= RDWR;	
	fnp->f_dpb 		= get_dpb(stFNodeInfo->disk);		
    fnp->f_dirstart = CDS[stFNodeInfo->disk].cdsStrtClst;
	fnp->f_dir.dir_attrib = D_DIR; //by jk 2005/02/21
	
	if (media_check(fnp->f_dpb) < 0)
	{	
		release_f_node(fnp);
		return -1;
	}	
	
	/* --- initial folder information --- */
	gstDiskFindControl.cur_pos.f_dsk 	= stFNodeInfo->disk;
	gstDiskFindControl.cur_pos.f_entry 	= fnp->f_dirstart;
	gstDiskFindControl.cur_pos.f_offset = 0;
	gstDiskFindControl.sfn_cluster		= 0;
	/* --- initial folder information end --- */
	
	release_f_node(fnp);
	
	gstDiskFindControl.level = 0;
				
	ret = SearchFileInFolder(stFNodeInfo, nFolderNum, nFileNum);
		
	return ret;
}

INT16S SearchFileInFolder(struct STFileNodeInfo *stFNodeInfo,INT32U *nFolderNum, INT32U *nFileNum)
{
	//f_pos	fpos;
	f_ppos	fppos;
	INT16U	i = 0;
	INT32U	count = 0;
	INT32U	nfolder = 0;
	INT32U	maxfile;
	INT16U	filebuffersize;	
	INT16U	folderbuffersize;
	INT8S	bCheckFolder;
	//INT16U	nFileInFolder = 0;
	INT8S 	*pExtName;
	INT16U	MarkDistance = stFNodeInfo->MarkDistance;
			
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;	
	struct STFolderNodeInfo		*pstFolderNodeInfo;	
	STDiskFindControl			stDiskFindControl;
	STDiskFindControl			*pstDiskFindControl = &stDiskFindControl;
	
	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	MarkDistance = 1;
	
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
#ifdef unSP
	memset(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	memset(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
#else
	fs_memset_word(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	fs_memset_word(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
#endif
	
	pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	folderbuffersize = (stFNodeInfo->FolderBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFolderNodeInfo);
	if(pstFolderNodeInfo == NULL || folderbuffersize == 0)
		bCheckFolder = 0;
	else
		bCheckFolder = 1;
	
	// 080602 add for extend disk find funtion
  	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	// 080602 add end
	
	// 090812 add for the config of former version
	stFNodeInfo->attr = D_ALL;
	// 090812 add end

	while((fppos = sfn_findnext(&gstDiskFindControl,stFNodeInfo->disk, (CHAR *)pExtName, &st_sfninfo, D_ALL, stFNodeInfo->filter)),fppos)
	{		
		//到指定个数的个文件了，把标记记入STFileNodeMark结构体
		if ( (count & (INT32U)(MarkDistance - 1) ) == 0)
		{
			pstFileNodeMark[i].level	=	pstDiskFindControl->level;
			pstFileNodeMark[i].entry	=	fppos->f_entry;
			pstFileNodeMark[i].offset	=	fppos->f_offset;
	
			i++;
		}
		
		//大于最多支持的文件数
		if(++count >= maxfile)
		{
		#if 1
			INT16U temp = MarkDistance;
			
			temp <<= 1;
			if(temp >= C_MaxMarkDistance)
			{
				break;
			}
			i = (i >> 1) + (i & 1);
			maxfile = (filebuffersize - 1) * (INT32U) temp;
			MarkDistance = temp;
			CompressFileNodeInfoBuffer(stFNodeInfo);
		#else
			CompressFileNodeInfoBuffer(stFNodeInfo);
			
			MarkDistance <<= 1;
			if (MarkDistance >= C_MaxMarkDistance)
			{
				*nFileNum = 0;
				gFS_errno = DE_TOOMANYFILES;
				FS_OS_UNLOCK();
				return -1;
			}
			
			i = (i >> 1) + (i & 1);	//i >>= 1;
			maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
		#endif
		}
		
	 }
	
	stFNodeInfo->flag = 1;
	*nFileNum = stFNodeInfo->MaxFileNum = count;
	*nFolderNum = stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return 0;
}/*----- search folder end ---------------------  */

//根据目录的index获取目录中的特定扩展名的文件的数目
//注意执行此函数前请确保执行了函数GetFileNumEx
INT16S GetFileNumOfFolder(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT16U *nFile)
{
#if	SUPPORT_GET_FOLDER_NAME == 1
	struct STFolderNodeInfo *pstFolderNodeInfo;
	INT32U	MaxFileNum = stFNodeInfo->MaxFileNum;
	INT32U	MaxFolderNum = stFNodeInfo->MaxFolderNum;
	
	FS_OS_LOCK();
	
	if (!stFNodeInfo->flag || !MaxFileNum || (nFolderIndex >= MaxFolderNum))
	{
		*nFile = 0;
		gFS_errno = DE_INVLDPARM;
		FS_OS_UNLOCK(); 
		return -1;
	}
	
	pstFolderNodeInfo = (struct STFolderNodeInfo *) stFNodeInfo->FolderBuffer;	
	*nFile = (pstFolderNodeInfo + nFolderIndex)->filenum;
	
	FS_OS_UNLOCK();
	return 0;
#else
	return -1;
#endif
}

//根据folder的index得到该folder的第一个文件的index
//注意执行此函数前请确保执行了函数GetFileNumEx
INT16S FolderIndexToFileIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, INT32U *nFileIndex)
{
#if	SUPPORT_GET_FOLDER_NAME == 1
	INT32U	i;
	INT32U	MaxFileNum = stFNodeInfo->MaxFileNum;
	INT32U	MaxFolderNum = stFNodeInfo->MaxFolderNum;
	
	struct STFolderNodeInfo *pstFolderNodeInfo;
	
	FS_OS_LOCK();
	
	*nFileIndex = 0;
	
	if (!stFNodeInfo->flag || !MaxFileNum || (nFolderIndex >= MaxFolderNum))
	{
		gFS_errno = DE_INVLDPARM;
		FS_OS_UNLOCK();
		return -1;
	}
	
	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	for (i = 0; i < nFolderIndex; i++)
	{
		*nFileIndex += (pstFolderNodeInfo++)->filenum;
	}
	
	FS_OS_UNLOCK();
	return 0;
#else
	return -1;
#endif
}

//根据file的index得到该file所在的folder的index
//注意执行此函数前请确保执行了函数GetFileNumEx
INT16S FileIndexToFolderIndex(struct STFileNodeInfo *stFNodeInfo, INT32U nFileIndex, INT32U *nFolderIndex)
{
#if	SUPPORT_GET_FOLDER_NAME == 1
	INT32U	i;
	INT32U	nFile = 0;
	INT32U	MaxFileNum = stFNodeInfo->MaxFileNum;
	INT32U	MaxFolderNum = stFNodeInfo->MaxFolderNum;
	
	struct STFolderNodeInfo *pstFolderNodeInfo;
	
	FS_OS_LOCK();
	
	*nFolderIndex = 0;
	
	if (!stFNodeInfo->flag || !MaxFileNum || (nFileIndex >= MaxFileNum))
	{
		gFS_errno = DE_INVLDPARM;
		FS_OS_UNLOCK(); 
		return -1;
	}
	
	pstFolderNodeInfo = (struct STFolderNodeInfo *) stFNodeInfo->FolderBuffer;	
	for (i = 0; i < MaxFolderNum; i++)
	{
		nFile += (pstFolderNodeInfo++)->filenum;
		if (nFileIndex < nFile)
			break;
	}
		
	*nFolderIndex = i;
	
	FS_OS_UNLOCK();
	return 0;
#else
	return -1;
#endif
}

//根据目录index获取该目录节点的信息
//注意执行此函数前请确保执行了函数GetFileNumEx
f_ppos GetFolderNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nFolderIndex, struct sfn_info *f_info)
{
#if SUPPORT_GET_FOLDER_NAME == 1
	INT16S	fd;
	f_pos	fpos;
	INT32U	MaxFileNum = stFNodeInfo->MaxFileNum;
	INT32U	MaxFolderNum = stFNodeInfo->MaxFolderNum;
	
	struct STFolderNodeInfo*	pstFolderNodeInfo;
	
	FS_OS_LOCK();
	
	if (!stFNodeInfo->flag || !MaxFileNum || (nFolderIndex >= MaxFolderNum))
	{
		gFS_errno = DE_INVLDPARM;
		FS_OS_UNLOCK(); 
		return NULL;
	}
	
	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	pstFolderNodeInfo += nFolderIndex;
	
	fpos.f_dsk		=	stFNodeInfo->disk;
	fpos.f_entry	=	pstFolderNodeInfo->entry;
	fpos.f_offset	=	pstFolderNodeInfo->offset;
	
	if((nFolderIndex == 0) && (stFNodeInfo->root_dir_flag))
	{
		// is root dir
		fpos.f_is_root = 1;
		f_info->f_time = 0;
		f_info->f_date = 0;
		f_info->f_size = 0;
		f_info->f_attrib = D_DIR;
		strcpy((CHAR *) f_info->f_name, "RootDir");
	}
	else
	{
		fpos.f_is_root = 0;
		fd = fs_sfn_open(&fpos, RDWR);
		if (fd < 0)
		{
			gFS_errno = DE_FILENOTFND;
			FS_OS_UNLOCK();
			return NULL;
		}
		fs_sfn_stat(fd, f_info);
		fat_close(fd);
	}
	f_info->f_pos = fpos;
	
	FS_OS_UNLOCK(); 
	return &f_info->f_pos;
#else
	return NULL;
#endif
}

#endif