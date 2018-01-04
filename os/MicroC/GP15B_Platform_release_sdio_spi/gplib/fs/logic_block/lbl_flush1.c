#include "fsystem.h"
#include "globals.h"

/*                                                                      */
/*      Write one disk buffer                                           */
/*                                                                      */
BOOLEAN flush1(struct buffer *bp)
{
	INT16U result;
	
	if(bp==NULL)
	{
		return FALSE;
	}
	
	if ((bp->b_flag & (BFR_VALID | BFR_DIRTY)) == (BFR_VALID | BFR_DIRTY)) {
	#ifndef READ_ONLY
        SetWirteBufferType(bp->b_flag & 0x0E);	
		result = dskxfer(bp->b_unit, bp->b_blkno, (INT32U) bp->b_buffer, 1, DSKWRITE);
		
		if (bp->b_flag & BFR_FAT) {
			INT16U b_copies = bp->b_copies; 	  
			INT32U blkno = bp->b_blkno;
		  #if WITHFAT32 == 1
			INT32U b_offset = bp->b_offset;
			
			if (b_offset == 0) 				// FAT32 FS
				b_offset = bp->b_dpbp->dpb_xfatsize;
		  #else
			INT16U b_offset = bp->b_offset;
		  #endif
			while (--b_copies > 0) {
				blkno += b_offset;
				result = dskxfer(bp->b_unit, blkno, (INT32U)bp->b_buffer, 1, DSKWRITE);
			}
		}
		#if WITHEXFAT == 1
		else if(bp->b_flag & BFR_BITMAP)
		{
			INT16U b_copies = bp->b_copies; 	  
			INT32U blkno = bp->b_blkno;
			INT32U b_offset = ((bp->b_dpbp->dpb_exmlength)+(bp->b_dpbp->dpb_clsmask))/(bp->b_dpbp->dpb_clsmask+1);
			b_offset=b_offset*(bp->b_dpbp->dpb_clsmask+1);		
			while (--b_copies > 0) {
				blkno += b_offset;
				result = dskxfer(bp->b_unit, blkno, (INT32U)bp->b_buffer, 1, DSKWRITE);
			}
		}
		#endif
	#else
			return DE_ACCESS;	
	#endif
	}
	else
		result = 0;                 /* This negates any error code returned in result...BER */
	bp->b_flag &= ~BFR_DIRTY;     			// Even if error, mark not dirty, otherwise system has trouble
	if (result != 0) {
		bp->b_flag &= ~BFR_VALID;
	}
	
	return (TRUE);                			// Forced to TRUE...was like this before dskxfer()
}
