#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

int fchmodat(int dirfd, const char* pathname, mode_t mode, int flags)
{
    if (flags & AT_SYMLINK_NOFOLLOW) {
        errno = ENOTSUP;
        return (-1);
    }
    int fd = openat(dirfd, pathname, 0);
    if (fd == -1) {
        /* errno set by openat */
        return (-1);
    }
    int result = fchmod(fd, mode);
    int save_errno = errno;
    if (close(fd) == -1) {
        errno = save_errno;
    }
    return (result);
}
