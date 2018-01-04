#include "fsystem.h"
#include "globals.h"

/*                                                                      */
/*                      Flush all buffers for a disk                    */
/*                                                                      */
/*      returns:                                                        */
/*              TRUE on success                                         */
/*                                                                      */
BOOLEAN flush_buffers(REG INT16S dsk)
{
  	struct buffer *bp = firstbuf;
  	REG BOOLEAN ok = TRUE;

  	bp = firstbuf;
  	do {
    	if (bp->b_unit == dsk) {
      		if (!flush1(bp)) {
        		ok = FALSE;
        	}
        }
    	bp = b_next(bp);
    	if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
  	} while (FP_OFF(bp) != FP_OFF(firstbuf));
  	
  	FileSysDrv[dsk]->Drv_Flush();
  	return ok;
}

BOOLEAN clear_buffers(REG INT16S dsk)
{
  	struct buffer *bp = firstbuf;
  	REG BOOLEAN ok = TRUE;

  	bp = firstbuf;
  	do {
    	if (bp->b_unit == dsk) {
    	 	if (!flush1(bp)) {
     	   		ok = FALSE;
     	   	}
    	 	bp->b_flag = 0; 				// clear each entry
    	}
    	bp = b_next(bp);
    	if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
	} while (FP_OFF(bp) != FP_OFF(firstbuf));
  	
  	FileSysDrv[dsk]->Drv_Flush();
  	return ok;
}

BOOLEAN invalid_buffers(REG INT16S dsk)
{
	struct buffer *bp = firstbuf;
	
	bp = firstbuf;
	do {
		if (bp->b_unit == dsk) {		
			bp->b_flag = 0; 				// clear each entry
		}
		bp = b_next(bp);
		if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
	} while (FP_OFF(bp) != FP_OFF(firstbuf));

	return TRUE;
}

BOOLEAN flush_fat_buffers(REG INT16S dsk)	// zurong add for fat buffer flush.2011.01.11
{
  	struct buffer *bp = firstbuf;
  	REG BOOLEAN ok = TRUE;

  	bp = firstbuf;
  	do {
    	if ((bp->b_unit == dsk)&&(bp->b_flag & (BFR_FAT|BFR_BITMAP))) {	//ÅÐ¶ÏÊÇ·ñÊÇFAT buffer
      		if (!flush1(bp)) {
        		ok = FALSE;
        	}
        }
    	bp = b_next(bp);
    	if ((INT32U)bp > 0x40000000 || ((INT32U)bp & 0x3)) {
    		return FALSE;
    	}
  	} while (FP_OFF(bp) != FP_OFF(firstbuf));
  	
  	//FileSysDrv[dsk]->Drv_Flush();
  	return ok;
}
