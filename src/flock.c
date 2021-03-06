/*
 * Solaris portability - flock()
 * https://www.perkin.org.uk/posts/solaris-portability-flock.html
 */

#define __EXTENSIONS__	1
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>

int flock(int fd, int op) {
	int rc = 0;

#if defined(F_SETLK) && defined(F_SETLKW)
	struct flock fl = {0};

	switch (op & (LOCK_EX|LOCK_SH|LOCK_UN)) {
	case LOCK_EX:
		fl.l_type = F_WRLCK;
		break;

	case LOCK_SH:
		fl.l_type = F_RDLCK;
		break;

	case LOCK_UN:
		fl.l_type = F_UNLCK;
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	fl.l_whence = SEEK_SET;
	rc = fcntl(fd, op & LOCK_NB ? F_SETLK : F_SETLKW, &fl);

	if (rc && (errno == EAGAIN))
		errno = EWOULDBLOCK;
#endif

	return rc;
}
