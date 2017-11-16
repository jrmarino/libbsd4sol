#ifndef LIBBSD_SYS_FILE_H
#define LIBBSD_SYS_FILE_H

# define LOCK_SH		0x01
# define LOCK_EX		0x02
# define LOCK_NB		0x04
# define LOCK_UN		0x08

int flock(int, int);

#endif /* LIBBSD_SYS_FILE_H */
