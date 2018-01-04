#include "fsystem.h"
#include "globals.h"

/* allocate a near fnode and copy the far fd fnode to it */
f_node_ptr xlt_fd(INT16S fd)
{
  	f_node_ptr fnp = (f_node_ptr) 0;

  	dbg_info1("xlt_fd: fd = %d\n", fd);
  	if (( INT16U)fd < MAX_FILE_NUM) {
    	fnp = &f_nodes_array[fd];
    	dbg_info1("xlt_fd: fnp = %p\n", fnp);
  	}
  	
  	return fnp;
}
