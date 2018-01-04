#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
// initialize all direntry fields except for the name
void init_direntry(struct dirent *dentry, INT16U attrib, CLUSTER cluster)
{
    dostime_t dt;
	
  	dentry->dir_size = 0;
  #if WITHFAT32 == 1
  	dentry->dir_start_high = (INT16U) (cluster >> 16);
  #else
  	dentry->dir_start_high = 0;
  #endif
  	dentry->dir_start = (INT16U) cluster;
  	dentry->dir_attrib = attrib;
  	dentry->dir_case = 0;
  	
  	FS_OS_GetTime(&dt);
  	dentry->dir_crtimems = dt.hundredth;
  	if (dt.second & 1) {
    	dentry->dir_crtimems += 100;
    }
  	dentry->dir_time = dentry->dir_crtime = time_encode(&dt);
  	dentry->dir_date = dentry->dir_crdate = dentry->dir_accdate = dos_getdate();
}


#if WITHEXFAT == 1
//初始化主目录  生成时间缀
void init_ExFatdirentry(struct dirent *dentry, INT16U attrib, CLUSTER cluster)
{
	ExFATMainDIR *MainDiR=(ExFATMainDIR *)dentry;

	MainDiR->EntryType=0x85;
	MainDiR->FileAttributes=attrib;
	
    //时间
	MainDiR->CreateTimestamp=MainDiR->LastAccessedTimestamp=MainDiR->LastModifiedTimestamp=dostimeget_ExFat();
	//10ms增量
	MainDiR->CreatemsIncrement=MainDiR->LastAccmsIncrement=MainDiR->LastModmsIncrement=dosms_Exfat();
}

void init_exfatdir(f_node_ptr fnp, INT16U attrib, CLUSTER cluster)
{	
	init_ExFatdirentry((struct dirent *)&fnp->extmain_dir, attrib, cluster);	
	fnp->ext_dir->DataLength=0;
	fnp->ext_dir->ValidDataLength=0;
	fnp->ext_dir->FirstCluster=cluster;
	fnp->ext_dir->SecondaryFlags=0x01;
	
	fnp->f_flags = F_DMOD | F_DDIR|F_ExMDir;

}
#endif

#endif
