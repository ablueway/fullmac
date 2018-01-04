/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "creat.c" - File creation and deletion services                *
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
/* Copies the 16-bit character Ch into the LFN slot Slot in the position */
/* SlotPos, taking care of the slot structure.                           */
/* Called by split_lfn.                                                  */
/**************************************************************************/
void copy_char_in_lfn_slot(tLfnEntry *Slot, INT16S SlotPos, UTF16 Ch)
{
	if ((SlotPos >= 0)  && (SlotPos <= 4))  
		Slot->Name0_4  [SlotPos]    = Ch;
	else if ((SlotPos >= 5)  && (SlotPos <= 10)) 
		Slot->Name5_10 [SlotPos-5]  = Ch;
	else if ((SlotPos >= 11) && (SlotPos <= 12)) 
		Slot->Name11_12[SlotPos-11] = Ch;
}

/**************************************************************************/
#endif // LFN_FLAG

