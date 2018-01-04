#include "fsystem.h"
#include "globals.h"

/* Try to allocate an f_node from the available files array */
f_node_ptr get_f_node(void)
{
	INT8U i;

	for (i=0; i<MAX_FILE_NUM; i++) {
		if (f_nodes_array[i].f_count == 0) {
			++f_nodes_array[i].f_count;
			return &f_nodes_array[i];
		}
	}

	return (f_node_ptr) 0;
}

INT32U get_used_f_node_number(void)
{
	INT32U i, count;

	count = 0;
	for (i=0; i<MAX_FILE_NUM; i++) {
		if (f_nodes_array[i].f_count) {
			count++;
		}
	}

	return count;
}