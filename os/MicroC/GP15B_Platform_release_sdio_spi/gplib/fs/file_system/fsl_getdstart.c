#include "fsystem.h"
#include "globals.h"

#if WITHFAT32 == 1 || WITHEXFAT == 1
CLUSTER getdstart(struct dpb *dpbp, struct dirent *dentry)
{
	#if WITHEXFAT == 1
	ExFATExtDIR *dent=(ExFATExtDIR *)dentry;
	#endif
	
	if (!(ISFAT32(dpbp)||ISEXFAT(dpbp))) {
    	return dentry->dir_start;
    }
	
	#if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		return (CLUSTER)dent->FirstCluster;
	}
	#endif
	
  	return (((CLUSTER) dentry->dir_start_high << 16) | dentry->dir_start);
}
#endif
