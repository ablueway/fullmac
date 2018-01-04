#include "fsystem.h"
#include "globals.h"
#include "drv_l1_cache.h"
#include "driver_l1_cfg.h"

#define __SWITCH_BANK

#ifdef unSP
void MemPackByteCopyFarEE(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarOO(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarOE(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarEO(INT32U s, INT32U d, INT16U n);

void MemPackByteCopyFarEEO(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarOOO(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarOEO(INT32U s, INT32U d, INT16U n);
void MemPackByteCopyFarEOO(INT32U s, INT32U d, INT16U n);

void MemPackWord2ByteCopyFarE(INT32U s, INT32U d, INT16U n);
void MemPackWord2ByteCopyFarO(INT32U s, INT32U d, INT16U n);

void MemPackByte2WordCopyFarE(INT32U s, INT32U d, INT16U n);
void MemPackByte2WordCopyFarO(INT32U s, INT32U d, INT16U n);

#ifdef __SWITCH_BANK
void MemPackBackupBankAddr(void);
void MemPackRestoreBankAddr(void);
unsigned int MemPackReadWord(unsigned long addr);
void MemPackWriteWord(unsigned long addr, unsigned int data);
unsigned int MemPackReadByte(unsigned long addr);
void MemPackWriteByte(unsigned long addr, unsigned int data);
#endif

void MemPackByteCopyFar(INT32U s, INT32U d, INT16U n)
{
	INT32U sour, dest;
	INT16U sel;
	INT16U cnt;
	
	sour = s >> 1;
	dest = d >> 1;
	
	cnt = (n+1) >> 1;
	
  #ifdef __SWITCH_BANK
	if ((sour<0x400000) && (dest<0x400000)) {
  #endif	
		sel = (((INT16U)s&0x1)<<1) | ((INT16U)d&0x1) | (((INT16U)n&0x1)<<2);
	
		switch (sel) {
	 	case 0x0:
		 	MemPackByteCopyFarEE((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x1:
		 	MemPackByteCopyFarEO((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x2:
		 	MemPackByteCopyFarOE((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x3:
		 	MemPackByteCopyFarOO((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x4:
		 	MemPackByteCopyFarEEO((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x5:
			MemPackByteCopyFarEOO((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x6:
			MemPackByteCopyFarOEO((INT32U)sour, dest, cnt);
		 	break;
	 	case 0x7:
		 	MemPackByteCopyFarOOO((INT32U)sour, dest, cnt);
		 	break;
	 	}
  #ifdef __SWITCH_BANK	 
	} else {
		INT16U i ,data;
		
		MemPackBackupBankAddr();
		
		if ((s&1) || (d&1)) {
			for (i=0; i<n; i++) {
				data = MemPackReadByte(s++);
				MemPackWriteByte(d++, data);
			}
		} else {
			cnt = n >> 1;
			for (i=0; i<cnt; i++) {
				data = MemPackReadWord(sour++);
				MemPackWriteWord(dest++, data);
			}
			if (n & 1) {
				data = MemPackReadByte(s + n - 1);
				MemPackWriteByte(d + n - 1, data);
			}
		}
		
		MemPackRestoreBankAddr();
	}
  #endif
	 return;
}


/*
 func:	
 in:	
		s 		以字节 为单位的地址空间值
		dest 	以字 为单位的地址空间
		cnt 	字节的数量
 */
void _MemPackWord2ByteCopyFar(INT32U s, INT32U dest, INT16U cnt)
{
	 INT32U 	 sour;
	
	 sour = s >> 1;
	
	 if (s&0x1)
	 {
		 MemPackWord2ByteCopyFarO(sour, dest, cnt);
	 }
	 else
	 {
		 MemPackWord2ByteCopyFarE(sour, dest, cnt);
	 }

	 return;
}

/*
	func:
	
	in:	
		s 		以 WORD 为单位的地址空间值
		dest 	以 BYTE 为单位的地址空间
		cnt 	字节的数量
	out:
 */

void _MemPackByte2WordCopyFar(INT32U sour, INT32U  d, INT16U cnt)
{
	 INT32U 	 dest;
	
	 dest = d >> 1;
	
	 if (d&0x1)
	 {
		 MemPackByte2WordCopyFarO(sour, dest, cnt);
	 }
	 else
	 {
		 MemPackByte2WordCopyFarE(sour, dest, cnt);
	 }

	 return;
}

#else


void MemPackByteCopyFar(INT32U s, INT32U d, INT16U n)
{
	INT16S  i;
	INT8S	*ps, *pd;

	ps = (INT8S *) s;
	pd = (INT8S *) d;

	for (i=0x00; i<n; i++) {
		pd[i] = ps[i];
	}
#if _DRV_L1_CACHE 	
	cache_drain_range((INT32U) d, (INT32U) n);
#endif	
}

/*
		s 		以字节为单位的地址空间值
		dest 	以字为单位的地址空间
		cnt 	字节的数量
*/
void _MemPackWord2ByteCopyFar(INT32U s, INT32U dest, INT16U cnt)
{
	INT32U s_add, d_add;
	
	s_add = s/2;
	d_add = dest;

	memcpy((INT8U *) d_add, (INT8U *) s_add, cnt);
#if _DRV_L1_CACHE	
	cache_drain_range((INT32U) d_add, (INT32U) cnt);
#endif	
}

/*
		s 		以 WORD 为单位的地址空间值
		dest 	以 BYTE 为单位的地址空间
		cnt 	字节的数量
*/
void _MemPackByte2WordCopyFar(INT32U sour, INT32U d, INT16U cnt)
{
	INT32U s_add, d_add;
	
	s_add = sour;
	d_add = d/2;
	
	memcpy((INT8U *) d_add, (INT8U *) s_add, cnt);
#if _DRV_L1_CACHE	
	cache_drain_range((INT32U) d_add, (INT32U) cnt);
#endif	
}

#endif