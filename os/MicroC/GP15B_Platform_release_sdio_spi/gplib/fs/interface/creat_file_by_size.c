/************************************************************************/
/* 	
	zhangzha add。
	创建指定大小的文件。
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
static INT16S AllocFileSpace(INT16S fd, INT32U size);

//if file does not exist, create it. if file exist,return error
INT16S CreatFileBySize(CHAR *path,INT32U size)
{
	INT16S	fd;
	INT16S	ret;
	
	if (path == 0) {
		FS_OS_UNLOCK();	
		return -1;
	}

	FS_OS_LOCK();
	fd = fs_lfnopen(path,O_CREAT|O_RDWR,D_NORMAL);
	FS_OS_UNLOCK();
	if (fd < 0) {
		handle_error_code (fd);
		return -1;
	}
	
	ret = AllocFileSpace(fd, size);
	
	if (ret < 0) {
		close(fd);
		unlink(path);
		handle_error_code(ret);
		return -1;
	} else {
		lseek(fd, 0, 0);
		return fd;
	}
}

static INT16S AllocFileSpace(INT16S fd, INT32U size)
{
	REG f_node_ptr fnp;
	INT32U	FreeSpace;

	fnp = xlt_fd(fd);
	if ( (fnp == (f_node_ptr) 0)) {
		return DE_INVLDHNDL;
	}
	if (!fnp ->f_dpb ->dpb_mounted || !fnp->f_count) {						
		return DE_INVLDHNDL;
	}
	if (fnp->f_dir.dir_attrib & D_DIR) {
		return DE_ACCESS; 
	}
	FreeSpace = vfsFreeSpace(fnp->f_dpb->dpb_unit);
	if (size > FreeSpace)
		return DE_HNDLDSKFULL;

	FS_OS_LOCK();

	fnp->f_dir.dir_attrib |= D_ARCHIVE;
	fnp->f_flags |= F_DMOD;       /* mark file as modified */
	fnp->f_flags &= ~F_DDATE;     /* set date not valid any more */
	
	fnp->f_offset = size;
	
	if (dos_extend(fnp) != SUCCESS) {
		save_far_f_node(fnp);
		FS_OS_UNLOCK();	
		return DE_HNDLDSKFULL; 	 
	}	
	merge_file_changes(fnp, FALSE); 
	save_far_f_node(fnp);
	
	FS_OS_UNLOCK();	
	
	return SUCCESS;
}
#endif
