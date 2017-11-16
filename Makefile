# This package is unique for Solaris.  Use libbsd for Linux
# Leave shared library instructions in, but this starts as static lib
#
# Directories             installs
#   bsd                     $LB/include/bsd
#   bsd/sys                 $LB/include/bsd/sys

.SUFFIXES:	.o .c
.SUFFIXES:	.So .c

.c.o:
	${CC} ${_${.IMPSRC:T}_FLAGS} ${PICFLAG} -DPIC ${CFLAGS} -c ${.IMPSRC}

.c.So:
	${CC} ${_${.IMPSRC:T}_FLAGS} ${PICFLAG} -DPIC ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

LIB=		bsd
LIB_STATIC=	lib${LIB}.a
PICFLAG=	-fpic
CFLAGS+=	-Ibsd -I.
SRCDIR=		src

.PATH:		${SRCDIR}

all: ${LIB_STATIC} # ${LIB_SHARED}

TSORT?=		tsort 2>/dev/null
LORDER?=	lorder

SRCS=		err.c stringlist.c strsep.c bzero.c reallocarray.c vis.c \
		setmode.c unvis.c setproctitle.c progname.c dprintf.c \
		strndup.c strmode.c flock.c getline.c time.c fchmodat.c \
		arc4random.c arc4random_uniform.c explicit_bzero.c \
		strcasestr.c getentropy_solaris.c sha512c.c reallocf.c \
		strtonum.c fgetln.c

# libexec sources
PREFIX?=	/usr/local
OBJS=		${SRCS:.c=.o}
SOBJS=		${SRCS:.c=.So}

${LIB_STATIC}: ${OBJS}
	@${ECHO} building static ${LIB} library
	${AR} cq ${.TARGET} `${LORDER} ${OBJS} | ${TSORT}`
	ranlib ${.TARGET}

${LIB_SHARED}: ${SOBJS}
	@${ECHO} building shared ${LIB} library
	${CC_LINK} ${LDFLAGS} -shared -Wl,-x \
	    -o ${.TARGET} -Wl,-soname,${.TARGET} \
	    `${LORDER} ${SOBJS} | ${TSORT}`

install:
	mkdir -p ${DESTDIR}${PREFIX}/include/bsd/sys
	mkdir -p ${DESTDIR}${PREFIX}/lib
	# ${BSD_INSTALL_LIB} ${LIB_SHARED} ${DESTDIR}${PREFIX}/lib
	${BSD_INSTALL_DATA} ${LIB_STATIC} ${DESTDIR}${PREFIX}/lib
	# ln -s ${LIB_SHARED} ${DESTDIR}${PREFIX}/lib/${LIB_LINK}
.for h in err.h stringlist.h string.h strings.h stdlib.h stdio.h \
	unistd.h vis.h fcntl.h paths.h \
	sys/cdefs.h sys/tree.h sys/file.h sys/time.h sys/stat.h \
	sys/endian.h
	${BSD_INSTALL_DATA} bsd/$h ${DESTDIR}${PREFIX}/include/bsd/$h
.endfor
