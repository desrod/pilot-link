# Generated automatically from Makefile.in by configure.

O = o

CCOBJS = datebook.$(O) address.$(O) todo.$(O) appInfo.$(O) memo.$(O) iambicExpense.$(O)

prefix = /usr/pilot
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
cclib = libpicc.a
srcdir = .

top_builddir = ..

CC = gcc
CXX = c++
CFLAGS = -I../include -I$(srcdir)/../include -g -O2 -Wall
# -DPADP_TRACE
# -DNO_DLP_TRACE
# -DNO_SERIAL_TRACE
# -DDEBUG
CXXFLAGS = -I../include -I$(srcdir)/../include -g -O2 -Wall
AR = ar
ARFLAGS = cur
RANLIB = ranlib
RM = rm -f
LIBTOOL = $(top_builddir)/libtool

.SUFFIXES: .out .o .lo .la .cc .c .e .r .f .y .l .s .p

.cc.o:
	$(CXX) $(CXXFLAGS) -c $*.cc

.cc.lo:
	$(LIBTOOL) --mode=compile $(CXX) $(CXXFLAGS) -c $<

all: libpicc.a

libpicc.la: $(CCOBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CXXFLAGS) \
		-export-dynamic -static -rpath $(libdir) -version-info 0:0:0 \
		-o libpicc.la $(CCOBJS)

libpicc.a: $(CCOBJS)
	$(RM) libpicc.a
	$(AR) $(ARFLAGS) libpicc.a $(CCOBJS)
	$(RANLIB) libpicc.a
	
clean:
	$(RM) *.o *.a core a.out *.orig *.lo *.la
	$(RM) -rf .libs _libs

depend:
	perl -i~ -ne 'print if 1../^#Depend/' Makefile.in ; gcc -MM -I../include *.cc >> Makefile.in

#Dependencies
address.o: address.cc ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h \
 ../include/pi-address.h ../include/pi-appinfo.h \
 ../include/pi-appinfo.hxx ../include/pi-address.hxx
appInfo.o: appInfo.cc ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h \
 ../include/pi-appinfo.hxx
datebook.o: datebook.cc ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h \
 ../include/pi-datebook.h ../include/pi-appinfo.h \
 ../include/pi-appinfo.hxx ../include/pi-datebook.hxx
iambicExpense.o: iambicExpense.cc ../include/pi-source.h \
 ../include/pi-config.h ../include/pi-socket.h ../include/pi-args.h \
 ../include/pi-version.h ../include/pi-sockaddr.h \
 ../include/pi-macros.h ../include/pi-iambicExpense.h \
 ../include/pi-appinfo.h ../include/pi-appinfo.hxx \
 ../include/pi-iambicExpense.hxx
memo.o: memo.cc ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h ../include/pi-memo.h \
 ../include/pi-appinfo.h ../include/pi-appinfo.hxx \
 ../include/pi-memo.hxx
todo.o: todo.cc ../include/pi-source.h ../include/pi-config.h \
 ../include/pi-socket.h ../include/pi-args.h ../include/pi-version.h \
 ../include/pi-sockaddr.h ../include/pi-macros.h ../include/pi-todo.h \
 ../include/pi-appinfo.h ../include/pi-appinfo.hxx \
 ../include/pi-todo.hxx
