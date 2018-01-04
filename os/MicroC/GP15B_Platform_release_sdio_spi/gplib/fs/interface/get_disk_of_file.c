#include "fsystem.h"
#include "globals.h"

INT16S GetDiskOfFile(INT16S fd)
{
	REG f_node_ptr fnp;
	
	fnp = xlt_fd(fd);
	if ( (fnp == (f_node_ptr)0))
	{
		return 	 DE_INVLDHNDL;
	}
	
	if (!fnp ->f_dpb ->dpb_mounted || !fnp->f_count){						
		return (INT32S) DE_INVLDHNDL;		
	}
	
	return fnp->f_dpb->dpb_unit;
}
