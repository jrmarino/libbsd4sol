/*
 * Copyright © 2010 William Ahern
 * Copyright © 2012-2013 Guillem Jover <guillem@hadrons.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <string.h>

static struct {
	/* Original value. */
	const char *arg0;

	/* Title space available. */
	char *base, *end;

	 /* Pointer to original nul character within base. */
	char *nul;

	bool warned;
	bool reset;
	int error;
} SPT;


static inline size_t
spt_min(size_t a, size_t b)
{
	return a < b ? a : b;
}

#ifndef SPT_MAXTITLE
#define SPT_MAXTITLE 255
#endif

void
setproctitle(const char *fmt, ...)
{
	/* Use buffer in case argv[0] is passed. */
	char buf[SPT_MAXTITLE + 1];
	va_list ap;
	char *nul;
	int len;

	if (SPT.base == NULL) {
		if (!SPT.warned) {
			warnx("setproctitle not initialized, please either call "
			      "setproctitle_init() or link against libbsd-ctor.");
			SPT.warned = true;
		}
		return;
	}

	if (fmt) {
		if (fmt[0] == '-') {
			/* Skip program name prefix. */
			fmt++;
			len = 0;
		} else {
			/* Print program name heading for grep. */
			snprintf(buf, sizeof(buf), "%s: ", getprogname());
			len = strlen(buf);
		}

		va_start(ap, fmt);
		len += vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
		va_end(ap);
	} else {
		len = snprintf(buf, sizeof(buf), "%s", SPT.arg0);
	}

	if (len <= 0) {
		SPT.error = errno;
		return;
	}

	if (!SPT.reset) {
		memset(SPT.base, 0, SPT.end - SPT.base);
		SPT.reset = true;
	} else {
		memset(SPT.base, 0, spt_min(sizeof(buf), SPT.end - SPT.base));
	}

	len = spt_min(len, spt_min(sizeof(buf), SPT.end - SPT.base) - 1);
	memcpy(SPT.base, buf, len);
	nul = &SPT.base[len];

	if (nul < SPT.nul) {
		*SPT.nul = '.';
	} else if (nul == SPT.nul && &nul[1] < SPT.end) {
		*SPT.nul = ' ';
		*++nul = '\0';
	}
}
