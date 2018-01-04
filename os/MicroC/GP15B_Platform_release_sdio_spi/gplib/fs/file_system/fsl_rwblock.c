#include "fsystem.h"
#include "globals.h"

/*
  comments read optimization for large reads: read total clusters in one piece

  running a program like 
  
  while (1) {
    read(fd, header, sizeof(header));   // small read 
    read(fd, buffer, header.size);      // where size is large, up to 63K 
                                        // with average ~32K
    }                                        

    FreeDOS 2025 is really slow. 
    on a P200 with modern 30GB harddisk, doing above for a 14.5 MB file
    
    MSDOS 6.22 clustersize 8K  ~2.5 sec (accumulates over clusters, reads for 63 sectors seen),
    IBM PCDOS 7.0          8K  ~4.3 
    IBM PCDOS 7.0          16K ~2.8 
    FreeDOS ke2025             ~17.5

    with the read optimization (ke2025a),    
    
        clustersize 8K  ~6.5 sec
        clustersize 16K ~4.2 sec
        
    it was verified with IBM feature tool,
    that the drive read ahead cache (says it) is on. still this huge difference ;-)
        

    it's coded pretty conservative to avoid all special cases, 
    so it shouldn't break anything :-)

    possible further optimization:
    
        collect read across clusters (if file is not fragmented).
        MSDOS does this (as readcounts up to 63 sectors where seen)
        specially important for diskettes, where clustersize is 1 sector 
        
        the same should be done for writes as well

    the time to compile the complete kernel (on some P200) is 
    reduced from 67 to 56 seconds - in an otherwise identical configuration.

    it's not clear if this improvement shows up elsewhere, but it shouldn't harm either
    

    TE 10/18/01 14:00
    
    collect read across clusters (if file is not fragmented) done.
        
    seems still to work :-))
    
    no large performance gains visible, but should now work _much_
    better for the people, that complain about slow floppy access

    the 
        fnp->f_offset +to_xfer < fnp->f_dir.dir_size &&  avoid EOF problems 

    condition can probably _carefully_ be dropped    
    
    
    TE 10/18/01 19:00
    
*/

/* Read/write block from disk */
/* checking for valid access was already done by the functions in dosfns.c */

#ifdef unSP
INT32S rwblock(INT16S fd, INT32U lpBuf, INT16U count, INT16S mode) 	 
{
	REG f_node_ptr fnp;
	REG struct buffer *bp;
	INT16U xfr_cnt = 0;
	INT16U ret_cnt = 0;
	INT16U secsize;
	INT16U to_xfer;
	INT32U currentblock;
	INT16S ret;
	 
	to_xfer = count;
	fnp = xlt_fd(fd);
	if (fnp == (f_node_ptr) 0) {
		return DE_INVLDHNDL;
	}

	if (!fnp->f_dpb->dpb_mounted || !fnp->f_count) {
		return (INT32S) DE_INVLDHNDL;		
	}

	if (mode & XFR_WRITE) {
	#ifndef READ_ONLY
		if ((fnp->f_dir.dir_attrib & D_RDONLY) || ((fnp->f_mode & O_ACCMODE)==O_RDONLY)) {
			return DE_ACCESS; 	
		}
		fnp->f_dir.dir_attrib |= D_ARCHIVE;
		fnp->f_flags |= F_DMOD;       		// mark file as modified
		fnp->f_flags &= ~F_DDATE;     		// set date not valid any more
		
		if (dos_extend(fnp) != SUCCESS) {
			save_far_f_node(fnp);
			return DE_HNDLDSKFULL;
		}
	#else	
		return DE_ACCESS;
	#endif	
	}
	// If the count is zero and the mode is XFR_READ, just exit. (Any read with a count of zero is a nop).
	// A write (mode is XFR_WRITE) is a special case. It sets the file length to the current length (truncates it).
	// NOTE: doing this up front saves a lot of headaches later.
  #ifdef SHINKFILE
	if (count == 0) {
		// NOTE: doing this up front made a lot of headaches later :-(
		// FAT allocation has to be extended if necessary, now done in dos_extend
		// remove all the following allocated clusters in shrink_file
		if (mode & XFR_WRITE) {
		#ifndef READ_ONLY		
			fnp->f_dir.dir_size = fnp->f_offset;
			shrink_file(fnp);
		#else
			return DE_ACCESS;
		#endif	
		}
		save_far_f_node(fnp);
		return 0;
	}
  #endif
	// The variable secsize will be used later.
	secsize = fnp->f_dpb->dpb_secsize;

  	// Do the data transfer. Use block transfer methods so that we can utilize memory management in future DOS-C versions.
  	while (ret_cnt < count) {
    	INT16U sector, boff;

    	// Do an EOF test and return whatever was transferred but only for regular files.
    	if ((mode & XFR_READ) && !(fnp->f_flags & F_DDIR) && (fnp->f_offset >= fnp->f_dir.dir_size)) {
      		save_far_f_node(fnp);
			return ret_cnt;
    	}

	    /* Position the file to the fnode's pointer position. This is   */
	    /* done by updating the fnode's cluster, block (sector) and     */
	    /* byte offset so that read or write becomes a simple data move */
	    /* into or out of the block data buffer.                        */
	
	    /* The more difficult scenario is the (more common)     */
	    /* file offset case. Here, we need to take the fnode's  */
	    /* offset pointer (f_offset) and translate it into a    */
	    /* relative cluster position, cluster block (sector)    */
	    /* offset (sector) and byte offset (boff). Once we      */
	    /* have this information, we need to translate the      */
	    /* relative cluster position into an absolute cluster   */
	    /* position (f_cluster). This is unfortunate because it */
	    /* requires a linear search through the file's FAT      */
	    /* entries. It made sense when DOS was originally       */
	    /* designed as a simple floppy disk operating system    */
	    /* where the FAT was contained in core, but now         */
	    /* requires a search through the FAT blocks.            */
	    /*                                                      */
	    /* The algorithm in this function takes advantage of    */
	    /* the blockio block buffering scheme to simplify the   */
	    /* task.                                                */

		ret = map_cluster(fnp, mode & (XFR_READ|XFR_WRITE));
	
    	if (ret != SUCCESS) {
			gFS_errno = ret;
		 	save_far_f_node(fnp);

		 	return ret_cnt;
    	}

    	// Compute the block within the cluster and the offset within the block.
    	sector = (INT8U)(fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask;
    	boff = (INT16U)(fnp->f_offset & 0x1ff);
	 	currentblock = clus2phys(fnp->f_cluster, fnp->f_dpb) + sector;

		if (!(fnp->f_flags & F_DDIR) && boff == 0) { // don't experiment with directories yet, complete sectors only
      		INT32U startoffset;
      		INT16U sectors_to_xfer, sectors_wanted;

      		startoffset = fnp->f_offset;
      		
      		// avoid EOF problems
      		if ((mode & XFR_READ) && (to_xfer>(fnp->f_dir.dir_size-fnp->f_offset))) {
        		sectors_wanted = (INT16U) (fnp->f_dir.dir_size - fnp->f_offset);
        	} else {
        		sectors_wanted = to_xfer;
        	}
      		
      		sectors_wanted >>= 9;

//			if (sectors_wanted == 0) {
        		goto normal_xfer;
//        	}

      		sectors_to_xfer = fnp->f_dpb->dpb_clsmask + 1 - sector;
      		sectors_to_xfer = min(sectors_to_xfer, sectors_wanted);

      		fnp->f_offset += sectors_to_xfer << (9-WORD_PACK_VALUE);

      		while (sectors_to_xfer < sectors_wanted) {
        		if (map_cluster(fnp, mode & (XFR_READ|XFR_WRITE)) != SUCCESS) {
          			break;
          		}

        		if (clus2phys(fnp->f_cluster, fnp->f_dpb) != (currentblock + sectors_to_xfer)) {
          			break;
          		}

        		sectors_to_xfer += fnp->f_dpb->dpb_clsmask + 1;
        		sectors_to_xfer = min(sectors_to_xfer, sectors_wanted);

        		fnp->f_offset = startoffset + (sectors_to_xfer << (9-WORD_PACK_VALUE));
      		}

      		xfr_cnt = sectors_to_xfer << (9-WORD_PACK_VALUE);

      		// avoid caching trouble
      		DeleteBlockInBufferCache(currentblock, currentblock+sectors_to_xfer-1, fnp->f_dpb->dpb_unit, mode & (XFR_READ|XFR_WRITE));
            if((mode & XFR_READ) == 0)
            {
                SetWirteBufferType(BFR_DATA);    
      		}
      		if (dskxfer(fnp->f_dpb->dpb_unit, currentblock, (INT32U) lpBuf>>WORD_PACK_VALUE, sectors_to_xfer,
                  (mode & XFR_READ) ? DSKREAD : DSKWRITE))
      		{
        		fnp->f_offset = startoffset;
        		save_far_f_node(fnp);
				return DE_INVLDBUF;
			}
      		goto update_pointers;
    	} 

normal_xfer:
    	// normal read: just the old, buffer = sector based read

    	// Get the block we need from cache
    	bp = getblock(currentblock, fnp->f_dpb->dpb_unit);
    	if (bp == NULL) {
      		save_far_f_node(fnp);
      		return ret_cnt;
    	}

    	/* transfer a block                                     */
    	/* Transfer size as either a full block size, or the    */
    	/* requested transfer size, whichever is smaller.       */
    	/* Then compare to what is left, since we can transfer  */
    	/* a maximum of what is left.                           */
    	xfr_cnt = min(to_xfer, secsize - boff);
    	if (!(fnp->f_flags & F_DDIR) && (mode & XFR_READ)) {
      		xfr_cnt = (INT16U) min(xfr_cnt, fnp->f_dir.dir_size - fnp->f_offset);
      	}

    	/* transfer a block                                     */
    	/* Transfer size as either a full block size, or the    */
    	/* requested transfer size, whichever is smaller.       */
    	/* Then compare to what is left, since we can transfer  */
    	/* a maximum of what is left.                           */

		switch (mode)
	 	{
	 	case XFR_READ:
			fmemcpy(lpBuf>>WORD_PACK_VALUE, &bp->b_buffer[boff>>WORD_PACK_VALUE], xfr_cnt>>WORD_PACK_VALUE);
			break;
	 	case XFR_WRITE:
		#ifndef READ_ONLY	 
			fmemcpy(&bp->b_buffer[boff>>WORD_PACK_VALUE], lpBuf>>WORD_PACK_VALUE, xfr_cnt>>WORD_PACK_VALUE);
			break;
		#else
		 return DE_ACCESS;		 
		#endif		 
	 	case XFR_READ|XFR_BYTEMODE:
			MemPackByteCopyFar(((INT32U) bp->b_buffer<<WORD_PACK_VALUE)+boff, lpBuf, xfr_cnt);
			break;
		case XFR_WRITE|XFR_BYTEMODE:
	 	#ifndef READ_ONLY
			MemPackByteCopyFar(lpBuf, ((INT32U) bp->b_buffer<<WORD_PACK_VALUE)+boff, xfr_cnt);
			break;
		 #else
		 return DE_ACCESS;
		#endif
	 	case XFR_READ|XFR_WORDMODE:
			_MemPackWord2ByteCopyFar(((INT32U) bp->b_buffer<<WORD_PACK_VALUE)+boff, lpBuf, xfr_cnt);
			break;
	 	case XFR_WRITE|XFR_WORDMODE:
	 	#ifndef READ_ONLY
			_MemPackByte2WordCopyFar(lpBuf, ((INT32U) bp->b_buffer<<WORD_PACK_VALUE)+boff, xfr_cnt);
			break;
		#else
		return DE_ACCESS;
	 	#endif
	  #ifdef _555_FS_
	 	case XFR_READ|NOT_DMA_SRAM:
			hwDMAExec((INT32U) (&bp->b_buffer[boff]), (INT32U) lpBuf, DramToSram, xfr_cnt);
		 	break;
	 	case XFR_WRITE|NOT_DMA_SRAM:
			hwDMAExec((INT32U) (&bp->b_buffer[boff]), (INT32U) lpBuf, SramToDram, xfr_cnt);
			break;
	  #endif
	 	}
	
		if (mode & XFR_WRITE) {
	 #ifndef READ_ONLY
			bp->b_flag |= BFR_DIRTY | BFR_VALID;
	 #else
	 	return DE_ACCESS;
	 #endif
	 	}

    	// complete buffer transferred? Probably not reused later
    	if (xfr_cnt==(sizeof(bp->b_buffer)<<WORD_PACK_VALUE) ||
        	((mode & XFR_READ) && fnp->f_offset + xfr_cnt == fnp->f_dir.dir_size))
    	{
      		bp->b_flag |= BFR_UNCACHE;
    	}

    	// update pointers and counters
    	fnp->f_offset += xfr_cnt;

update_pointers:
    	ret_cnt += xfr_cnt;
    	to_xfer -= xfr_cnt;

		lpBuf = ((INT32U) lpBuf + xfr_cnt);

    	if (mode & XFR_WRITE) {
    #ifndef READ_ONLY
      		if (fnp->f_offset > fnp->f_dir.dir_size) {
        		fnp->f_dir.dir_size = fnp->f_offset;
      		}
      		merge_file_changes(fnp, FALSE);
    #else
    	return DE_ACCESS;  
	#endif     
    	}
  	}
  	save_far_f_node(fnp);

	return ret_cnt;
}
#endif