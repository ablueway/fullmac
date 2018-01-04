#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
INT8S *init_dir_startcluster(INT8S *Path, f_node_ptr fnp)
{
	if (Path[1] == ':') {
		fnp->f_dir.dir_attrib = D_DIR;
		dir_init_fnode(fnp, 0);
		return Path;
	}

	if (CDS[default_drive].cdsStrtClst == -1) {
		fnp->f_dir.dir_attrib = D_DIR;
		dir_init_fnode(fnp, 0);
		return Path;
	}

	dir_init_fnode(fnp, CDS[default_drive].cdsStrtClst);
	return Path;
}
#endif
