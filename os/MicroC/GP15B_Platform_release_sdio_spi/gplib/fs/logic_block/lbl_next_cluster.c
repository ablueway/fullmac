#include "fsystem.h"
#include "globals.h"

#define READ_CLUSTER 1		// Tristan: READ_CLUSTER is defined in fsl_link_fat.c

/* Given the disk parameters, and a cluster number, this function
   looks at the FAT, and returns the next cluster in the clain. */

CLUSTER next_cluster(struct dpb *dpbp, CLUSTER ClusterNum)
{
  	// Tristan: link_fat belongs to file system layer, should not be called by logic layer
  	return link_fat(dpbp, ClusterNum, READ_CLUSTER,NULL);
}
