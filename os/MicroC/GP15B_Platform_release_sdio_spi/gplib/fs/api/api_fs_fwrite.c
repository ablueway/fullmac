#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
INT32S fs_fwrite(INT16S fd, INT32U bp, INT32U n)
{
#if _READ_WRITE_SPEED_UP == 1
    INT32S XferCount = fast_wrblock(fd, bp, n, XFR_WRITE|XFR_BYTEMODE);
#else
	INT32S XferCount = rwblock(fd, bp, n, XFR_WRITE|XFR_BYTEMODE);
#endif
    return XferCount;
}

#ifdef unSP
INT32S fs_fwriteB(INT16S fd,  INT32U bp, INT16U n)
{
    INT32S XferCount = rwblock(fd, bp, n, XFR_WRITE|XFR_WORDMODE);
    return XferCount;
}
#endif
#endif//READ_ONLY
