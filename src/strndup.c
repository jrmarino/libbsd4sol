#include <stdlib.h>
#include <string.h>

char *
strndup(const char *str, size_t n)
{
    size_t len;
    char *copy;

    len = strlen(str);
    if (len <= n)
        return strdup(str);
    if ((copy = malloc(len + 1)) == NULL)
        return (NULL);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return (copy);
}
