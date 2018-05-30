#include_next <sys/time.h>

#ifndef LIBBSD_TIME_H
#define LIBBSD_TIME_H

#ifndef timersub
#define timersub(a, b, result) \
    do { \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((result)->tv_usec < 0) { \
            (result)->tv_sec--; \
            (result)->tv_usec += 1000000; \
        } \
    } while (0)
#endif /* ifndef timersub */

#ifndef timeradd
#define timeradd(a, b, result)						\
	do {								\
		(result)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
		(result)->tv_usec = (a)->tv_usec + (b)->tv_usec;	\
		if ((result)->tv_usec >= 1000000) {			\
			(result)->tv_sec++;				\
			(result)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#endif /* ifndef timeradd */

time_t timegm(struct tm * const);

#endif /* !LIBBSD_TIME_H */
