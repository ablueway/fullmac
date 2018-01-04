#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/* extends a file from f_dir.dir_size to f_offset              */
/* Proper OS's write zeros in between, but DOS just adds       */
/* garbage sectors, and lets the caller do the zero filling    */
/* if you prefer you can have this enabled using               */
/* #define WRITEZEROS 1                                        */
/* but because we want to be compatible, we don't do this by   */
/* default                                                     */

#define WRITEZEROS 0
INT16S dos_extend(f_node_ptr fnp)
{
  #if WRITEZEROS == 1
  	struct buffer *bp;
  	INT16U xfr_cnt = 0;
  	INT16U secsize = fnp->f_dpb->dpb_secsize;
	#if WITHEXFAT == 1
	  INT64U count;
	#else
	  INT32U count;
	#endif
  	INT16U sector, boff;
  #endif
	
  #if WITHEXFAT == 1
  	INT64U filesize;
  #else
	INT32U filesize;
  #endif


  #if WITHEXFAT == 1
  	if(ISEXFAT(fnp->f_dpb))
  	{
		filesize=fnp->ext_dir->ValidDataLength;
	}
	else
  #endif
  	    filesize=fnp->f_dir.dir_size;

  	if (fnp->f_offset <= filesize) {
    	return SUCCESS;
    }

  #if WRITEZEROS == 1 
  	count = fnp->f_offset - filesize;
  	fnp->f_offset = filesize;
  	while (count > 0)
  #endif
  	{
    	if (map_cluster(fnp, XFR_WRITE) != SUCCESS) {
      		return DE_HNDLDSKFULL;
      	}

	  #if WRITEZEROS == 1
    	// Compute the block within the cluster and the offset within the block.
    	sector = (INT8U) (fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask;
    	boff = (INT16U) (fnp->f_offset & 0x1ff);

    	//printf("write %d links; dir offset %ld, cluster %d\n", fnp->f_count, fnp->f_diroff, fnp->f_cluster);

    	xfr_cnt = count < (INT32U) (secsize - boff) ? (INT16U) count : (secsize - boff);

    	// get a buffer to store the block in
    	if ((boff == 0) && (xfr_cnt == secsize)) {
      		bp = getblockOver(clus2phys(fnp->f_cluster, fnp->f_dpb) + sector, fnp->f_dpb->dpb_unit);
    	} else {
      		bp = getblock(clus2phys(fnp->f_cluster, fnp->f_dpb) + sector, fnp->f_dpb->dpb_unit);
    	}
    	if (bp == NULL) {
      		return DE_BLKINVLD;
    	}

    	// set a block to zero
    	fmemset((INT8S *) & bp->b_buffer[boff >> WORD_PACK_VALUE], 0, xfr_cnt);
    	bp->b_flag |= BFR_DIRTY | BFR_VALID;

    	if (xfr_cnt == sizeof(bp->b_buffer)) {        // probably not used later
      		bp->b_flag |= BFR_UNCACHE;
    	}

    	// update pointers and counters
    	count -= xfr_cnt;
    	fnp->f_offset += xfr_cnt;
	  #endif

	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			fnp->ext_dir->ValidDataLength = fnp->f_offset;
			fnp->ext_dir->DataLength = fnp->f_offset;
		}
		else
	  #endif
    		fnp->f_dir.dir_size = fnp->f_offset;
    	merge_file_changes(fnp, FALSE);
  	}
  	return SUCCESS;
}
#endif //#ifndef READ_ONLY
