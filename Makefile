CC = gcc
LINKER = gcc
CFLAGS = -std=c99 -Wall # -Werror -Wpointer-arith -Wfatal-errors
DEBUG = -g
PROG1LIBNAME = prog1
PROG1LIBDIR = ../prog1lib/lib


CORE_SRC = game.c board.c

# disable default suffixes
.SUFFIXES:

# pattern rule for compiling the library
prog1lib:
	cd $(PROG1LIBDIR) && make

# pattern rule for compiling .c-file to executable

%: %.c prog1lib
	$(CC) $(CFLAGS) $(DEBUG) $< -L$(PROG1LIBDIR) -l$(PROG1LIBNAME) -iquote$(PROG1LIBDIR) -o $@

core.o: board.c game.c
	$(CC) -c $(CFLAGS) $(DEBUG) $< -o $@
  
evol_ai.o: evol_ai.c
	$(CC) -c $(CFLAGS) $(DEBUG) $< -o $@

evol_ai: core.o evol_ai.o
	$(LINKER) $(CFLAGS) $(DEBUG) $< -o $@

tweaked_evol_player: tweaked_evol_ai.c board.c game.c
	$(CC) $(CFLAGS) $(DEBUG) $< -o $@

