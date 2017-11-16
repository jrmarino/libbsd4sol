#include_next <stdlib.h>

#ifndef LIBBSD_STDLIB_H
#define	LIBBSD_STDLIB_H

#include <sys/types.h>

void *reallocf(void *ptr, size_t size);
void *reallocarray(void *ptr, size_t nmemb, size_t size);
const char *getprogname();

uint32_t arc4random(void);
void arc4random_stir(void);
void arc4random_addrandom(unsigned char *dat, int datlen);
void arc4random_buf(void *_buf, size_t n);
uint32_t arc4random_uniform(uint32_t upper_bound);

long long strtonum(const char *nptr, long long minval, long long maxval,
                   const char **errstr);

#endif
