#include_next <string.h>

#ifndef LIBBSD_STRING_H
#define	LIBBSD_STRING_H

#include <sys/types.h>

char	*strsep(char **, const char *);
char	*strndup(const char *str, size_t n);
char	*strcasestr(const char *, const char *);
void	strmode(mode_t mode, char *str);
size_t	strnlen(const char *, size_t);

void	explicit_bzero(void *buf, size_t len);
#endif
