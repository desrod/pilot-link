OBJS = socket.o serial.o slp.o padp.o prcread.o utils.o dlp.o cmp.o
SERVEROBJS = test_s.o all-sync.o DB-sync.o

CC = gcc
CFLAGS = -O2 -g
# -DDEBUG
AR = ar -cur
RANLIB = ranlib
RM = rm -f

all: libpisock.a install-prc test_c install-memo
#test_s  #test_s and install-prc have not tracked the DLP patches!

libpisock.a: $(OBJS)
	$(RM) libpisock.a
	$(AR) libpisock.a $(OBJS)
	$(RANLIB) libpisock.a

test_s: libpisock.a $(SERVEROBJS)
	$(CC) $(CFLAGS) $(SERVEROBJS) libpisock.a -o test_s

test_c: libpisock.a test_c.o
	$(CC) $(CFLAGS) test_c.o libpisock.a -o test_c

install-memo: libpisock.a install-memo.o
	$(CC) $(CFLAGS) install-memo.o libpisock.a -o $@

install-prc: libpisock.a install-prc.o
	$(CC) $(CFLAGS) install-prc.o libpisock.a -o install-prc

clean:
	$(RM) *.[oa] *~ core a.out test_[sc] install-prc install-memo
