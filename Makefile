CC = gcc
CFLAGS = -O2 -g -I./include
# -DDEBUG
AR = ar cur
RANLIB = ranlib
#RANLIB = ar -s
#RANLIB = true
RM = rm -f
LIBS =
# -lefence
#LIBS = -lsocket -los2
EXT =
#EXT = .exe
SUBMAKE_COMM = cd lib ; $(MAKE)
#SUBMAKE_COMM = $(MAKE) -C lib

all:	submake \
        install-prc$(EXT) install-pdb$(EXT) install-memo$(EXT) install-user$(EXT)\
	install-todos$(EXT) read-ical$(EXT)\
	retrieve-prc$(EXT) retrieve-pdb$(EXT)\
        reminders$(EXT) memos$(EXT) addresses$(EXT) read-todos$(EXT)\
	test-connector$(EXT) test-acceptor$(EXT) debugsh$(EXT) dlpsh$(EXT)

submake:
	$(SUBMAKE_COMM)

libpisock.a: lib/libpisock.a
	cp lib/libpisock.a libpisock.a

install-memo$(EXT): libpisock.a install-memo.o
	$(CC) $(CFLAGS) install-memo.o libpisock.a -o $@ $(LIBS)

install-todos$(EXT): libpisock.a install-todos.o
	$(CC) $(CFLAGS) install-todos.o libpisock.a -o $@ $(LIBS)

install-user$(EXT): libpisock.a install-user.o
	$(CC) $(CFLAGS) install-user.o libpisock.a -o $@ $(LIBS)

install-prc$(EXT): libpisock.a install-prc.o
	$(CC) $(CFLAGS) install-prc.o libpisock.a -o $@ $(LIBS)

install-pdb$(EXT): libpisock.a install-pdb.o
	$(CC) $(CFLAGS) install-pdb.o libpisock.a -o $@ $(LIBS)

retrieve-prc$(EXT): libpisock.a retrieve-prc.o
	$(CC) $(CFLAGS) retrieve-prc.o libpisock.a -o $@ $(LIBS)

retrieve-pdb$(EXT): libpisock.a retrieve-pdb.o
	$(CC) $(CFLAGS) retrieve-pdb.o libpisock.a -o $@ $(LIBS)

reminders$(EXT): libpisock.a reminders.o
	$(CC) $(CFLAGS) reminders.o libpisock.a -o $@ $(LIBS)

memos$(EXT): libpisock.a memos.o
	$(CC) $(CFLAGS) memos.o libpisock.a -o $@ $(LIBS)

read-todos$(EXT): libpisock.a read-todos.o
	$(CC) $(CFLAGS) read-todos.o libpisock.a -o $@ $(LIBS)

read-ical$(EXT): libpisock.a read-ical.o
	$(CC) $(CFLAGS) read-ical.o libpisock.a -o $@ $(LIBS)

addresses$(EXT): libpisock.a addresses.o
	$(CC) $(CFLAGS) addresses.o libpisock.a -o $@ $(LIBS)

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
	$(RM) install-memo$(EXT) install-user$(EXT) dlpsh$(EXT) install-pdb$(EXT)
	$(RM) read-ical$(EXT)
	$(RM) retrieve-prc$(EXT) retrieve-pdb$(EXT) install-todos$(EXT)
	$(RM) reminders$(EXT) memos$(EXT) read-todos$(EXT) addresses$(EXT)
	$(RM) test-acceptor$(EXT) test-connector$(EXT) debugsh$(EXT)
	$(RM) include/*~ man/*~ *.orig include/*.orig
	$(SUBMAKE_COMM) clean
