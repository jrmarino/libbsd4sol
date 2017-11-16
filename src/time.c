#include <time.h>
#include <sys/time.h>

int
leapyear (int year)
{
    return ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0));
}

/*
 * This is a simple implementation of timegm() which does what is needed
 * by create_output() -- just turns the "struct tm" into a GMT time_t.
 * It does not normalize any of the fields of the "struct tm", nor does
 * it set tm_wday or tm_yday.
 */
time_t
timegm (struct tm *tm)
{
    int monthlen[2][12] = {
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    };
    int year, month, days;

    days = 365 * (tm->tm_year - 70);
    for (year = 70; year < tm->tm_year; year++) {
        if (leapyear(1900 + year)) {
            days++;
        }
    }
    for (month = 0; month < tm->tm_mon; month++) {
        days += monthlen[leapyear(1900 + year)][month];
    }
    days += tm->tm_mday - 1;

    return ((((days * 24) + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec);
}
