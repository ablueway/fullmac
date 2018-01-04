#include "fsystem.h"
#include "globals.h"



struct buffer * getMapblock(struct dpb *dpbp, CLUSTER clussec)
{
	struct buffer *bp;
	
	bp = getblock(clussec, dpbp->dpb_unit);
	
	if (bp) {
		bp->b_flag &= ~(BFR_DATA | BFR_DIR);
		bp->b_flag |= BFR_BITMAP | BFR_VALID;
		bp->b_dpbp = dpbp;
		bp->b_copies = dpbp->dpb_fats;
		bp->b_offset = 0;
#if 0
        bp->b_copies = 1;
#else
	  #if WITHFAT32 == 1 
		if (ISFAT32(dpbp)) {
			if (dpbp->dpb_xflags & FAT_NO_MIRRORING) {
				bp->b_copies = 1;
			}
		}	
		else if(ISEXFAT(dpbp)) {
			if (dpbp->dpb_xflags & EX_FAT_NO_MIRRORING) {
				bp->b_copies = 1;
			}	
		}
	  #endif
#endif      
	}

	return bp;
}

BOOLEAN gFAT2_WriteEn = 0;      //FAT2 write(or not)




struct buffer * getFATblock(struct dpb *dpbp, CLUSTER clussec)
{
	struct buffer *bp;
	
	bp = getblock(clussec, dpbp->dpb_unit);
	
	if (bp) {
		bp->b_flag &= ~(BFR_DATA | BFR_DIR);
		bp->b_flag |= BFR_FAT | BFR_VALID;
		bp->b_dpbp = dpbp;
		bp->b_copies = dpbp->dpb_fats;
		bp->b_offset = dpbp->dpb_fatsize;
		if(gFAT2_WriteEn == 0)
        {
            bp->b_copies = 1;
        }
        else  
        {
		  #if WITHFAT32 == 1 
			if (ISFAT32(dpbp)) {
				if (dpbp->dpb_xflags & FAT_NO_MIRRORING) {
					bp->b_copies = 1;
				}
			}
		  #if WITHEXFAT == 1		
			else if(ISEXFAT(dpbp)) {
				if (dpbp->dpb_xflags & EX_FAT_NO_MIRRORING) {
					bp->b_copies = 1;
				}	
			}
		  #endif	
	    }
#endif      
	}

	return bp;
}