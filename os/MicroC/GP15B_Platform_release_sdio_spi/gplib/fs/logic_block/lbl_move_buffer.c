#include "fsystem.h"
#include "globals.h"

/*
    this searches the buffer list for the given disk/block.
    
    returns:
    a far pointer to the buffer.

    If the buffer is found the UNCACHE bit is not set, else it is set.
        
    new:
        upper layer may set UNCACHE attribute
        UNCACHE buffers are recycled first.
        intended to be used for full sector reads into application buffer
        resets UNCACHE upon a "HIT" -- so then this buffer will not be
        recycled anymore.
*/

void move_buffer(struct buffer *bp, INT32U firstbp)
{
	/* connect bp->b_prev and bp->b_next */
	b_next(bp)->b_prev = bp->b_prev;
	b_prev(bp)->b_next = bp->b_next;
	
	/* insert bp between firstbp and firstbp->b_prev */
	bp->b_prev = bufptr(firstbp)->b_prev;
	bp->b_next = bufptr(firstbp); 
	
	b_next(bp)->b_prev = bp;
	b_prev(bp)->b_next = bp;
}
