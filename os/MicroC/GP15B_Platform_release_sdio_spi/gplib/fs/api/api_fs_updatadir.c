/************************************************************************/
/* 	
	zhangzha add。
	在写数据的过程中有时需要把目录信息更新回文件系统中，比如长时间录音，
	如果中途掉电，则之前的录音数据有可能全部丢失，而调用此函数可确保之前的
	数据不会丢失。
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if _DIR_UPDATA == 1
INT16S fs_UpdataDir(INT16S fd)
{
	f_node_ptr fnp;
	
	/* Translate the fd into a useful pointer                       */
	fnp = xlt_fd(fd);
	/* If the fd was invalid because it was out of range or the     */
	/* requested file was not open, tell the caller and exit        */
	/* note: an invalid fd is indicated by a 0 return               */
	if (fnp == (f_node_ptr) 0)
		return DE_INVLDHNDL;
		
	if (!fnp->f_count) 
		return DE_INVLDHNDL;
		
#if WITHFAT32 == 1
	if (fnp->f_dpb->dpb_fsinfoflag & FSINFODIRTYFLAG)
	{
		write_fsinfo(fnp->f_dpb);
	}
#endif
	
	/* Write out the entry                                          */
	dir_lfn_updata(fnp, (tDirEntry*)&fnp->f_dir);	

	/* Clear buffers after release                                  */
	/* hazard: no error checking! */
	flush_buffers(fnp->f_dpb->dpb_unit);		//zhangzha marked for test
			
	return SUCCESS;
}


/**************************************************************************/
INT16S dir_lfn_updata(f_node_ptr fnp, tDirEntry *shortdir)
{
	struct buffer *bp;
	REG INT16U secsize = fnp->f_dpb->dpb_secsize;
	INT32S sector;
	
	/* Update the entry if it was modified by a write or create...  */
	if (fnp->f_flags & F_DMOD)
	{
		/* Root is a consecutive set of blocks, so handling is  */
		/* simple.                                              */
		//if (fnp->f_dir_address == 0)
		if (fnp->f_dirstart == 0)
		{
			if (fnp->f_diroff >= fnp->f_dpb->dpb_dirents)
				return DE_HNDLDSKFULL; 	

			bp = getblock(fnp->f_diroff / (secsize / DIRENT_SIZE)
                             + fnp->f_dpb->dpb_dirstrt,
                    fnp->f_dpb->dpb_unit);
		}
		else
		{
			sector = (INT8U)(fnp->f_dir_address / secsize) & fnp->f_dpb->dpb_clsmask;
			/* Root is a consecutive set of blocks, so handling is  */
			/* simple.                                              */
			bp = getblock(clus2phys(fnp->f_dir_cluster, fnp->f_dpb) + sector,
				fnp->f_dpb->dpb_unit);
		}
		
		/* Now that we have a block, transfer the directory      */
		/* entry into the block.                                */
		if (bp == NULL)
		{
			release_f_node(fnp);
			return DE_INVLDBUF; 
		}
      #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			INT8U *p;			
			p=&bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)];
			putdirent((struct dirent *)shortdir, p);
			memcpy((p+8),p+12,8);	
		}
		else
	  #endif
		{
			swap_deleted(fnp->f_dir.dir_name);

			putdirent((struct dirent *)shortdir, &bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)]);

			swap_deleted(fnp->f_dir.dir_name);
		}
		bp->b_flag &= ~(BFR_DATA | BFR_FAT);
		bp->b_flag |= BFR_DIR | BFR_DIRTY | BFR_VALID;
		
		//flush1(bp);			//060829 zhangzha add
	}
	return TRUE;
}

#endif // _DIR_UPDATA
#endif //#ifndef READ_ONLY
