CC = gcc
CFLAGS = -O2 -g -I./include
# -DDEBUG
AR = ar -cur
RANLIB = ranlib
#RANLIB = ar -s
RM = rm -f
LIBS =
#LIBS = -lsocket -los2
EXT =
#EXT = .exe

all: install-prc$(EXT) install-memo$(EXT) install-user$(EXT) \
	test-connector$(EXT) test-acceptor$(EXT) test-debug$(EXT) dlpsh$(EXT)

libpisock.a: lib/libpisock.a
	mv lib/libpisock.a libpisock.a

install-memo$(EXT): libpisock.a install-memo.o
	$(CC) $(CFLAGS) install-memo.o libpisock.a -o $@ $(LIBS)

install-user$(EXT): libpisock.a install-user.o
	$(CC) $(CFLAGS) install-user.o libpisock.a -o $@ $(LIBS)

install-prc$(EXT): libpisock.a install-prc.o
	$(CC) $(CFLAGS) install-prc.o libpisock.a -o $@ $(LIBS)

test-connector$(EXT): libpisock.a test-connector.o
	$(CC) $(CFLAGS) test-connector.o libpisock.a -o $@ $(LIBS)

test-acceptor$(EXT): libpisock.a test-acceptor.o
	$(CC) $(CFLAGS) test-acceptor.o libpisock.a -o $@ $(LIBS)

dlpsh$(EXT): libpisock.a dlpsh.o
	$(CC) $(CFLAGS) dlpsh.o libpisock.a -o $@ $(LIBS)

test-debug$(EXT): libpisock.a test-debug.o
	$(CC) $(CFLAGS) test-debug.o libpisock.a -o $@ $(LIBS)

lib/libpisock.a:
	cd lib ; make

clean:
	$(RM) *.o *.a *~ core a.out test_s test_c install-prc$(EXT) 
	$(RM) install-memo$(EXT) install-user$(EXT) dlpsh$(EXT)
	$(RM) test-acceptor$(EXT) test-connector$(EXT) test-debug$(EXT)
	$(RM) include/*~
	cd lib ; make clean
