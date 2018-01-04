#include "fsystem.h"
#include "globals.h"

INT32S fs_GetCluster(INT16S fd)
{
	 INT32S ret;
	 REG f_node_ptr fnp;
	
	 fnp = xlt_fd(fd);

	 if (fnp == (f_node_ptr) 0) 	 
	 {
		 ret = DE_INVLDHNDL;
		 return ret;
	 }

	 if (!fnp ->f_dpb ->dpb_mounted)
	 {
		 ret = DE_INVLDHNDL;	
		 return ret;
	 }

	 ret = map_cluster(fnp, XFR_READ);
	
	 if (ret==SUCCESS)
	 {
		 return fnp->f_cluster;
	 }
	 else
	 {
		 return ret;
	 }
}
