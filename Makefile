CC = gcc
LINKER = gcc
CFLAGS = -std=c99 -Wall # -Werror -Wpointer-arith -Wfatal-errors
DEBUG = -g
PROG1LIBNAME = prog1
PROG1LIBDIR = ../prog1lib/lib

# disable default suffixes
.SUFFIXES:

# pattern rule for compiling the library
prog1lib:
	cd $(PROG1LIBDIR) && make

# pattern rule for compiling .c-file to executable

max_salt: get_smoked.c prog1lib
	$(CC) $(CFLAGS) $(DEBUG) $< -L$(PROG1LIBDIR) -l$(PROG1LIBNAME) -iquote$(PROG1LIBDIR) -o the_player

%: %.c prog1lib
	$(CC) $(CFLAGS) $(DEBUG) $< -L$(PROG1LIBDIR) -l$(PROG1LIBNAME) -iquote$(PROG1LIBDIR) -o $@
