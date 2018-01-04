#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if WITHFAT32 == 1
BOOLEAN checkdstart(struct dpb *dpbp, struct dirent *dentry, CLUSTER value)
{
	if (!ISFAT32(dpbp)) {
		return dentry->dir_start == (INT16U) value;
	}
	return (dentry->dir_start==(INT16U) value && dentry->dir_start_high==(INT16U) (value >> 16));
}
#endif
#endif
