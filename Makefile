CC = gcc
CFLAGS = -O2 -g -I./include
# -DDEBUG
AR = ar -cur
RANLIB = ranlib
RM = rm -f

all: install-prc install-memo install-user

libpisock.a: lib/libpisock.a
	mv lib/libpisock.a libpisock.a

install-memo: libpisock.a install-memo.o
	$(CC) $(CFLAGS) install-memo.o libpisock.a -o $@

install-user: libpisock.a install-user.o
	$(CC) $(CFLAGS) install-user.o libpisock.a -o $@

install-prc: libpisock.a install-prc.o
	$(CC) $(CFLAGS) install-prc.o libpisock.a -o $@

lib/libpisock.a:
	make -C lib

clean:
	$(RM) *.[oa] *~ core a.out test_[sc] install-prc install-memo install-user
	make -C lib clean
