OBJS = socket.o serial.o slp.o padp.o prcread.o utils.o \
       dlp.o cmp.o syspkt.o datebook.o memo.o address.o todo.o \
       sync.c

CC = gcc
CFLAGS = -O2 -g -I../include
# -DDEBUG
# -DTRACE
AR = ar -cur
RANLIB = ranlib
#RANLIB = ar -s
#RANLIB = true
RM = rm -f

all: libpisock.a

libpisock.a: $(OBJS)
	$(RM) libpisock.a
	$(AR) libpisock.a $(OBJS)
	$(RANLIB) libpisock.a

clean:
	$(RM) *.o *.a *~ core a.out *.orig

depend:
	gcc -MM -I../include *.c >> Makefile

#Dependencies
address.o: address.c ../include/pi-socket.h ../include/dlp.h \
 ../include/address.h
cmp.o: cmp.c ../include/pi-socket.h ../include/padp.h ../include/cmp.h
datebook.o: datebook.c ../include/pi-socket.h ../include/dlp.h \
 ../include/datebook.h
dlp.o: dlp.c ../include/pi-socket.h ../include/dlp.h
memo.o: memo.c ../include/pi-socket.h ../include/dlp.h \
 ../include/memo.h
padp.o: padp.c ../include/pi-socket.h ../include/padp.h \
 ../include/slp.h
prcread.o: prcread.c ../include/prc.h ../include/dlp.h \
 ../include/pi-socket.h
serial.o: serial.c ../include/pi-socket.h ../include/pi-serial.h
slp.o: slp.c ../include/pi-socket.h ../include/slp.h
socket.o: socket.c ../include/pi-socket.h ../include/pi-serial.h \
 ../include/padp.h ../include/cmp.h ../include/dlp.h
sync.o: sync.c ../include/pi-socket.h ../include/padp.h \
 ../include/dlp.h ../include/sync.h
syspkt.o: syspkt.c ../include/pi-socket.h ../include/syspkt.h
todo.o: todo.c ../include/pi-socket.h ../include/dlp.h \
 ../include/todo.h
utils.o: utils.c ../include/pi-socket.h
