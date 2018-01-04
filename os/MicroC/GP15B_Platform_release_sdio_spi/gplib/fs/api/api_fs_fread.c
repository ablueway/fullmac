#include "fsystem.h"
#include "globals.h"

INT32S fs_fread(INT16S fd, INT32U bp, INT32U n)  
{    
  #if _READ_WRITE_SPEED_UP == 1
	return fast_rdblock(fd, bp, n, XFR_READ|XFR_BYTEMODE); 	
  #else
	return rwblock(fd, bp, n, XFR_READ|XFR_BYTEMODE); 	
  #endif
}

#ifdef unSP
INT32S fs_freadB(INT16S fd, INT32U bp, INT16U n)
{
    return rwblock(fd, bp, n, XFR_READ|XFR_WORDMODE);
}
#endif
