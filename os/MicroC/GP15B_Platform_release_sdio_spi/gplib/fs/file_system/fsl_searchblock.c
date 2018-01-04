#include "fsystem.h"
#include "globals.h"

struct buffer * searchblock(INT32U blkno, INT16S dsk)
{
	INT16S fat_count = 0;
	struct buffer *bp;
	INT32U lastNonFat = 0;
	INT32U uncacheBuf = 0;  
	INT32U firstbp = FP_OFF(firstbuf);
	
	// Search through buffers to see if the required block is already in a buffer
	bp = bufptr(firstbp);
	do {
		if ((bp->b_blkno==blkno) && (bp->b_flag&BFR_VALID) && (bp->b_unit==dsk)) {
			// found it -- rearrange LRU links
			bp->b_flag &= ~BFR_UNCACHE;  	// reset uncache attribute
			
			if (FP_OFF(bp) != firstbp) {
				firstbuf = bp;
				move_buffer(bp, firstbp);
			}
			return bp;
		}

		if (bp->b_flag & BFR_UNCACHE) {
			uncacheBuf = FP_OFF(bp);
		}
		if (bp->b_flag & (BFR_FAT|BFR_BITMAP)) {
			fat_count++;
		} else {
			lastNonFat = FP_OFF(bp);
		}
		
		bp = b_next(bp);
		if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
	} while (FP_OFF(bp) != firstbp) ;

	// now take either the last buffer in chain (not used recently) or, if we are low on FAT buffers, the last non FAT buffer
	if (uncacheBuf) {
		bp = bufptr(uncacheBuf);
	} else if (bp->b_flag&(BFR_FAT|BFR_BITMAP) && fat_count<3 && lastNonFat) {
		bp = bufptr(lastNonFat);
	} else {
		bp = b_prev(bufptr(firstbp));
	}
	
	bp->b_flag |= BFR_UNCACHE;  			// set uncache attribute

  #ifdef DISPLAY_GETBLOCK
	printf("MISS, replace %04x:%04x]\n", FP_SEG(bp), FP_OFF(bp));
  #endif

	if (FP_OFF(bp) != firstbp) {         // move to front
		move_buffer(bp, firstbp);
		firstbuf = bp;
	}  
  
	return bp;
}