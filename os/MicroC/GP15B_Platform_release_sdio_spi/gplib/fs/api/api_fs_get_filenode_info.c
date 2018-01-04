#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

//根据index获取文件名节点信息
//注意执行此函数前请确保执行了函数GetFileNumEx
//0 <= nIndex < nMaxFileNum
f_ppos GetFileNodeInfo(struct STFileNodeInfo *stFNodeInfo, INT32U nIndex, struct sfn_info *f_info)
{
	f_pos	fpos;
	f_ppos	fppos;
	INT16U	IndexBase;
	INT16U	IndexOffset;
	INT8S 	*pExtName;
	
	struct STFileNodeMark	*pstFileNodeMark;
	
	INT16U	MarkDistance = stFNodeInfo->MarkDistance;
	INT32U	MaxFileNum = stFNodeInfo->MaxFileNum;
	
	STDiskFindControl	stDiskFindControl;
	STDiskFindControl	*pstDiskFindControl = &stDiskFindControl;
	
	FS_OS_LOCK();
	
	if (!stFNodeInfo->flag || !MaxFileNum || (nIndex >= MaxFileNum))
	{
		gFS_errno = DE_INVLDPARM;
		FS_OS_UNLOCK(); 
		return NULL;
	}
	
	IndexBase	= nIndex / MarkDistance;
	IndexOffset	= nIndex % MarkDistance;
	
	pstFileNodeMark = (struct STFileNodeMark *) stFNodeInfo->FileBuffer; 
	fpos.f_dsk = stFNodeInfo->disk;
	fpos.f_entry = pstFileNodeMark[IndexBase].entry;
	fpos.f_offset = pstFileNodeMark[IndexBase].offset;
	pstDiskFindControl->level = pstFileNodeMark[IndexBase].level;
	if (pstDiskFindControl->level)
		memcpy(pstDiskFindControl->dir_offset, pstFileNodeMark[IndexBase].dirtrack, pstDiskFindControl->level * (2 >> WORD_PACK_VALUE) );
	sfn_setcurpos(pstDiskFindControl, &fpos);
	
	// 080321 add for disk find speedup
  	pstDiskFindControl->sfn_cluster = 0;
  	pstDiskFindControl->sfn_cluster_offset = 0;
  	// 080321 add end
  	
  	// 080602 add for extend disk find funtion
  	if(stFNodeInfo->pExtName)
		pExtName = stFNodeInfo->pExtName;
	else
		pExtName = stFNodeInfo->extname;
	// 080602 add end
	
	if (IndexOffset)
	{
		pstDiskFindControl->find_begin_dir = 0;
		pstDiskFindControl->find_begin_file = 1;
		do
		{
			fppos = fs_getnextfile(pstDiskFindControl, stFNodeInfo->disk, pExtName, f_info, stFNodeInfo->attr, 0, stFNodeInfo->filter);
			if (fppos == NULL)
			{
				FS_OS_UNLOCK();
				return NULL;
			}
		} while (IndexOffset--);
	}
	else
	{
		INT16S fd;
		
		fd = fs_sfn_open(&fpos, RDWR);
		if (fd < 0)
		{
			FS_OS_UNLOCK();
			return NULL;
		}
		fs_sfn_stat(fd, f_info);
		fat_close(fd);
		f_info->f_pos = fpos;
	}
	
	FS_OS_UNLOCK();
	return &f_info->f_pos;
}
#endif