#include "fsystem.h"
#include "globals.h"

void fs_getdiskfree ( INT32U * total_clusters,
	  INT32U * sector_per_cluster,
	  INT32U * byte_per_sector, INT16S dsk) 	 { /* INT16S ; by jk 2005/02/16 */

	 *total_clusters = (_ISFAT32(CDS[dsk].cdsDpb))||(_ISEXFAT(CDS[dsk].cdsDpb)) ? CDS[dsk].cdsDpb ->dpb_xsize : CDS[dsk].cdsDpb ->dpb_size;
	 *sector_per_cluster = CDS[dsk].cdsDpb->dpb_clsmask+1;
	 *byte_per_sector = CDS[dsk].cdsDpb->dpb_secsize;
}



