OBJS = socket.o serial.o slp.o padp.o prcread.o utils.o dlp.o cmp.o

CC = gcc
CFLAGS = -O2 -g -I../include
# -DDEBUG
AR = ar -cur
RANLIB = ranlib
RM = rm -f

all: libpisock.a

libpisock.a: $(OBJS)
	$(RM) libpisock.a
	$(AR) libpisock.a $(OBJS)
	$(RANLIB) libpisock.a

clean:
	$(RM) *.[oa] *~ core a.out test_[sc] install-prc install-memo install-user
