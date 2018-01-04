#include "fsystem.h"
#include "globals.h"

INT32U clus2phys(CLUSTER cl_no, struct dpb *dpbp)
{
	CLUSTER cdata =
  #if WITHFAT32 == 1 || WITHEXFAT == 1
	(ISFAT32(dpbp)||ISEXFAT(dpbp)) ? dpbp->dpb_xdata :
  #endif
	dpbp->dpb_data;
	
	return ((INT32U) (cl_no - 2) << dpbp->dpb_shftcnt) + cdata;
}

CLUSTER phys2clus(INT32U sec_no, struct dpb *dpbp)
{
	CLUSTER cdata =
  #if WITHFAT32 == 1
	ISFAT32(dpbp) ? dpbp->dpb_xdata :
  #endif
	dpbp->dpb_data;
	
	return ((sec_no - cdata) >> dpbp->dpb_shftcnt) + 2;
}