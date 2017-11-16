#include_next <stdio.h>

#ifndef LIBBSD_STDIO_H
#define LIBBSD_STDIO_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

int	dprintf(int, const char *, ...);
ssize_t	getdelim(char **, size_t *, int, FILE *);
ssize_t	getline(char **, size_t *, FILE *);
char	*fgetln(FILE *, size_t *);

__END_DECLS

#endif
