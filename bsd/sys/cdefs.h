#ifndef LIBBSD_SYS_CDEFS_H
#define LIBBSD_SYS_CDEFS_H

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS	extern "C" {
#define __END_DECLS	}
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

/*
 * Some kFreeBSD headers expect those macros to be set for sanity checks.
 */
#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_
#endif
#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H
#endif

#ifdef __GNUC__
#define LIBBSD_GCC_VERSION (__GNUC__ << 8 | __GNUC_MINOR__)
#else
#define LIBBSD_GCC_VERSION 0
#endif

#if LIBBSD_GCC_VERSION >= 0x0405
#define LIBBSD_DEPRECATED(x) __attribute__((deprecated(x)))
#elif LIBBSD_GCC_VERSION >= 0x0301
#define LIBBSD_DEPRECATED(x) __attribute__((deprecated))
#else
#define LIBBSD_DEPRECATED(x)
#endif

#ifndef __dead2
# if LIBBSD_GCC_VERSION >= 0x0207
#  define __dead2 __attribute__((__noreturn__))
# else
#  define __dead2
# endif
#endif

#ifndef __pure2
# if LIBBSD_GCC_VERSION >= 0x0207
#  define __pure2 __attribute__((__const__))
# else
#  define __pure2
# endif
#endif

#ifndef __packed
# if LIBBSD_GCC_VERSION >= 0x0207
#  define __packed __attribute__((__packed__))
# else
#  define __packed
# endif
#endif

#ifndef __aligned
# if LIBBSD_GCC_VERSION >= 0x0207
#  define __aligned(x) __attribute__((__aligned__(x)))
# else
#  define __aligned(x)
# endif
#endif

/* Linux headers define a struct with a member names __unused.
 * Debian bugs: #522773 (linux), #522774 (libc).
 * Disable for now. */
#if 0
#ifndef __unused
# if LIBBSD_GCC_VERSION >= 0x0300
#  define __unused __attribute__((unused))
# else
#  define __unused
# endif
#endif
#endif

#ifndef __printflike
# if LIBBSD_GCC_VERSION >= 0x0300
#  define __printflike(x, y) __attribute((format(printf, (x), (y))))
# else
#  define __printflike(x, y)
# endif
#endif

#ifndef __nonnull
# if LIBBSD_GCC_VERSION >= 0x0302
#  define __nonnull(x) __attribute__((__nonnull__(x)))
# else
#  define __nonnull(x)
# endif
#endif

#ifndef __bounded__
# define __bounded__(x, y, z)
#endif

/*
 * We define this here since <stddef.h>, <sys/queue.h>, and <sys/types.h>
 * require it.
 */
#ifndef __offsetof
# if LIBBSD_GCC_VERSION >= 0x0401
#  define __offsetof(type, field)	__builtin_offsetof(type, field)
# else
#  ifndef __cplusplus
#   define __offsetof(type, field) \
           ((__size_t)(__uintptr_t)((const volatile void *)&((type *)0)->field))
#  else
#   define __offsetof(type, field) \
	(__offsetof__ (reinterpret_cast <__size_t> \
	               (&reinterpret_cast <const volatile char &> \
	                (static_cast<type *> (0)->field))))
#  endif
# endif
#endif

#define __rangeof(type, start, end) \
        (__offsetof(type, end) - __offsetof(type, start))

/*
 * Given the pointer x to the member m of the struct s, return
 * a pointer to the containing structure.  When using GCC, we first
 * assign pointer x to a local variable, to check that its type is
 * compatible with member m.
 */
#ifndef __containerof
# if LIBBSD_GCC_VERSION >= 0x0301
#  define __containerof(x, s, m) ({ \
	const volatile __typeof(((s *)0)->m) *__x = (x); \
	__DEQUALIFY(s *, (const volatile char *)__x - __offsetof(s, m)); \
})
# else
#  define __containerof(x, s, m) \
          __DEQUALIFY(s *, (const volatile char *)(x) - __offsetof(s, m))
# endif
#endif

#ifndef __RCSID
# define __RCSID(x)
#endif

#ifndef __FBSDID
# define __FBSDID(x)
#endif

#ifndef __RCSID
# define __RCSID(x)
#endif

#ifndef __RCSID_SOURCE
# define __RCSID_SOURCE(x)
#endif

#ifndef __SCCSID
# define __SCCSID(x)
#endif

#ifndef __COPYRIGHT
# define __COPYRIGHT(x)
#endif

#undef	__DECONST
#define __DECONST(type, var)	((type)(uintptr_t)(const void *)(var))

#ifndef __DEVOLATILE
#define __DEVOLATILE(type, var)	((type)(uintptr_t)(volatile void *)(var))
#endif

#ifndef __DEQUALIFY
#define __DEQUALIFY(type, var)	((type)(uintptr_t)(const volatile void *)(var))
#endif

#endif
