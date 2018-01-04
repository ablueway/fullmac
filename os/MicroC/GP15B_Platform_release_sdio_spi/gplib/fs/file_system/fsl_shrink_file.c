/* TE
    if the current filesize in FAT is larger than the dir_size, it's truncated here.
    the BUG was:
    copy COMMAND.COM xxx
    echo >xxx
    
    then, the dirsize of xxx was set to ~20, but the allocated FAT entries not returned.
    this code corrects this
    
    Unfortunately, this code _nearly_ works, but fails one of the 
    Apps tested (VB ISAM); BO: confirmation???
*/

#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
void shrink_file(f_node_ptr fnp)
{
  	INT32U lastoffset = fnp->f_offset;     	// has to be saved
  	CLUSTER next, st;
  	struct dpb *dpbp = fnp->f_dpb;

  	fnp->f_offset = fnp->f_dir.dir_size;	// end of file
  	if (fnp->f_offset) {
    	fnp->f_offset--;            		// last existing cluster
    }

  	if (map_cluster(fnp, XFR_READ) != SUCCESS) {   // error, don't truncate
    	goto done;
    }

  	st = fnp->f_cluster;
	next = next_cluster(dpbp, st);
	if (next == 1) { 						// error
    	goto done;
    }

  	// Loop from start until either a FREE entry is encountered (due to a damaged file system)
  	// or the last cluster is encountered.
  	if (fnp->f_dir.dir_size == 0) {
    	fnp->f_cluster = FREE;
    	setdstart(dpbp, &fnp->f_dir, FREE);
      #if WITHEXFAT == 1
      	if(ISEXFAT(fnp->f_dpb))	
      	{
    		link_fat(dpbp, st, FREE,fnp->ext_dir->SecondaryFlags);
      	}
      	else
      	{
       		link_fat(dpbp, st, FREE,NULL);
      	}
      #else
        link_fat(dpbp, st, FREE,NULL);
      #endif	
  	} else {
    	if (next == LONG_LAST_CLUSTER) {	// nothing to do
      		goto done;
      	}
      #if WITHEXFAT == 1	
        if(ISEXFAT(fnp->f_dpb))	
		{
    		link_fat(dpbp, st, LONG_LAST_CLUSTER,fnp->ext_dir->SecondaryFlags);
      	}
      	else
      	{
      		link_fat(dpbp, st, LONG_LAST_CLUSTER,NULL);
      	}
      #else
        link_fat(dpbp, st, LONG_LAST_CLUSTER,NULL);
      #endif	
  	}

 	wipe_out_clusters(dpbp, next);

done:
  	fnp->f_offset = lastoffset;   			// has to be restored
}
#endif	//#ifndef READ_ONLY
