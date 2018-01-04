/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "open.c" - Open, create and close a file (or even a directory  *
 *                    as a file) in any directory, allocating and freeing *
 *                    handles                                             *
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
/* Descends a path, opening each component and returning a pointer to  */
/* a file structure for the last path component in the Fp pointer.     */
/* This file structure can be used to manage the parent directory of   */
/* a file to open.                                                     */
/* If the same path has been already opened before, a file structure   */
/* used for it may be present, with the path name cached. If that is   */
/* the case, that file structure is returned without descending again. */
/* Returns 0 on success, or a negative error code on failure.          */
/**************************************************************************/
f_node_ptr descend_path(CHAR *Path, tFatFind *D)
{
	INT16S		Res;
	CHAR		*Component;
	CHAR		*pComponent;
	f_node_ptr	fnp;

	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
		gFS_errno = DE_TOOMANY; 
		return (f_node_ptr) 0;
	}

	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;

	/* determine what drive and dpb we are using...                 */
	if ((*Path == '\0') || (Path[1] != ':')) 
	{
		fnp->f_dpb = get_dpb(default_drive);
		if (Path[0] == '\\')
			fnp->f_dirstart = 0;
	 	else 	
      		fnp->f_dirstart = CDS[default_drive].cdsStrtClst;
	 	fnp->f_dir.dir_attrib = D_DIR; //by jk 2005/02/21 
	}
	else
	{
		if ((Path[0]-'A'>= MAX_DISK_NUM) || (Path[0]-'A'<0))     //disk id error
		{
			gFS_errno = DE_INVLDDRV;    //20051027 modify by zhangyan
			return (f_node_ptr) 0;
		}
		fnp->f_dpb = get_dpb(Path[0]-'A'); 
		fnp->f_dirstart = 0;    //wanghuidi
		fnp->f_dir.dir_attrib = D_DIR; //by jk 2005/02/21 
	}

	/* Perform all directory common handling after all special      */
	/* handling has been performed.                                 */	
	if (media_check(fnp->f_dpb) < 0)
	{
		gFS_errno = DE_INVLDDRV;    //20050223
		release_f_node(fnp);
		return (f_node_ptr) 0;
	}

	dir_init_fnode(fnp, fnp->f_dirstart);   //wanghuidi,20050217

	if ((*Path != '\0') && (*(Path+1) == ':'))  /* by jk 2005/02/16 */
		Path += 2;

	while (*Path != '\0')
	{
		/* skip all path seperators                             */
		while (*Path == '\\')
		{
			++Path;		
		}

	    /* don't continue if we're at the end */
	    if (*Path == '\0'){
			break;		
		}
	
		pComponent = strchr(Path, '\\');
		Component = Path;	
		if (pComponent != NULL)
		{
			*pComponent = 0;
		}
		Path += strlen(Component);
	
	    /* Search for the file named Component in the directory */
	    Res = fat_find(fnp, Component, FD32_FRNONE | FD32_FAALL, D);
	    
	    //restore the '\\'
		if (pComponent != NULL)
			*pComponent = '\\';			
		
	    if (Res < 0)
	    {
			if (!strcmp (Component, ".") || !strcmp (Component, "..")) 	
			{
				dir_init_fnode(fnp, fnp ->f_dirstart);  //release by wanghuidi,to fix bug
				continue; 	 // "." and ".." can be used under root directory
			}
			
			if (Res == FD32_ENMFILE) Res = FD32_ENOTDIR;
			release_f_node(fnp);
			gFS_errno = DE_PATHNOTFND;
			return (f_node_ptr) 0;
		}
			
		dir_init_fnode(fnp, getdstart(fnp->f_dpb, &fnp->f_dir));
	}
  
	return fnp;
}

#endif // LFN_FLAG
