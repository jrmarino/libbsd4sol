/*
 * Implementation of replacement timegm function
 *
 * Copyright (c) 2018, John R. Marino
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <time.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>

/*
 * The Gregorian reform modified the Julian calendar's scheme of leap years
 * as follows:
 *    Every year that is exactly divisible by four is a leap year, except
 *    for years that are exactly divisible by 100, but these centurial
 *    years are leap years if they are exactly divisible by 400. For
 *    example, the years 1700, 1800, and 1900 were not leap years, but the
 *    years 1600 and 2000 were.
 * ref: http://aa.usno.navy.mil/faq/docs/calendars.php
 *
 * Output: 1 if year is a leapyear, 0 if year is not a leap year
 */
static int
is_leapyear (int year_since_1900)
{
   int year = year_since_1900 + 1900;

   if (year % 4 == 0) {
      if (year % 100 == 0) {
         return (year % 400 == 0);
      }
      return (1);
   }
   return (0);
}

static unsigned
seconds_per_year (int year_since_1900)
{
   return (86400 * (is_leapyear(year_since_1900) ? 366 : 365));
}

/*
 * This implementation is equivalent to FreeBSD's.
 * The timegm() function interprets the input structure as representing
 * Universal Coordinated Time (UTC).
 *
 * The original values of the tm_wday and tm_yday components of the
 * structure are ignored, and the original values of the other components
 * are not restricted to their normal ranges, and will be normalized if
 * needed.  For example, October 40 is changed into November 9, a tm_hour of
 * -1 means 1 hour before midnight, tm_mday of 0 means the day preceding the
 * current month, and tm_mon of -2 means 2 months before January of tm_year.
 * The tm_isdst and tm_gmtoff members are forced to zero by timegm().)
 */

#ifndef WRONG
#define WRONG	(-1)
#endif

time_t
timegm(struct tm *const tmp)
{
	const unsigned days_past[2][12] = {
	  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	  { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
        };
	long long int	result = 0;
	int 		leapyear = is_leapyear(tmp->tm_year);
	int 		year;

	if (tmp == NULL) {
		errno = EINVAL;
		return WRONG;
	}
	tmp->tm_isdst = 0;
	tmp->tm_gmtoff = 0;

	result += tmp->tm_sec;
	result += tmp->tm_min * 60;
	result += tmp->tm_hour * 3600;
	result += days_past[leapyear][tmp->tm_mon] * 86400;
	result += (tmp->tm_mday - 1) * 86400;
	for (year = tmp->tm_year; year < 70; year++)
		result -= seconds_per_year(year);
	for (year = 70; year < tmp->tm_year; year++)
		result += seconds_per_year(year);

	if (sizeof(result) > sizeof(time_t)) {
		if (result > INT_MAX || result < INT_MIN) {
			errno = EOVERFLOW;
			return WRONG;
		}
	}
	return ((time_t) result);
}
