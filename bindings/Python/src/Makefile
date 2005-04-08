
all:
	swig -python pisock.i
	perl nonblocking_multithread.pl < pisock_wrap.c > pisock_wrap1.c
	mv pisock_wrap1.c pisock_wrap.c
