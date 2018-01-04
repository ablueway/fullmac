#include "fsystem.h"
#include "globals.h"

struct dpb * get_dpb(INT16S dsk)
{
  	register struct cds *cdsp = get_cds(dsk);
  
  	if (cdsp == NULL) {
    	return NULL;
    }

  	return cdsp->cdsDpb;
}
