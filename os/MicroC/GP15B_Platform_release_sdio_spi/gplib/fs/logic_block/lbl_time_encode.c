#include "fsystem.h"
#include "globals.h"

/* FAT time notation in the form of hhhh hmmm mmmd dddd (d = double second) */
dostime time_encode(dostime_t *t)
{
  	return (t->hour << 11) | (t->minute << 5) | (t->second >> 1);
}
