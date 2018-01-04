#include "fsystem.h"
#include "globals.h"

/* translate the f_node pointer into an fd                  */
INT16S xlt_fnp(f_node_ptr fnp)
{
	return fnp - f_nodes_array;
}
