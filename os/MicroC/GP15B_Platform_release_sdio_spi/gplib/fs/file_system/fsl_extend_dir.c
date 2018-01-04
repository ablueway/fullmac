#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 0
INT16S extend_dir(f_node_ptr fnp)
{
  	REG INT16S idx;
	CLUSTER cluster;
	
  	cluster = extend(fnp);
  	if (cluster == LONG_LAST_CLUSTER) {
    	dir_close(fnp);
    	return DE_HNDLDSKFULL;
  	}

  	// clear out the blocks in the cluster
  	for (idx=0; idx<=fnp->f_dpb->dpb_clsmask; idx++) {
    	REG struct buffer *bp;

    	// as we are overwriting it completely, don't read first
    	bp = getblockOver(clus2phys(cluster, fnp->f_dpb) + idx, fnp->f_dpb->dpb_unit);

	    if (bp == NULL) {
      		dir_close(fnp);
      		return DE_BLKINVLD;
    	}
    	fmemset(bp->b_buffer, 0, BUFFERSIZE);
    	bp->b_flag |= BFR_DIRTY | BFR_VALID;

    	if (idx != 0) {
      		bp->b_flag |= BFR_UNCACHE;        // needs not be cached
      	}
  	}

  	if (!find_free(fnp)) {
    	dir_close(fnp);
    	return DE_HNDLDSKFULL;
  	}

  	// flush the drive buffers so that all info is written
  	flush_buffers(fnp->f_dpb->dpb_unit);

  	return SUCCESS;
}
#endif
#endif	//#ifndef READ_ONLY