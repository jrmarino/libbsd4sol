/*
 * Copyright © 2006 Robert Millan
 * Copyright © 2009, 2011 Guillem Jover <guillem@hadrons.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBBSD_ERR_H
#define LIBBSD_ERR_H

#include <sys/cdefs.h>
#include <stdlib.h>
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
