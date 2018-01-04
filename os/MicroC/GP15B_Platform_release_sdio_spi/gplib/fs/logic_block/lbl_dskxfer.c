#include "fsystem.h"
#include "globals.h"

INT16U dskxfer(INT16S dsk, INT32U blkno, INT32U buf, INT16U numblocks, INT16S mode)
{
	INT16S	Flag = 1;
	INT8U	i=0;
	
	if ((FileSysDrv[dsk]==NULL) || (FileSysDrv[dsk]->Status==0)) {
		return Flag;
	}
		
	if (mode == DSKREAD) { 				    
		for (i=0;i<FSDRV_MAX_RETRY;i++)  {
			Flag = FileSysDrv[dsk]->Drv_ReadSector(blkno, numblocks, buf);
			if (Flag==0) {
				break;
			}
		}
	} else if (mode == DSKWRITE) {
	#ifndef READ_ONLY
		for (i=0; i<FSDRV_MAX_RETRY; i++) {
			Flag = FileSysDrv[dsk]->Drv_WriteSector(blkno, numblocks, buf);			
			if (Flag==0) {
				break;
			}
		}
	#else
		return DE_ACCESS;		
	#endif	
	}
	
	else Flag = 1;
	return Flag;
}
