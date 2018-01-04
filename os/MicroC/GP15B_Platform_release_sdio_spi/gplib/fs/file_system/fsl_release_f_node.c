#include "fsystem.h"
#include "globals.h"

void release_f_node(f_node_ptr fnp)
{
  #if _SEEK_SPEED_UP == 1
	if (fnp->f_link_attribute) {
		if (fnp->plink_key_point != 0) {	// if memory allocate, just free 
			FS_SeekSpeedup_Free(fnp->plink_key_point);
			fnp->plink_key_point = 0; 			
		}

		if (fnp->plink_cache_buf != 0) {	// if memory allocate, just free 
			FS_SeekSpeedup_Free(fnp->plink_cache_buf);
			fnp->plink_cache_buf = 0; 						
		}
		// reset FLINK attribute
		fnp->f_link_attribute = 0x00;		 
	}
  #endif
	 
	fnp->f_count = 0x00;
	return;	
}