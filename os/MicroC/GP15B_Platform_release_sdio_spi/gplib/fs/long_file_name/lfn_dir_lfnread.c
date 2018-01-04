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

#if LFN_FLAG == 1
/**************************************************************************/
INT16S dir_lfnread(f_node_ptr fnp, tDirEntry *shortdir)
{
	struct buffer *bp;
	REG INT16U secsize = fnp->f_dpb->dpb_secsize;
	
	/* can't have more than 65535 directory entries */
	if (fnp->f_diroff >= 65535U)
		return DE_HNDLDSKFULL;

	/* Determine if we hit the end of the directory. If we have,    */
	/* bump the offset back to the end and exit. If not, fill the   */
	/* dirent portion of the fnode, clear the f_dmod bit and leave, */
	/* but only for root directories                                */
	if (fnp->f_dirstart == 0)
	{
		if (fnp->f_diroff >= fnp->f_dpb->dpb_dirents)
			return DE_HNDLDSKFULL; 

		bp = getblock(fnp->f_diroff / (secsize / DIRENT_SIZE)
			+ fnp->f_dpb->dpb_dirstrt, fnp->f_dpb->dpb_unit);
	#ifdef DISPLAY_GETBLOCK
		printf("DIR (dir_read)\n");
	#endif
	}
	else
	{
		/* Do a "seek" to the directory position        */
		fnp->f_offset = fnp->f_diroff * (INT32U)DIRENT_SIZE;
		
		/* Search through the FAT to find the block     */
		/* that this entry is in.                       */
	#ifdef DISPLAY_GETBLOCK
		printf("dir_read: ");
	#endif
		if (map_cluster(fnp, XFR_READ) != SUCCESS)
			return DE_HNDLDSKFULL; 	

		bp = getblock_from_off(fnp, secsize);
	#ifdef DISPLAY_GETBLOCK
		printf("DIR (dir_read)\n");
	#endif
	}

	/* Now that we have the block for our entry, get the    */
	/* directory entry.                                     */
	if (bp == NULL)
		return DE_BLKINVLD;
	
	bp->b_flag &= ~(BFR_DATA | BFR_FAT);
	bp->b_flag |= BFR_DIR | BFR_VALID;
	
	getdirent((INT8U *) & bp->
		b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)],
		(struct dirent *)shortdir);
	if(!ISEXFAT(fnp->f_dpb))
	{
		swap_deleted(fnp->f_dir.dir_name);
	}
	/* Update the fnode's directory info                    */
	fnp->f_flags &= ~F_DMOD;

	/* and for efficiency, stop when we hit the first       */
	/* unused entry.                                        */
	/* either returns 1 or 0           */
	

	if (shortdir -> Name[0] == '\0')
		return 0;
	else
		return DIRENT_SIZE;
	
}

/**************************************************************************/
#endif // LFN_FLAG

