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

all:	install-prc$(EXT) install-memo$(EXT) install-user$(EXT)\
        reminders$(EXT) memos$(EXT)\
	test-connector$(EXT) test-acceptor$(EXT) debugsh$(EXT) dlpsh$(EXT)

submake:
	cd lib; make

libpisock.a: submake
	cp lib/libpisock.a libpisock.a

install-memo$(EXT): libpisock.a install-memo.o
	$(CC) $(CFLAGS) install-memo.o libpisock.a -o $@ $(LIBS)

install-user$(EXT): libpisock.a install-user.o
	$(CC) $(CFLAGS) install-user.o libpisock.a -o $@ $(LIBS)

install-prc$(EXT): libpisock.a install-prc.o
	$(CC) $(CFLAGS) install-prc.o libpisock.a -o $@ $(LIBS)

reminders$(EXT): libpisock.a reminders.o
	$(CC) $(CFLAGS) reminders.o libpisock.a -o $@ $(LIBS)

memos$(EXT): libpisock.a memos.o
	$(CC) $(CFLAGS) memos.o libpisock.a -o $@ $(LIBS)

test-connector$(EXT): libpisock.a test-connector.o
	$(CC) $(CFLAGS) test-connector.o libpisock.a -o $@ $(LIBS)

test-acceptor$(EXT): libpisock.a test-acceptor.o
	$(CC) $(CFLAGS) test-acceptor.o libpisock.a -o $@ $(LIBS)

dlpsh$(EXT): libpisock.a dlpsh.o
	$(CC) $(CFLAGS) dlpsh.o libpisock.a -o $@ $(LIBS)

debugsh$(EXT): libpisock.a debugsh.o
	$(CC) $(CFLAGS) debugsh.o libpisock.a -o $@ $(LIBS)

clean:
	$(RM) *.o *.a *~ core a.out test_s test_c install-prc$(EXT) 
	$(RM) install-memo$(EXT) install-user$(EXT) dlpsh$(EXT)
	$(RM) reminders$(EXT) memos$(EXT)
	$(RM) test-acceptor$(EXT) test-connector$(EXT) debugsh$(EXT)
	$(RM) include/*~ *.orig
	cd lib ; make clean
