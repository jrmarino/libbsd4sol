#include_next <stdio.h>

#ifndef LIBBSD_STDIO_H
#define LIBBSD_STDIO_H

#include <stdarg.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

int	dprintf(int, const char *, ...);
ssize_t	getdelim(char **, size_t *, int, FILE *);
ssize_t	getline(char **, size_t *, FILE *);
char	*fgetln(FILE *, size_t *);

#define asprintf	sol_asprintf
#define vasprintf	sol_vasprintf

int	 asprintf(char **, const char *, ...);
int	 vasprintf(char **, const char *, va_list);

__END_DECLS

#endif
