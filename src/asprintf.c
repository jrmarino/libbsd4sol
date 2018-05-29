/* $Id: asprintf.c 4057 2008-04-08 23:36:59Z rra $
 *
 * Replacement for a missing asprintf and vasprintf.
 *
 * Provides the same functionality as the standard GNU library routines
 * asprintf and vasprintf for those platforms that don't have them.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * This work is hereby placed in the public domain by its author.
*/

#include <stdio.h>

int
asprintf(char **strp, const char *fmt, ...)
{
    va_list args;
    int status;

    va_start(args, fmt);
    status = vasprintf(strp, fmt, args);
    va_end(args);
    return status;
}
