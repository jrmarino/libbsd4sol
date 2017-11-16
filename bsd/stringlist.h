#ifndef LIBBSD_STRINGLIST_H
#define LIBBSD_STRINGLIST_H
#include <sys/cdefs.h>
#include <sys/types.h>

/*
 * Simple string list
 */
typedef struct _stringlist {
	char	**sl_str;
	size_t	  sl_max;
	size_t	  sl_cur;
} StringList;

__BEGIN_DECLS
StringList	*sl_init(void);
int		 sl_add(StringList *, char *);
void		 sl_free(StringList *, int);
char		*sl_find(StringList *, const char *);
int		 sl_delete(StringList *, const char *, int);
__END_DECLS

#endif /* LIBBSD_STRINGLIST_H */
