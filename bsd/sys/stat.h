#include_next <sys/stat.h>

#ifndef LIBBSD_STAT_H
#define	LIBBSD_STAT_H

int	fchmodat(int, const char *, mode_t, int);

#endif
