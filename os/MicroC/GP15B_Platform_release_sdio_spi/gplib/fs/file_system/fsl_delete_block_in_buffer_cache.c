#include "fsystem.h"
#include "globals.h"

BOOLEAN DeleteBlockInBufferCache(INT32U blknolow, INT32U blknohigh, INT16S dsk, INT16S mode)
{
	struct buffer *bp = firstbuf;
	
	// Search through buffers to see if the required block is already in a buffer
	do {
		if ((bp->b_flag&BFR_VALID) && (blknolow<=bp->b_blkno) && (bp->b_blkno<=blknohigh) && (bp->b_unit==dsk)) {
			if (mode == XFR_READ) {
				flush1(bp);
			} else {
				bp->b_flag = 0;
			}
		}
		bp = b_next(bp);
		if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
	} while (FP_OFF(bp) != FP_OFF(firstbuf));
	
	return FALSE;
}