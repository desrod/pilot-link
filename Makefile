OBJS = socket.o serial.o slp.o padp.o utils.o
SERVEROBJS = test_s.o all-sync.o DB-sync.o

CC = gcc
CFLAGS = -O2 -g -DDEBUG
AR = ar -cur
RANLIB = ranlib
RM = rm -f

all: libpisock.a test_s test_c

libpisock.a: $(OBJS)
	$(RM) libpisock.a
	$(AR) libpisock.a $(OBJS)
	$(RANLIB) libpisock.a

test_s: libpisock.a $(SERVEROBJS)
	$(CC) $(CFLAGS) $(SERVEROBJS) libpisock.a -o test_s

test_c: libpisock.a test_c.o
	$(CC) $(CFLAGS) test_c.o libpisock.a -o test_c

clean:
	$(RM) *.[oa] core a.out
