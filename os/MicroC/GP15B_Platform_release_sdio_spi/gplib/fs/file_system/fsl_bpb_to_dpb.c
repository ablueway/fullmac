#include "fsystem.h"
#include "globals.h"

#if WITHFAT32 == 1
INT16S bpb_to_dpb(bpb *bpbp, REG struct dpb *dpbp, BOOLEAN extended)
#else
INT16S bpb_to_dpb(bpb *bpbp, REG struct dpb *dpbp)
#endif
{
  	INT32U size;
  	INT16U shftcnt;
  	INT32U total_sec;
  	INT16U fatsiz;
    INT32U tmp;

  	for (shftcnt=0; (bpbp->bpb_nsector>>shftcnt)>1; shftcnt++) ;
  	dpbp->dpb_shftcnt = shftcnt;

  	dpbp->dpb_secsize = bpbp->bpb_nbyte;
  	dpbp->dpb_clsmask = bpbp->bpb_nsector - 1;
  	dpbp->dpb_dirents = bpbp->bpb_ndirent;
  	
	dpbp->dpb_fatstrt = bpbp->bpb_nreserved + bpbp->bpb_hidden;
  	dpbp->dpb_fats = bpbp->bpb_nfat;
  	dpbp->dpb_fatsize = bpbp->bpb_nfsect;
  	dpbp->dpb_dirstrt = dpbp->dpb_fatstrt + dpbp->dpb_fats * dpbp->dpb_fatsize;		// Sector number of root directory
  	

  	tmp = dpbp->dpb_secsize/DIRENT_SIZE;	// Number of directory entries in a sector
  	dpbp->dpb_data = dpbp->dpb_dirstrt + (dpbp->dpb_dirents+tmp-1)/tmp;		// Sector number of data sector after root directory

	size = bpbp->bpb_nsize == 0 ? bpbp->bpb_huge : (INT32U) bpbp->bpb_nsize;
	total_sec = size - dpbp->dpb_data + bpbp->bpb_hidden;	// Total sector numbers of data region
	dpbp->dpb_size = total_sec >> shftcnt;	// Total cluster numbers of data region

	// Make sure the number of FAT sectors is actually enough to hold that many clusters.
	// Otherwise back the number of clusters down
    tmp = dpbp->dpb_fatsize * (INT32U)(dpbp->dpb_secsize / 2);		// entries/2
    if (tmp < 0x10000UL) {
	    fatsiz = (INT16U) tmp;
	    if (dpbp->dpb_size > FAT_MAGIC) {	// FAT16
	      	if (fatsiz > FAT_MAGIC) {     		
			    if (dpbp->dpb_size >= fatsiz) {
					dpbp->dpb_size = fatsiz - 2;
				}
	        }
	    } else {                         	// FAT12
	      	if (fatsiz < 0x4000) {
			    fatsiz = fatsiz * 4 / 3;
			    if (dpbp->dpb_size >= fatsiz) {
					dpbp->dpb_size = fatsiz - 2;
				}
	        }
	    }
	}

  	dpbp->dpb_cluster = UNKNCLUSTER;
  	dpbp->dpb_nfreeclst = UNKNCLSTFREE;		// number of free clusters

  #if WITHFAT32 == 1 || WITHEXFAT == 1
  	if (extended) {
    	dpbp->dpb_xflags = 0;
    	dpbp->dpb_xfsinfosec = 0xffff;
    	dpbp->dpb_xbackupsec = 0xffff;
    	dpbp->dpb_xdata = dpbp->dpb_data;
    	dpbp->dpb_xsize = dpbp->dpb_size;
    	dpbp->dpb_xfatsize = bpbp->bpb_nfsect==0 ? bpbp->bpb_xnfsect : bpbp->bpb_nfsect;
		dpbp->dpb_xrootclst = 0;
    	dpbp->dpb_xcluster = UNKNCLUSTER;
    	dpbp->dpb_xnfreeclst = XUNKNCLSTFREE;       // number of free clusters
    	dpbp->dpb_fsinfoflag = 0;
		
		#if WITHEXFAT == 1  
		dpbp->dpb_exsize=bpbp->bpb_huge==0 ? bpbp->bpb_exfsize : 0;
		#endif
		
    	if (ISFAT32(dpbp)) {
			dpbp->dpb_dirents = 0;
			dpbp->dpb_dirstrt = 0xfffffffful;
			dpbp->dpb_size = 0;
			dpbp->dpb_xflags = bpbp->bpb_xflags;
			dpbp->dpb_xfsinfosec = bpbp->bpb_xfsinfosec + bpbp->bpb_hidden;
			dpbp->dpb_xbackupsec = bpbp->bpb_xbackupsec;
			dpbp->dpb_xdata = dpbp->dpb_fatstrt + dpbp->dpb_fats * dpbp->dpb_xfatsize;

			total_sec = size - dpbp->dpb_xdata + bpbp->bpb_hidden;	// Total sector numbers of data region
			dpbp->dpb_xsize = total_sec >> shftcnt;		// Total cluster numbers of data region

			dpbp->dpb_xrootclst = bpbp->bpb_xrootclst;
    	}
		#if WITHEXFAT == 1  
		else if(ISEXFAT(dpbp))
		{

			dpbp->dpb_fatstrt = bpbp->bpb_exFAT1Adr + bpbp->bpb_hidden;
			dpbp->dpb_fats = bpbp->bpb_nfat;
			dpbp->dpb_xfatsize = bpbp->bpb_exFATLong;
			dpbp->dpb_xdata = bpbp->bpb_exmapbit+bpbp->bpb_hidden;
			dpbp->dpb_xrootclst = bpbp->bpb_xrootclst;

			total_sec = bpbp->bpb_exfsize;// Total sector numbers of data region
			dpbp->dpb_xsize = bpbp->bpb_exclucount;//total_sec >> shftcnt; 	// Total cluster numbers of data region
			
			dpbp->dpb_size = 0;
			dpbp->dpb_xflags = bpbp->bpb_xflags;
			dpbp->dpb_xfsinfosec = -1;
			dpbp->dpb_xbackupsec = -1;
			
		
			dpbp->dpb_dirstrt = ((INT32U) (bpbp->bpb_xrootclst - 2) << (dpbp->dpb_shftcnt)) + dpbp->dpb_xdata;

			//get maptable ,up case


			
			dpbp->dpb_exmtable=bpbp->bpb_exmtable;
			dpbp->dpb_exmlength=bpbp->bpb_exmlength;
			
			dpbp->dpb_exuptable=bpbp->bpb_exuptable;
			dpbp->dpb_exuplength=bpbp->bpb_exuplength;


			
			
		}
		#endif
		#if 1 // 2011.05.13 
			if (read_fsinfo(dpbp)) {
				return DE_INVLDFUNC;
			}
		#endif
  	}
  #endif
	
	
	




	return 0;
}
