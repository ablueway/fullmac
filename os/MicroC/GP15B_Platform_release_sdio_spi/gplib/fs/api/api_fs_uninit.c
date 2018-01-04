#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
INT16S fs_uninit(void)
{
	 INT16S i;
	 INT16S ret;
	
	 for (i = 0; i < MAX_DISK_NUM; i++)
	 {
		 ret = fs_unmount(i);
		 if (ret!=SUCCESS)
		 {
			 return -1;
		 }
	 }
	
	 return 0;
}
#endif
