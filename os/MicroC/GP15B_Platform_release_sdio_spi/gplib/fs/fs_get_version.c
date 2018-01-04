#include "fsystem.h"
#include "globals.h"

static const char *filesystem_ver="GP$0200";		// v0.2.00

const char *fs_get_version(void)
{
	return 	filesystem_ver;
}

