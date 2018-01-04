#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if WITHFAT32 == 1 || WITHEXFAT == 1 
void setdstart(struct dpb *dpbp, struct dirent *dentry, CLUSTER value)
{
	ExFATExtDIR *ExtDTIR=(ExFATExtDIR *)dentry;
	
	#if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		ExtDTIR->FirstCluster=value;
	}else{
	#endif	
	  	dentry->dir_start = (INT16U) value;
	  	if (ISFAT32(dpbp)) {
	    	dentry->dir_start_high = (INT16U) (value >> 16);
	    }
	#if WITHEXFAT == 1
	}
	#endif		
}
#endif
#endif
