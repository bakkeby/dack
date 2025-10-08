# dack version
VERSION = 0.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

PKG_CONFIG = pkg-config

# Optionally dack can be compiled with imlib2 support to load e.g. .png or jpg images
# to be used as a background. Note that if this is not included dack can still read a
# farbfeld image without the use of this library. Screenshot of the current desktop is
# also done without the use of this library.
#IMLIB=-lImlib2
#IM=-DHAVE_IMLIB=1

CONFIG = `$(PKG_CONFIG) --libs libconfig`

# includes and libs
INCS = -I. -I/usr/include -I${X11INC}
LIBS = -L/usr/lib -lc -lcrypt -L${X11LIB} -lX11 -lm -lXext -lXrandr ${CONFIG} ${IMLIB}

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -DHAVE_SHADOW_H ${IM}
CFLAGS = -std=c99 -pedantic -Wall -Wno-unused-function -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}
