#include "fsystem.h"
#include "globals.h"

#if WITHFAT32 == 1
void write_fsinfo(struct dpb *dpbp)
{
	struct buffer *bp;
	struct fsinfo *fip;

  #if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
	{
		return;
	}
  #endif	
	if (!(dpbp->dpb_fsinfoflag & FSINFODIRTYFLAG)) {
		return;
	}
	
	bp = getblock(dpbp->dpb_xfsinfosec, dpbp->dpb_unit);
	if(bp==NULL)	//zurong 2011.5.12
	{
		return;
	}
	bp->b_flag &= ~(BFR_DATA | BFR_DIR | BFR_FAT);
	bp->b_flag |= BFR_VALID | BFR_DIRTY;
	fip = (struct fsinfo *) &bp->b_buffer[0x1e4>>WORD_PACK_VALUE];
	putlong(&fip->fi_nfreeclst, dpbp->dpb_xnfreeclst);
	putlong(&fip->fi_cluster, dpbp->dpb_xcluster);
	
	dpbp->dpb_fsinfoflag = 0;
}
#endif

