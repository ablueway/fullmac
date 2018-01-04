#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT16S _getdiskfree(INT16S driver, struct _diskfree_t *dfreep) 	 
{ 
	CLUSTER count;
	
	FS_OS_LOCK();

	fs_getdiskfree (&(dfreep ->total_clusters),
		&(dfreep ->sectors_per_cluster),
		&(dfreep ->bytes_per_sector), driver);

 	count = fat_free(driver);
	dfreep ->avail_clusters = count;

	FS_OS_UNLOCK();
	
	return 0;
}

//INT32S vfsFreeSpace(INT16S drive) 
INT64U vfsFreeSpace(INT16S drive) 
{
	 struct _diskfree_t dfreep;
	 //INT32S ret;
	 INT64U ret;
	
	 if ( drive >= 0) 
		 _getdiskfree(drive, &dfreep); 
	 else
		 _getdiskfree(default_drive, &dfreep); 
	 ret = (INT64U)dfreep.avail_clusters*(INT64U)dfreep.sectors_per_cluster*(INT64U)dfreep.bytes_per_sector;
	 return ret;
}

