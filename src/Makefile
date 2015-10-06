# See LICENSE file for copyright and license details.

include config.mk

all: dotfiles

dotfiles: dlist.o parse.o regex.o dmap.o

.c.o: config.mk
	@echo CC -c $<
	@${CC} -c $< ${CFLAGS}

dotfiles path:
	@echo CC -o $@
	@${CC} -o $@ $+ ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f dotfiles *.o

update: clean dotfiles
