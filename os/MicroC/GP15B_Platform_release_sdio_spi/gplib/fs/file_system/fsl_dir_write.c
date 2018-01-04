#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
BOOLEAN dir_write(REG f_node_ptr fnp)
{
#if WITHEXFAT == 1
	if(fnp->f_flags & F_ExMDir)
	{
		fnp->f_diroff--;
		dir_lfnwrite(fnp, (tDirEntry*) &fnp->extmain_dir);
		fnp->f_flags=F_DDIR|F_DMOD;
		fnp->f_diroff++;
	}
#endif
	return dir_lfnwrite(fnp, (tDirEntry*) &fnp->f_dir);	// Tristan: dir_lfnwrite belongs to LFN layer, should not be called in file system layer
}
#endif
