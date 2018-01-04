#include "fsystem.h"
#include "globals.h"

struct buffer * getblock_from_off(f_node_ptr fnp, INT16U secsize)
{
  	// Compute the block within the cluster and the offset within the block.
  	INT16U sector;

  	sector = (INT8U) (fnp->f_offset / secsize) & fnp->f_dpb->dpb_clsmask;

  	// Get the block we need from cache
  	return getblock(clus2phys(fnp->f_cluster, fnp->f_dpb) + sector, fnp->f_dpb->dpb_unit);
}
