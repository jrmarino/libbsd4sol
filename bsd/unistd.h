#include_next <unistd.h>

#ifndef LIBBSD_UNISTD_H
#define	LIBBSD_UNISTD_H

#ifndef S_ISTXT
#define S_ISTXT S_ISVTX
#endif

#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)

#include <sys/cdefs.h>

mode_t getmode(const void *set, mode_t mode);
void *setmode(const char *mode_str);
void setproctitle(const char *fmt, ...) __printflike(1, 2);

#endif
