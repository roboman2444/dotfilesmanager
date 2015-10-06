VERSION = 4.2.1


# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man


LIBS = -lm

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c11 -pedantic -rdynamic -Wextra -Wall ${CPPFLAGS} -g
LDFLAGS  = ${LIBS}

# compiler and linker
CC = gcc
