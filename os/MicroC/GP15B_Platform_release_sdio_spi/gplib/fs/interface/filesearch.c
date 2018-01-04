#include "fsystem.h"
#include "globals.h"


// start search file, search first file
INT32S file_search_start(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
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
	
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;	
	struct STFolderNodeInfo		*pstFolderNodeInfo;
	
	INT16U	MarkDistance;
	INT8S 	*pExtName;
	
	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	MarkDistance = 1;
	
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
	fs_memset_word(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	fs_memset_word(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
	
	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	folderbuffersize = (stFNodeInfo->FolderBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFolderNodeInfo);
	if (pstFolderNodeInfo == NULL || folderbuffersize == 0)
		bCheckFolder = 0;
	else
		bCheckFolder = 1;
	
	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	//fppos = fs_getfirstfile_1(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr);
	fppos = fs_getfirstfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1,stFNodeInfo->filter);
	if (fppos == NULL)
	{
		gFS_errno = DE_FILENOTFND; 			//no file found
		FS_OS_UNLOCK();
		return -1;
	}
	
	if (bCheckFolder)
		sfn_getcurdirpos(pstDiskFindControl, &fpos1);
	
	// restore the first file to buffer
	pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
	pstFileNodeMark[i].level	=	pstDiskFindControl->level;
	pstFileNodeMark[i].entry	=	fppos->f_entry;
	pstFileNodeMark[i].offset	=	fppos->f_offset;
	if (pstDiskFindControl->level)
		memcpy(pstFileNodeMark[i].dirtrack, pstDiskFindControl->dir_offset, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE));
	
	i++;		

	if (bCheckFolder)
	{			
		sfn_getcurdirpos(pstDiskFindControl, &fpos2);
		if ( sfn_cmpdirpos(&fpos1, &fpos2) )
		{				
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
					bCheckFolder = 0;
				}
				pstFolderNodeInfo++;
			}
		}
		nFileInFolder++;
	}
			
	if (++count >= maxfile)
	{
		CompressFileNodeInfoBuffer(stFNodeInfo);
		
		MarkDistance <<= 1;
		if (MarkDistance >= C_MaxMarkDistance)
		{
			gFS_errno = DE_TOOMANYFILES;
			FS_OS_UNLOCK();
			return -1;
		}
		
		i = (i >> 1) + (i & 1);
		maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
	}
		

	if (bCheckFolder && nFileInFolder)
	{		
	#if SUPPORT_GET_FOLDER_NAME == 1
		pstFolderNodeInfo->entry = fpos1.f_entry;
		pstFolderNodeInfo->offset = fpos1.f_offset;
	#endif					
		pstFolderNodeInfo->filenum = nFileInFolder;
		nfolder++;
	}
	
	pstDiskFindControl->index = i;
	
	stFNodeInfo->flag = 2;		// flag == 2, means continue to search file
	stFNodeInfo->MaxFileNum = count;
	stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return 0;
}

// continue search file, every time search N files, 
// if search end, return -1. 
// if search n files, return n(may be 0 file).
// if search faile, return -x.
INT32S file_search_continue(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
{
	f_ppos	fppos;
	f_pos	fpos1, fpos2;
	INT16U	i = pstDiskFindControl->index;
	INT32U	count = stFNodeInfo->MaxFileNum;
	INT32U	nfolder = stFNodeInfo->MaxFolderNum;
	INT32U	maxfile;
	INT16U	filebuffersize;	
	INT16U	folderbuffersize;
	INT8S	bCheckFolder;
	INT16U	nFileInFolder = 0;
	INT32S	ret;
	
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;	
	struct STFolderNodeInfo		*pstFolderNodeInfo;
	
	INT16U	MarkDistance = stFNodeInfo->MarkDistance;	
	INT8S 	*pExtName;

	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;

	pstFolderNodeInfo = (struct STFolderNodeInfo*)stFNodeInfo->FolderBuffer;
	folderbuffersize = (stFNodeInfo->FolderBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFolderNodeInfo);
	if (pstFolderNodeInfo == NULL || folderbuffersize == 0)
		bCheckFolder = 0;
	else
		bCheckFolder = 1;
	
	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	
	while(1) {	
		//fppos = fs_getnextfile_1(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr);
		fppos = fs_getnextfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1,stFNodeInfo->filter);
		if (fppos == NULL)
		{
			ret = 1;		// if return 1, complete searching file			
			break;
		}
		
		if (bCheckFolder)
			sfn_getcurdirpos(pstDiskFindControl, &fpos1);
	
		if ( (count & (INT32U)(MarkDistance - 1) ) == 0)
		{
			pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
			pstFileNodeMark[i].level	=	pstDiskFindControl->level;
			pstFileNodeMark[i].entry	=	fppos->f_entry;
			pstFileNodeMark[i].offset	=	fppos->f_offset;
			if (pstDiskFindControl->level)
				memcpy(pstFileNodeMark[i].dirtrack, pstDiskFindControl->dir_offset, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE));
			
			i++;
		}
		
		if (bCheckFolder)
		{			
			sfn_getcurdirpos(pstDiskFindControl, &fpos2);
			if ( sfn_cmpdirpos(&fpos1, &fpos2) )
			{				
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
						bCheckFolder = 0;
					}
					pstFolderNodeInfo++;
				}
			}
			nFileInFolder++;
		}
				
		if (++count >= maxfile)
		{
			CompressFileNodeInfoBuffer(stFNodeInfo);
			
			MarkDistance <<= 1;
			if (MarkDistance >= C_MaxMarkDistance)
			{
				gFS_errno = DE_TOOMANYFILES;
				//FS_OS_UNLOCK();
				ret = -1;
			}
			
			i = (i >> 1) + (i & 1);	
			maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
		}
		ret = 0;
		break;
	}
	
	if (bCheckFolder && nFileInFolder)
	{		
	#if SUPPORT_GET_FOLDER_NAME == 1
		pstFolderNodeInfo->entry = fpos1.f_entry;
		pstFolderNodeInfo->offset = fpos1.f_offset;
	#endif					
		pstFolderNodeInfo->filenum = nFileInFolder;
		nfolder++;
	}

	if(ret == 1)
		stFNodeInfo->flag = 1;			// flag == 1, means search file complete
	else
		stFNodeInfo->flag = 2;			// flag == 2, means continue to search file
		
	pstDiskFindControl->index = i;	
	stFNodeInfo->MaxFileNum = count;
	stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return ret;
}

#if 1
// start search file, search first file
INT32S file_search_in_folder_start(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
{
	f_ppos	fppos;
	INT16U	i = 0;
	INT32U	count = 0;
	INT32U	nfolder = 0;
	INT32U	maxfile;
	INT16U	filebuffersize;	
	
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;
	
	INT16U	MarkDistance;
	INT8S 	*pExtName;
	
	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	MarkDistance = 1;
	
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
	fs_memset_word(stFNodeInfo->FileBuffer, 0xffff, stFNodeInfo->FileBufferSize);
	fs_memset_word(stFNodeInfo->FolderBuffer, 0xffff, stFNodeInfo->FolderBufferSize);
	
	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;

	//fppos = fs_getfirstfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1);
	fppos = fs_get_first_file_in_folder(pstDiskFindControl, stFNodeInfo->path, pExtName, &st_sfninfo, stFNodeInfo->attr, 0 ,stFNodeInfo->filter);
	if (fppos == NULL)
	{
		gFS_errno = DE_FILENOTFND; 			//no file found
		FS_OS_UNLOCK();
		return -1;
	}
		
	// restore the first file to buffer
	pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
	pstFileNodeMark[i].level	=	pstDiskFindControl->level;
	pstFileNodeMark[i].entry	=	fppos->f_entry;
	pstFileNodeMark[i].offset	=	fppos->f_offset;
	if (pstDiskFindControl->level)
		memcpy(pstFileNodeMark[i].dirtrack, pstDiskFindControl->dir_offset, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE));
	
	i++;
			
	if (++count >= maxfile)
	{
		CompressFileNodeInfoBuffer(stFNodeInfo);
		
		MarkDistance <<= 1;
		if (MarkDistance >= C_MaxMarkDistance)
		{
			gFS_errno = DE_TOOMANYFILES;
			FS_OS_UNLOCK();
			return -1;
		}
		
		i = (i >> 1) + (i & 1);
		maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
	}
	
	pstDiskFindControl->index = i;
	
	stFNodeInfo->flag = 2;		// flag == 2, means continue to search file
	stFNodeInfo->MaxFileNum = count;
	stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return 0;
}

// continue search file, every time search N files, 
// if search end, return -1. 
// if search n files, return n(may be 0 file).
// if search faile, return -x.
INT32S file_search_in_folder_continue(struct STFileNodeInfo *stFNodeInfo, STDiskFindControl *pstDiskFindControl)
{
	f_ppos	fppos;
	INT16U	i = pstDiskFindControl->index;
	INT32U	count = stFNodeInfo->MaxFileNum;
	INT32U	nfolder = stFNodeInfo->MaxFolderNum;
	INT32U	maxfile;
	INT16U	filebuffersize;	
	INT32S	ret;
	struct sfn_info				st_sfninfo;
	struct STFileNodeMark		*pstFileNodeMark;	
	INT16U	MarkDistance = stFNodeInfo->MarkDistance;	
	INT8S 	*pExtName;

	FS_OS_LOCK();
	
	filebuffersize = (stFNodeInfo->FileBufferSize << (1 - WORD_PACK_VALUE)) / sizeof(struct STFileNodeMark);
	stFNodeInfo->flag = 0;
	maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
	
	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	
	while(1) {
		//fppos = fs_getnextfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, &st_sfninfo, stFNodeInfo->attr, 1);
		fppos = fs_get_next_file_in_folder(pstDiskFindControl, stFNodeInfo->path, pExtName, &st_sfninfo, stFNodeInfo->attr, 0, stFNodeInfo->filter);
		if (fppos == NULL)
		{
			ret = 1;		// if return 1, complete searching file			
			break;
		}
			
		if ( (count & (INT32U)(MarkDistance - 1) ) == 0)
		{
			pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer;
			pstFileNodeMark[i].level	=	pstDiskFindControl->level;
			pstFileNodeMark[i].entry	=	fppos->f_entry;
			pstFileNodeMark[i].offset	=	fppos->f_offset;
			if (pstDiskFindControl->level)
				memcpy(pstFileNodeMark[i].dirtrack, pstDiskFindControl->dir_offset, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE));
			
			i++;
		}
				
		if (++count >= maxfile)
		{
			CompressFileNodeInfoBuffer(stFNodeInfo);
			
			MarkDistance <<= 1;
			if (MarkDistance >= C_MaxMarkDistance)
			{
				gFS_errno = DE_TOOMANYFILES;
				//FS_OS_UNLOCK();
				ret = -1;
			}
			
			i = (i >> 1) + (i & 1);	
			maxfile = (filebuffersize - 1) * (INT32U) MarkDistance;
		}
		ret = 0;
		break;
	}

	if(ret == 1)
		stFNodeInfo->flag = 1;			// flag == 1, means search file complete
	else
		stFNodeInfo->flag = 2;			// flag == 2, means continue to search file
		
	pstDiskFindControl->index = i;	
	stFNodeInfo->MaxFileNum = count;
	stFNodeInfo->MaxFolderNum = nfolder;
	stFNodeInfo->MarkDistance = MarkDistance;
	FS_OS_UNLOCK();
	return ret;
}
#endif




