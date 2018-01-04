#include "fsystem.h"
#include "globals.h"

INT32S Clus2Phy( INT16U dsk,  INT32U cl_no)
{
	struct cds *cdsp;
	struct dpb *dpbp;
	CLUSTER cdata;
	
	cdsp = get_cds(dsk);
	
	if (cdsp==NULL)
	{
		return -1;
	}
	
	dpbp = cdsp->cdsDpb;
	
	if (!dpbp ->dpb_mounted)
		return -1;			 // device not ready
	
	cdata =
#if WITHFAT32 == 1||WITHEXFAT == 1
		(ISFAT32(dpbp)||ISEXFAT(dpbp)) ? dpbp->dpb_xdata :
#endif
		dpbp->dpb_data;
	
	return ((INT32U) (cl_no - 2) << dpbp->dpb_shftcnt) + cdata;
}
