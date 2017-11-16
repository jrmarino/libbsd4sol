#ifndef LIBBSD_ERR_H
#define LIBBSD_ERR_H

#include <sys/cdefs.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#define err(exitcode, format, args...) \
	errx(exitcode, format ": %s", ## args, strerror(errno))
#define errx(exitcode, format, args...) \
	{ warnx(format, ## args); exit(exitcode); }
#define warn(format, args...) \
	warnx(format ": %s", ## args, strerror(errno))
#define warnx(format, args...) \
	fprintf(stderr, format "\n", ## args)

__BEGIN_DECLS
void warnc(int code, const char *format, ...)
	__printflike(2, 3);
void vwarn(const char *format, va_list ap)
	__printflike(1, 0);
void vwarnc(int code, const char *format, va_list ap)
	__printflike(2, 0);
void errc(int status, int code, const char *format, ...)
	__printflike(3, 4);
void verr(int status, const char *format, va_list ap)
	__printflike(2, 0);
void verrc(int status, int code, const char *format, va_list ap)
	__printflike(3, 0);
__END_DECLS

#endif
