#include "fsystem.h"
#include "globals.h"

BOOLEAN ChangeFatWriteEn(BOOLEAN enable)
{
    BOOLEAN ret;
    
    FS_OS_LOCK();
    
    ret = gFAT2_WriteEn;
    gFAT2_WriteEn = enable;
    
    FS_OS_UNLOCK();
    
    return ret;
}