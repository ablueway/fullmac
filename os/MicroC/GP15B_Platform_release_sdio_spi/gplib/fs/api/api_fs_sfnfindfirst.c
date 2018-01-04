/************************************************************************/
/* 
	change to father dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
f_ppos sfn_findfirst(STDiskFindControl *pstDiskFindControl, INT8S dsk, CHAR *extname, struct sfn_info *sfn_info, INT16S attr,INT32S (*filter)(INT8S *str))
{
	// 080321 add for disk find speedup
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	// 080321 add end
	
	if (dsk == pstDiskFindControl->cur_pos.f_dsk)
		pstDiskFindControl->cur_pos.f_offset = 0;
	else
		sfn_changedisk(pstDiskFindControl, dsk);		//如果disk改变了，需要从根目录开始查找
	return sfn_findnext(pstDiskFindControl, dsk, extname, sfn_info, attr,filter);
}
#endif
