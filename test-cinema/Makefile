SRCS=example-cinema.c test-res-students.c
OBJS=${SRCS:.c=}

INCLUDES = -I. -I$(srcdir) -I../include -I ../include 
CC = gcc
CFLAGS ?= -g -O0 -Wall -m32 -pthread 
DEFS ?= -DHAVE_CONFIG_H -DUSE_PTHREADS
LIBSTHREAD = ../sthread_lib/libsthread.a
LIBCINEMA =  ../cinema/libcinema.a

COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS)

all: $(OBJS)

% : %.c
	$(COMPILE) ../sthread_lib/sthread_start.o -o $@ $<  $(LIBSTHREAD)  $(LIBCINEMA)

clean: 
	rm -f $(OBJS)


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
