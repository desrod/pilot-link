# Generated automatically from Makefile.in by configure.

OBJS = Interface.lo Pdapilot_calls.lo Pdapilot_memo_Record.lo Pdapilot_memo_AppBlock.lo\
		Pdapilot_todo_Record.lo Pdapilot_todo_AppBlock.lo

LIBS = -L${libdir} -lpisock

STUBH = java_util_Date.h Pdapilot_calls.h Pdapilot_constants.h\
		Pdapilot_Database.h Pdapilot_Block.h Pdapilot_Record.h\
		Pdapilot_memo_Record.h Pdapilot_Resource.h Pdapilot_Char4.h Pdapilot_RecordID.h\
		Pdapilot_CardInfo.h Pdapilot_SysInfo.h Pdapilot_UserInfo.h \
		Pdapilot_DBInfo.h Pdapilot_NetInfo.h Pdapilot_AppBlock.h Pdapilot_SortBlock.h\
		Pdapilot_Pref.h Pdapilot_memo_AppBlock.h Pdapilot_todo_AppBlock.h Pdapilot_todo_Record.h

STUBC = Pdapilot_calls.c Pdapilot_memo_Record.c Pdapilot_memo_AppBlock.c Pdapilot_constants.c \
		Pdapilot_todo_Record.c Pdapilot_todo_AppBlock.c

prefix = /usr/pilot
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
srcdir = .

java = /usr/local/java
JAVACOMPILE = $(java)/bin/javac

top_builddir = ..

CC = gcc
CFLAGS = -I$(java)/include/genunix -I$(java)/include -I../include -I$(srcdir)/../include -g -O2 -Wall
AR = ar
ARFLAGS = cur
RANLIB = ranlib
RM = rm -f
LIBTOOL = $(top_builddir)/libtool

.SUFFIXES: .c .lo .o .class .java

.c.o:
	$(CC) $(CFLAGS) -c $<

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

.java.class:
	$(java)/bin/javac $<

all: libJavaPisock.la $(CLASSES)

doConstants: doConstants.o
	$(CC) -o doConstants doConstants.o

stubs: $(STUBH) $(STUBC)
	
java_util_Date.h:
	$(java)/bin/javah java.util.Date

Pdapilot/constants.java: doConstants
	doConstants

Pdapilot_calls.h: Pdapilot/calls.class
	$(java)/bin/javah Pdapilot.calls

Pdapilot_UserInfo.h: Pdapilot/UserInfo.class
	$(java)/bin/javah Pdapilot.UserInfo

Pdapilot_SysInfo.h: Pdapilot/SysInfo.class
	$(java)/bin/javah Pdapilot.SysInfo

Pdapilot_calls.c: Pdapilot/calls.class
	$(java)/bin/javah -stubs Pdapilot.calls

Pdapilot_memo_Record.c: Pdapilot/memo/Record.class
	$(java)/bin/javah -stubs Pdapilot.memo.Record

Pdapilot_todo_Record.c: Pdapilot/todo/Record.class
	$(java)/bin/javah -stubs Pdapilot.todo.Record

Pdapilot_constants.h: Pdapilot/constants.class
	$(java)/bin/javah Pdapilot.constants

Pdapilot_DBInfo.h: Pdapilot/DBInfo.class
	$(java)/bin/javah Pdapilot.DBInfo

Pdapilot_NetInfo.h: Pdapilot/NetInfo.class
	$(java)/bin/javah Pdapilot.NetInfo

Pdapilot_AppBlock.h: Pdapilot/AppBlock.class
	$(java)/bin/javah Pdapilot.AppBlock

Pdapilot_memo_AppBlock.c: Pdapilot/memo/AppBlock.class
	$(java)/bin/javah -stubs Pdapilot.memo.AppBlock

Pdapilot_todo_AppBlock.c: Pdapilot/todo/AppBlock.class
	$(java)/bin/javah -stubs Pdapilot.todo.AppBlock

Pdapilot_SortBlock.h: Pdapilot/SortBlock.class
	$(java)/bin/javah Pdapilot.SortBlock

Pdapilot_Database.h: Pdapilot/Database.class
	$(java)/bin/javah Pdapilot.Database

Pdapilot_Block.h: Pdapilot/Block.class
	$(java)/bin/javah Pdapilot.Block

Pdapilot_CardInfo.h: Pdapilot/CardInfo.class
	$(java)/bin/javah Pdapilot.CardInfo

Pdapilot_Record.h: Pdapilot/Record.class
	$(java)/bin/javah Pdapilot.Record

Pdapilot_memo_Record.h: Pdapilot/memo/Record.class
	$(java)/bin/javah Pdapilot.memo.Record

Pdapilot_memo_AppBlock.h: Pdapilot/memo/AppBlock.class
	$(java)/bin/javah Pdapilot.memo.AppBlock

Pdapilot_todo_Record.h: Pdapilot/todo/Record.class
	$(java)/bin/javah Pdapilot.todo.Record

Pdapilot_todo_AppBlock.h: Pdapilot/todo/AppBlock.class
	$(java)/bin/javah Pdapilot.todo.AppBlock

Pdapilot_Resource.h: Pdapilot/Resource.class
	$(java)/bin/javah Pdapilot.Resource

Pdapilot_Char4.h: Pdapilot/Char4.class
	$(java)/bin/javah Pdapilot.Char4

Pdapilot_Pref.h: Pdapilot/Pref.class
	$(java)/bin/javah Pdapilot.Pref

Pdapilot_RecordID.h: Pdapilot/RecordID.class
	$(java)/bin/javah Pdapilot.RecordID

Pdapilot_constants.c: Pdapilot/constants.class
	$(java)/bin/javah -stubs Pdapilot.constants

libJavaPisock.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) \
		-export-dynamic -rpath $(libdir) -version-info 0:0:0 \
		-o libJavaPisock.la $(OBJS) $(LIBS)

test: libJavaPisock.la $(CLASSES) test.class
	/bin/sh dotest $(java)/bin/java

realclean: clean
	$(RM) Makefile

clean:
	$(RM) *.o *.lo *.a *.la core a.out *.orig *.so *.class */*.class */*/*.class doConstants
	$(RM) $(STUBC) $(STUBH)
	$(RM) -r .libs

depend:
	perl -i~ -ne 'print if 1../^#Depend/' Makefile.in ; gcc -w -MM -I../include *.c | perl -pne 's/^(\S*)\.o/$$& $$1.lo/g' >> Makefile.in ; jdeps *.java */*.java */*/*.java >> Makefile.in

#Dependencies
Interface.o Interface.lo: Interface.c Pdapilot_calls.h Pdapilot_Database.h \
 Pdapilot_Record.h Pdapilot_Pref.h Pdapilot_AppBlock.h \
 Pdapilot_SortBlock.h Pdapilot_Resource.h Pdapilot_CardInfo.h \
 Pdapilot_UserInfo.h Pdapilot_SysInfo.h Pdapilot_NetInfo.h \
 Pdapilot_memo_Record.h Pdapilot_memo_AppBlock.h \
 Pdapilot_todo_Record.h Pdapilot_todo_AppBlock.h Pdapilot_RecordID.h \
 Pdapilot_Char4.h Pdapilot_DBInfo.h java_util_Date.h \
 ../include/pi-source.h ../include/pi-config.h ../include/pi-socket.h \
 ../include/pi-args.h ../include/pi-macros.h ../include/pi-dlp.h \
 ../include/pi-memo.h ../include/pi-todo.h
Pdapilot_calls.o Pdapilot_calls.lo: Pdapilot_calls.c
Pdapilot_constants.o Pdapilot_constants.lo: Pdapilot_constants.c
Pdapilot_memo_AppBlock.o Pdapilot_memo_AppBlock.lo: Pdapilot_memo_AppBlock.c
Pdapilot_memo_Record.o Pdapilot_memo_Record.lo: Pdapilot_memo_Record.c
Pdapilot_todo_AppBlock.o Pdapilot_todo_AppBlock.lo: Pdapilot_todo_AppBlock.c
Pdapilot_todo_Record.o Pdapilot_todo_Record.lo: Pdapilot_todo_Record.c
doConstants.o doConstants.lo: doConstants.c ../include/pi-source.h \
 ../include/pi-config.h ../include/pi-socket.h ../include/pi-args.h \
 ../include/pi-macros.h ../include/pi-dlp.h
# -- Automatically generated by JavaDeps --

# Rules for Pdapilot.Block
#
Pdapilot/Block.class: Pdapilot/Block.java
	$(JAVACOMPILE) Pdapilot/Block.java


# Rules for Pdapilot.RecordID
#
Pdapilot/RecordID.class: Pdapilot/RecordID.java
	$(JAVACOMPILE) Pdapilot/RecordID.java


# Rules for Pdapilot.Record
#
Pdapilot/Record.class: Pdapilot/Record.java
	$(JAVACOMPILE) Pdapilot/Record.java

Pdapilot/Record.class: Pdapilot/Block.class

Pdapilot/Record.class: Pdapilot/RecordID.class


# Rules for Pdapilot.Char4
#
Pdapilot/Char4.class: Pdapilot/Char4.java
	$(JAVACOMPILE) Pdapilot/Char4.java


# Rules for Pdapilot.Resource
#
Pdapilot/Resource.class: Pdapilot/Resource.java
	$(JAVACOMPILE) Pdapilot/Resource.java

Pdapilot/Resource.class: Pdapilot/Block.class

Pdapilot/Resource.class: Pdapilot/Char4.class


# Rules for Pdapilot.Pref
#
Pdapilot/Pref.class: Pdapilot/Pref.java
	$(JAVACOMPILE) Pdapilot/Pref.java

Pdapilot/Pref.class: Pdapilot/Block.class

Pdapilot/Pref.class: Pdapilot/Char4.class


# Rules for Pdapilot.AppBlock
#
Pdapilot/AppBlock.class: Pdapilot/AppBlock.java
	$(JAVACOMPILE) Pdapilot/AppBlock.java

Pdapilot/AppBlock.class: Pdapilot/Block.class


# Rules for Pdapilot.SortBlock
#
Pdapilot/SortBlock.class: Pdapilot/SortBlock.java
	$(JAVACOMPILE) Pdapilot/SortBlock.java

Pdapilot/SortBlock.class: Pdapilot/Block.class


# Rules for Pdapilot.memo.AppBlock
#
Pdapilot/memo/AppBlock.class: Pdapilot/memo/AppBlock.java
	$(JAVACOMPILE) Pdapilot/memo/AppBlock.java

Pdapilot/memo/AppBlock.class: Pdapilot/AppBlock.class


# Rules for Pdapilot.memo.Record
#
Pdapilot/memo/Record.class: Pdapilot/memo/Record.java
	$(JAVACOMPILE) Pdapilot/memo/Record.java

Pdapilot/memo/Record.class: Pdapilot/Record.class

Pdapilot/memo/Record.class: Pdapilot/RecordID.class


# Rules for Pdapilot.Database Pdapilot.memo.Database 
#
Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/Database.java Pdapilot/memo/Database.java 
	$(JAVACOMPILE) Pdapilot/Database.java Pdapilot/memo/Database.java 

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/Record.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/RecordID.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/Resource.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/Char4.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/Pref.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/AppBlock.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/SortBlock.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/memo/AppBlock.class

Pdapilot/Database.class Pdapilot/memo/Database.class : Pdapilot/memo/Record.class


# Rules for Pdapilot.todo.AppBlock
#
Pdapilot/todo/AppBlock.class: Pdapilot/todo/AppBlock.java
	$(JAVACOMPILE) Pdapilot/todo/AppBlock.java

Pdapilot/todo/AppBlock.class: Pdapilot/AppBlock.class


# Rules for Pdapilot.Util
#
Pdapilot/Util.class: Pdapilot/Util.java
	$(JAVACOMPILE) Pdapilot/Util.java


# Rules for Pdapilot.todo.Record
#
Pdapilot/todo/Record.class: Pdapilot/todo/Record.java
	$(JAVACOMPILE) Pdapilot/todo/Record.java

Pdapilot/todo/Record.class: Pdapilot/Util.class

Pdapilot/todo/Record.class: Pdapilot/Record.class

Pdapilot/todo/Record.class: Pdapilot/RecordID.class


# Rules for Pdapilot.todo.Database
#
Pdapilot/todo/Database.class: Pdapilot/todo/Database.java
	$(JAVACOMPILE) Pdapilot/todo/Database.java

Pdapilot/todo/Database.class: Pdapilot/Database.class Pdapilot/memo/Database.class 

Pdapilot/todo/Database.class: Pdapilot/AppBlock.class

Pdapilot/todo/Database.class: Pdapilot/todo/AppBlock.class

Pdapilot/todo/Database.class: Pdapilot/Record.class

Pdapilot/todo/Database.class: Pdapilot/todo/Record.class

Pdapilot/todo/Database.class: Pdapilot/RecordID.class

Pdapilot/todo/Database.class: Pdapilot/Char4.class


# Rules for Pdapilot.DBInfo
#
Pdapilot/DBInfo.class: Pdapilot/DBInfo.java
	$(JAVACOMPILE) Pdapilot/DBInfo.java

Pdapilot/DBInfo.class: Pdapilot/Char4.class


# Rules for Pdapilot.DlpException
#
Pdapilot/DlpException.class: Pdapilot/DlpException.java
	$(JAVACOMPILE) Pdapilot/DlpException.java


# Rules for Pdapilot.CardInfo
#
Pdapilot/CardInfo.class: Pdapilot/CardInfo.java
	$(JAVACOMPILE) Pdapilot/CardInfo.java


# Rules for Pdapilot.UserInfo
#
Pdapilot/UserInfo.class: Pdapilot/UserInfo.java
	$(JAVACOMPILE) Pdapilot/UserInfo.java


# Rules for Pdapilot.SysInfo
#
Pdapilot/SysInfo.class: Pdapilot/SysInfo.java
	$(JAVACOMPILE) Pdapilot/SysInfo.java


# Rules for Pdapilot.NetInfo
#
Pdapilot/NetInfo.class: Pdapilot/NetInfo.java
	$(JAVACOMPILE) Pdapilot/NetInfo.java


# Rules for Pdapilot.calls
#
Pdapilot/calls.class: Pdapilot/calls.java
	$(JAVACOMPILE) Pdapilot/calls.java

Pdapilot/calls.class: Pdapilot/DlpException.class

Pdapilot/calls.class: Pdapilot/AppBlock.class

Pdapilot/calls.class: Pdapilot/Database.class Pdapilot/memo/Database.class 

Pdapilot/calls.class: Pdapilot/SortBlock.class

Pdapilot/calls.class: Pdapilot/Pref.class

Pdapilot/calls.class: Pdapilot/Char4.class

Pdapilot/calls.class: Pdapilot/Record.class

Pdapilot/calls.class: Pdapilot/RecordID.class

Pdapilot/calls.class: Pdapilot/Resource.class

Pdapilot/calls.class: Pdapilot/CardInfo.class

Pdapilot/calls.class: Pdapilot/UserInfo.class

Pdapilot/calls.class: Pdapilot/SysInfo.class

Pdapilot/calls.class: Pdapilot/NetInfo.class

Pdapilot/calls.class: Pdapilot/DBInfo.class


# Rules for Pdapilot.NoCreatorException
#
Pdapilot/NoCreatorException.class: Pdapilot/NoCreatorException.java
	$(JAVACOMPILE) Pdapilot/NoCreatorException.java

Pdapilot/NoCreatorException.class: Pdapilot/DlpException.class


# Rules for Pdapilot.constants
#
Pdapilot/constants.class: Pdapilot/constants.java
	$(JAVACOMPILE) Pdapilot/constants.java


# Rules for Pdapilot.CancelSyncException
#
Pdapilot/CancelSyncException.class: Pdapilot/CancelSyncException.java
	$(JAVACOMPILE) Pdapilot/CancelSyncException.java

Pdapilot/CancelSyncException.class: Pdapilot/DlpException.class


# Rules for Pdapilot.DB Pdapilot.Dlp Pdapilot.Socket 
#
Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/DB.java Pdapilot/Dlp.java Pdapilot/Socket.java 
	$(JAVACOMPILE) Pdapilot/DB.java Pdapilot/Dlp.java Pdapilot/Socket.java 

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/Database.class Pdapilot/memo/Database.class 

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/DlpException.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/AppBlock.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/SortBlock.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/Record.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/RecordID.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/Resource.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/Char4.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/Pref.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/NoCreatorException.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/CardInfo.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/UserInfo.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/SysInfo.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/NetInfo.class

Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class : Pdapilot/DBInfo.class


# Rules for test
#
test.class: test.java
	$(JAVACOMPILE) test.java

test.class: Pdapilot/DlpException.class

test.class: Pdapilot/CardInfo.class

test.class: Pdapilot/DBInfo.class

test.class: Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class 

test.class: Pdapilot/todo/Database.class

test.class: Pdapilot/RecordID.class

test.class: Pdapilot/Record.class



CLASSES = \
	Pdapilot/Block.class \
	Pdapilot/RecordID.class \
	Pdapilot/Record.class \
	Pdapilot/Char4.class \
	Pdapilot/Resource.class \
	Pdapilot/Pref.class \
	Pdapilot/AppBlock.class \
	Pdapilot/SortBlock.class \
	Pdapilot/memo/AppBlock.class \
	Pdapilot/memo/Record.class \
	Pdapilot/Database.class Pdapilot/memo/Database.class  \
	Pdapilot/todo/AppBlock.class \
	Pdapilot/Util.class \
	Pdapilot/todo/Record.class \
	Pdapilot/todo/Database.class \
	Pdapilot/DBInfo.class \
	Pdapilot/DlpException.class \
	Pdapilot/CardInfo.class \
	Pdapilot/UserInfo.class \
	Pdapilot/SysInfo.class \
	Pdapilot/NetInfo.class \
	Pdapilot/calls.class \
	Pdapilot/NoCreatorException.class \
	Pdapilot/constants.class \
	Pdapilot/CancelSyncException.class \
	Pdapilot/DB.class Pdapilot/Dlp.class Pdapilot/Socket.class  \
	test.class \
# end of classes list
