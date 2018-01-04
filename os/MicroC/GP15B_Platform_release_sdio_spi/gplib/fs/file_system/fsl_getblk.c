#include "fsystem.h"
#include "globals.h"

/*                                                                      */
/*      Return the address of a buffer structure containing the         */
/*      requested block.                                                */
/*      if overwrite is set, then no need to read first                 */
/*                                                                      */
/*      returns:                                                        */
/*              requested block with data                               */
/*      failure:                                                        */
/*              returns NULL                                            */
/*                                                                      */
struct buffer * getblk(INT32U blkno, INT16S dsk, BOOLEAN overwrite)
{
	// Search through buffers to see if the required block is already in a buffer
	struct buffer *bp;
	
	bp = searchblock(blkno, dsk);
	if(bp==NULL)
	{
		return bp;
	}
	if (!(bp->b_flag & BFR_UNCACHE)) {
		return bp;
	}

	// The block we need is not in a buffer, flush bp and then read new block.
	if (!flush1(bp)) {
		return NULL;
	}
	
	// Fill the indicated disk buffer with the current track and sector
	if (!overwrite && dskxfer(dsk, blkno, (INT32U) bp->b_buffer, 1, DSKREAD)) {
		return NULL;
	}
	
	bp->b_flag = BFR_VALID | BFR_DATA;
	bp->b_unit = dsk;
	bp->b_blkno = blkno;
	
	return bp;
}
