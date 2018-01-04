/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "readwrit.c" - Read or write a block of data from/to a file    *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
#if LFN_FLAG == 1
/**************************************************************************/
INT16S dir_lfnwrite(f_node_ptr fnp, tDirEntry *shortdir)
{
	struct buffer *bp;
	REG INT16U secsize = fnp->f_dpb->dpb_secsize;
	BOOLEAN bExtended = FALSE; 		 //add by zy 05-01-06
	CLUSTER cluster; 	 //add by zy 05-01-06
	INT16S idx;
	INT32S sector;
	
	if (!(fnp->f_flags & F_DDIR))
	{
		return DE_INVLDACC; 		 //add by zhangyan 05-02-18
	}

	/* Update the entry if it was modified by a write or create...  */
	if (fnp->f_flags & F_DMOD)
	{
		/* Root is a consecutive set of blocks, so handling is  */
		/* simple.                                              */
		if (fnp->f_dirstart == 0)
		{
			if (fnp->f_diroff >= fnp->f_dpb->dpb_dirents)
				return DE_HNDLDSKFULL;
			bp = getblock(fnp->f_diroff / (secsize / DIRENT_SIZE)
				+ fnp->f_dpb->dpb_dirstrt,
				fnp->f_dpb->dpb_unit);
		}
		/* All other directories are just files. The only       */
		/* special handling is resetting the offset so that we  */
		/* can continually update the same directory entry.     */
		else
		{
			/* Do a "seek" to the directory position        */
			/* and convert the fnode to a directory fnode.  */
			fnp->f_offset = fnp->f_diroff * (INT32U)DIRENT_SIZE;
			fnp->f_cluster = fnp->f_dirstart;
			fnp->f_cluster_offset = 0;
			
			/* Search through the FAT to find the block     */
			/* that this entry is in.                       */
			if (map_cluster(fnp, XFR_READ) != SUCCESS)
			{
				if (fnp->f_dirstart == 0 && !_ISFAT32(fnp->f_dpb)&&!_ISEXFAT(fnp->f_dpb))
				{ 	 // can not extend root directory for fat16 or fat12
					return DE_ACCESS;
				}
	  	
				cluster = extend(fnp);
				
				if (cluster == LONG_LAST_CLUSTER)
				{
					release_f_node(fnp);
					return DE_HNDLDSKFULL; 
				}
	
				fnp->f_cluster = cluster;
				fnp->f_cluster_offset++;
				
				bExtended = TRUE;
				
				/* clear out the blocks in the cluster      */
				sector = clus2phys(fnp->f_cluster, fnp->f_dpb);
				
				for (idx = 1; idx <= fnp->f_dpb->dpb_clsmask; idx++)
				{
					/* as we are overwriting it completely, don't read first */
					bp = getblockOver(sector + idx, fnp->f_dpb->dpb_unit);
					
					if (bp == NULL)
					{
						dir_close(fnp);
						return DE_BLKINVLD;
					}
					
					fmemset(bp->b_buffer, 0, BUFFERSIZE);
					bp->b_flag |= BFR_DIRTY | BFR_VALID | BFR_UNCACHE; /* need not be cached */
				}
			}
			bp = getblock_from_off(fnp, secsize);
			if (bp == NULL)
		    {
				release_f_node(fnp);
				return DE_INVLDBUF; 		 //add by zhangyan 05-02-18
			}
			if (bExtended == TRUE)
			{
				fmemset(bp->b_buffer, 0, BUFFERSIZE);
			}
			bp->b_flag &= ~(BFR_DATA | BFR_FAT);
			bp->b_flag |= BFR_DIR | BFR_VALID;
		}

	    /* Now that we have a block, transfer the directory      */
	    /* entry into the block.                                */
	    if (bp == NULL)
	    {
			release_f_node(fnp);
			return DE_INVLDBUF; 		 //add by zhangyan 05-02-18
		}

	#if WITHEXFAT == 1

		if(ISEXFAT(fnp->f_dpb)&&(shortdir->Name[0]==0x85))
		{
			putdirent((struct dirent *)shortdir, &bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)]);
		}		
		else if(ISEXFAT(fnp->f_dpb)&&(shortdir->Name[0]==0xC0))
		{
			INT8U *p;			
			p=&bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)];
			putdirent((struct dirent *)shortdir, p);
			memcpy((p+8),p+12,8);		
		}
		else
	#endif
		{
			if(!ISEXFAT(fnp->f_dpb))
				swap_deleted(fnp->f_dir.dir_name);
			
			putdirent((struct dirent *)shortdir, &bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)]);

			if(!ISEXFAT(fnp->f_dpb))
				swap_deleted(fnp->f_dir.dir_name);
		}	
		bp->b_flag &= ~(BFR_DATA | BFR_FAT);
		bp->b_flag |= BFR_DIR | BFR_DIRTY | BFR_VALID;
		
	}
  
  	return TRUE;
}

/**************************************************************************/
#endif // LFN_FLAG
#endif // #ifndef READ_ONLY

