#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/* Description.
 *  Read next consequitive directory entry, pointed by fnp.
 *  If some error occures the other critical
 *  fields aren't changed, except those used for caching.
 *  The fnp->f_diroff always corresponds to the directory entry
 *  which has been read.
 * Return value.
 *  1              - all OK, directory entry having been read is not empty.
 *  0              - Directory entry is empty.
 *  DE_SEEK        - Attempt to read beyound the end of the directory.
 *  DE_BLKINVLD    - Invalid block.
 * Note. Empty directory entries always resides at the end of the directory. */
INT16S dir_read(REG f_node_ptr fnp)
{
	struct buffer *bp;
	REG INT16U secsize = fnp->f_dpb->dpb_secsize;
	
	// can't have more than 65535 directory entries
	if (fnp->f_diroff >= 65535U) {
		return DE_SEEK;
	}

	/* Determine if we hit the end of the directory. If we have,    */
	/* bump the offset back to the end and exit. If not, fill the   */
	/* dirent portion of the fnode, clear the f_dmod bit and leave, */
	/* but only for root directories                                */	
	if (fnp->f_dirstart == 0) {
		if (fnp->f_diroff >= fnp->f_dpb->dpb_dirents) {
			return DE_SEEK;
		}
		bp = getblock(fnp->f_diroff / (secsize / DIRENT_SIZE) + fnp->f_dpb->dpb_dirstrt, fnp->f_dpb->dpb_unit);
	} else {
		// Do a "seek" to the directory position
		fnp->f_offset = ((INT32U) fnp->f_diroff) << 5; 	  

		// Search through the FAT to find the block that this entry is in.
		if (map_cluster(fnp, XFR_READ) != SUCCESS) {
			return DE_SEEK;
		}
		
		bp = getblock_from_off(fnp, secsize);
	}

	// Now that we have the block for our entry, get the directory entry.
	if (bp == NULL) {
		return DE_BLKINVLD;
	}
	
	bp->b_flag &= ~(BFR_DATA | BFR_FAT);
	bp->b_flag |= BFR_DIR | BFR_VALID;

	getdirent((INT8S *) &bp->b_buffer[(((INT32U) fnp->f_diroff) << (5-WORD_PACK_VALUE)) & (0x1ff>>WORD_PACK_VALUE)], &fnp->f_dir); 

  #if WITHEXFAT == 1
	if(!ISEXFAT(fnp->f_dpb))
  #endif		
  	swap_deleted(fnp->f_dir.dir_name);
 	
	// Update the fnode's directory info
	fnp->f_flags &= ~F_DMOD;
	
	// and for efficiency, stop when we hit the first unused entry. either returns 1 or 0
	return (fnp->f_dir.dir_name[0] != '\0');
}
#endif	//#ifndef READ_ONLY
