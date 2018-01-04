#include "os.h"

void os_init(void)
{
  #if _OPERATING_SYSTEM == _OS_UCOS2
	OSInit();								// Initiate uC/OS-2
	OSTimeSet(0);

  #elif _OPERATING_SYSTEM == _OS_NUCLEUS

  #endif	
}