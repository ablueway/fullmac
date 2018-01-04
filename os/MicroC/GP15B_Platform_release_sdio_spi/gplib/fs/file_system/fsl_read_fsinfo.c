#include "fsystem.h"
#include "globals.h"

#if WITHFAT32 == 1
INT16S read_fsinfo(struct dpb *dpbp)
{
	struct buffer *bp;
	
	if (dpbp->dpb_fsinfoflag & (FSINFODIRTYFLAG | FSINFOLOADED)) {
		return 0;
	}

	dbg_info1("dpbp->dpb_xfsinfosec = %d\n", dpbp->dpb_xfsinfosec);
	if(!ISEXFAT(dpbp))
	{
	    bp = getblock(dpbp->dpb_xfsinfosec, dpbp->dpb_unit);
	    if (bp == NULL) {
		    return -1;
	    }
   	    dpbp->dpb_xcluster = getlong(&bp->b_buffer[0x1ec>>WORD_PACK_VALUE]);
	    bp->b_flag &= ~(BFR_DATA | BFR_DIR | BFR_FAT | BFR_DIRTY);
	    bp->b_flag |= BFR_VALID;
    }
	
	#if WITHEXFAT == 1 
	if(ISEXFAT(dpbp))    
	{
	    dpbp->dpb_xcluster=-1;
	}	
	#endif 
	
	
  //软件获取磁盘剩余容量簇大小
	dpbp->dpb_xnfreeclst= fat_free(dpbp->dpb_unit);

	dpbp->dpb_fsinfoflag |= FSINFOLOADED; 
	return 0;
}
#endif
