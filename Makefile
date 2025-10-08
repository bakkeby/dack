# dack - a background / wallpaper setter
# See LICENSE file for copyright and license details.

include config.mk

SRC = dack.c ${COMPATSRC}
OBJ = ${SRC:.c=.o}

all: dack

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

dack: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f dack ${OBJ} dack-${VERSION}.tar.gz

dist: clean
	mkdir -p dack-${VERSION}
	cp -R LICENSE Makefile dack.1 config.mk \
		${SRC} dack-${VERSION}
	tar -cf dack-${VERSION}.tar dack-${VERSION}
	gzip dack-${VERSION}.tar
	rm -rf dack-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dack ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dack
	chmod u+s ${DESTDIR}${PREFIX}/bin/dack
	echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" <dack.1 >${DESTDIR}${MANPREFIX}/man1/dack.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dack.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dack
	rm -f ${DESTDIR}${MANPREFIX}/man1/dack.1

.PHONY: all clean dist install uninstall
