OBJS = socket.o serial.o slp.o padp.o prcread.o utils.o dlp.o cmp.o syspkt.o

CC = gcc
CFLAGS = -O2 -g -I../include
# -DDEBUG
AR = ar -cur
RANLIB = ranlib
#RANLIB = ar -s
RM = rm -f

all: libpisock.a

libpisock.a: $(OBJS)
	$(RM) libpisock.a
	$(AR) libpisock.a $(OBJS)
	$(RANLIB) libpisock.a

clean:
	$(RM) *.o *.a *~ core a.out
