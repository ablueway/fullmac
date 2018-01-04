/**************************************************************************
 * FreeDOS 32 Unicode support library                                     *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2002, Salvatore Isaja                               *
 *                                                                        *
 * This file is part of FreeDOS 32                                        *
 *                                                                        *
 * FreeDOS 32 is free software; you can redistribute it and/or modify it  *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * FreeDOS 32 is distributed in the hope that it will be useful, but      *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with FreeDOS 32; see the file GPL.txt; if not, write to          *
 * the Free Software Foundation, Inc.                                     *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/***************************************************************************/
/* Converts a UTF-16 string into UTF-8.          */
/* Returns 0 on success, FD32_EUTF16 on failure. */
INT16S fd32_utf16to8(const UTF16 *Utf16, UTF8 *Utf8)
{
  UTF32 Ch;
  INT16S   Res;

  for (;;)
  {
    /* If the UTF-16 character fits in a single-byte UTF-8 character, */
    /* process it directly.                                           */
    if (( INT16U)(*Utf16) < 0x0080)
    {
		if ((*(Utf8++) = (UTF8) *(Utf16++)) == 0x00)
		{
			return 0;			
		} 
    }
    else
    {
		if ((Res = fd32_utf16to32(Utf16, &Ch)) < 0)
		{
			return FD32_EUTF16;			
		}

		Ch &= 0x0000ffff;
		Utf16 += Res;

		Utf8  += fd32_utf32to8(Ch, Utf8);
    }
  }
}

/************************************************************************/
/*	���������Ƶ� �� UTF16 ת��Ϊ UTF8 �ĺ���
	
	ԭ��������'\0'���������������������������Կ��ܲ�������'\0'

    ����ʹ�ô˺���ǰ����Ҫ��Utf8 ���������0 �Ķ���

  */
/************************************************************************/
INT16S fd32_utf16to8_limit(const UTF16 *Utf16, UTF8 *Utf8, INT16S space_num)
{
	UTF32	Ch;
	INT16S	i, Res;

	for ( ; space_num > 5; )
	{
		/* If the UTF-16 character fits in a single-byte UTF-8 character, */
		/* process it directly.                                           */
		if (( INT16U)(*Utf16) < 0x0080)
		{
			if ((*(Utf8++) = (UTF8) *(Utf16++)) == 0x00)
			{
				return 0;			
			} 

			// �ַ���ռ��һ���ֽڵĿռ�	
			space_num --;
		}
		else
		{
			if ((Res = fd32_utf16to32(Utf16, &Ch)) < 0)
			{
				return FD32_EUTF16;			
			}
			
			Ch &= 0x0000ffff;
			Utf16 += Res;
			
			//Utf8  += fd32_utf32to8(Ch, Utf8);
			i = fd32_utf32to8(Ch, Utf8);		// ����ʹ�õ��ַ��ĸ���	

			space_num -= i;		// �ռ��С
			Utf8 += i;			// ����ƶ�����ָ��
		}
	}

	// ��ǿ���˳��ģ�ĩβ���в�NULL �Ķ���
	*Utf8 = '\0';
	
	return 0;
}

/***************************************************************************/
#endif // LFN_FLAG

