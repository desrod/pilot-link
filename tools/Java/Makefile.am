all: java_pisock

INCLUDES= 				\
	-I../../include			\
	-I $(top_srcdir)/include 	\
	-I $(top_builddir)/include	\
	-I $(JAVABASE)/include		\
	-I $(JAVABASE)/include/linux

noinst_PROGRAMS = doConstants

java_files= \
	$(srcdir)/org/gnu/pdapilot/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/Block.java \
	$(srcdir)/org/gnu/pdapilot/CancelSyncException.java \
	$(srcdir)/org/gnu/pdapilot/CardInfo.java \
	$(srcdir)/org/gnu/pdapilot/CategoryAppBlock.java \
	$(srcdir)/org/gnu/pdapilot/DB.java \
	$(srcdir)/org/gnu/pdapilot/DBInfo.java \
	$(srcdir)/org/gnu/pdapilot/Database.java \
	$(srcdir)/org/gnu/pdapilot/Dlp.java \
	$(srcdir)/org/gnu/pdapilot/DlpException.java \
	$(srcdir)/org/gnu/pdapilot/File.java \
	$(srcdir)/org/gnu/pdapilot/NetInfo.java \
	$(srcdir)/org/gnu/pdapilot/NoCreatorException.java \
	$(srcdir)/org/gnu/pdapilot/Pref.java \
	$(srcdir)/org/gnu/pdapilot/Record.java \
	$(srcdir)/org/gnu/pdapilot/RecordID.java \
	$(srcdir)/org/gnu/pdapilot/Resource.java \
	$(srcdir)/org/gnu/pdapilot/Socket.java \
	$(srcdir)/org/gnu/pdapilot/SortBlock.java \
	$(srcdir)/org/gnu/pdapilot/SysInfo.java \
	$(srcdir)/org/gnu/pdapilot/UserInfo.java \
	$(srcdir)/org/gnu/pdapilot/address/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/address/Database.java \
	$(srcdir)/org/gnu/pdapilot/address/Record.java \
	$(srcdir)/org/gnu/pdapilot/appointment/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/appointment/Database.java \
	$(srcdir)/org/gnu/pdapilot/appointment/Record.java \
	$(srcdir)/org/gnu/pdapilot/appointment/Time.java \
	$(srcdir)/org/gnu/pdapilot/appointment/Repeat.java \
	$(srcdir)/org/gnu/pdapilot/End.java \
	$(srcdir)/org/gnu/pdapilot/expense/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/expense/CustomCurrency.java \
	$(srcdir)/org/gnu/pdapilot/expense/Database.java \
	$(srcdir)/org/gnu/pdapilot/expense/Pref.java \
	$(srcdir)/org/gnu/pdapilot/expense/Record.java \
	$(srcdir)/org/gnu/pdapilot/expense/Payment.java \
	$(srcdir)/org/gnu/pdapilot/expense/Sort.java \
	$(srcdir)/org/gnu/pdapilot/expense/Type.java \
	$(srcdir)/org/gnu/pdapilot/expense/Distance.java \
	$(srcdir)/org/gnu/pdapilot/mail/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/mail/Database.java \
	$(srcdir)/org/gnu/pdapilot/mail/Record.java \
	$(srcdir)/org/gnu/pdapilot/mail/SignaturePref.java \
	$(srcdir)/org/gnu/pdapilot/mail/SyncPref.java \
	$(srcdir)/org/gnu/pdapilot/mail/Sync.java \
	$(srcdir)/org/gnu/pdapilot/mail/Sort.java \
	$(srcdir)/org/gnu/pdapilot/memo/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/memo/Database.java \
	$(srcdir)/org/gnu/pdapilot/memo/Record.java \
	$(srcdir)/org/gnu/pdapilot/todo/AppBlock.java \
	$(srcdir)/org/gnu/pdapilot/todo/Database.java \
	$(srcdir)/org/gnu/pdapilot/todo/Record.java \
	$(srcdir)/org/gnu/pdapilot/Char4.java \
	$(srcdir)/org/gnu/pdapilot/Util.java \
	$(srcdir)/org/gnu/pdapilot/Constants.java \
	$(srcdir)/org/gnu/pdapilot/Calls.java \
	$(srcdir)/test.java

java_classes= \
	org/gnu/pdapilot/AppBlock.class \
	org/gnu/pdapilot/Block.class \
	org/gnu/pdapilot/CancelSyncException.class \
	org/gnu/pdapilot/CardInfo.class \
	org/gnu/pdapilot/CategoryAppBlock.class \
	org/gnu/pdapilot/DB.class \
	org/gnu/pdapilot/DBInfo.class \
	org/gnu/pdapilot/Database.class \
	org/gnu/pdapilot/Dlp.class \
	org/gnu/pdapilot/DlpException.class \
	org/gnu/pdapilot/File.class \
	org/gnu/pdapilot/NetInfo.class \
	org/gnu/pdapilot/NoCreatorException.class \
	org/gnu/pdapilot/Pref.class \
	org/gnu/pdapilot/Record.class \
	org/gnu/pdapilot/RecordID.class \
	org/gnu/pdapilot/Resource.class \
	org/gnu/pdapilot/Socket.class \
	org/gnu/pdapilot/SortBlock.class \
	org/gnu/pdapilot/SysInfo.class \
	org/gnu/pdapilot/UserInfo.class \
	org/gnu/pdapilot/address/AppBlock.class \
	org/gnu/pdapilot/address/Database.class \
	org/gnu/pdapilot/address/Record.class \
	org/gnu/pdapilot/appointment/AppBlock.class \
	org/gnu/pdapilot/appointment/Database.class \
	org/gnu/pdapilot/appointment/Record.class \
	org/gnu/pdapilot/appointment/Time.class \
	org/gnu/pdapilot/appointment/Repeat.class \
	org/gnu/pdapilot/End.class \
	org/gnu/pdapilot/expense/AppBlock.class \
	org/gnu/pdapilot/expense/CustomCurrency.class \
	org/gnu/pdapilot/expense/Database.class \
	org/gnu/pdapilot/expense/Pref.class \
	org/gnu/pdapilot/expense/Record.class \
	org/gnu/pdapilot/expense/Payment.class \
	org/gnu/pdapilot/expense/Sort.class \
	org/gnu/pdapilot/expense/Type.class \
	org/gnu/pdapilot/expense/Distance.class \
	org/gnu/pdapilot/mail/AppBlock.class \
	org/gnu/pdapilot/mail/Database.class \
	org/gnu/pdapilot/mail/Record.class \
	org/gnu/pdapilot/mail/SignaturePref.class \
	org/gnu/pdapilot/mail/SyncPref.class \
	org/gnu/pdapilot/mail/Sync.class \
	org/gnu/pdapilot/mail/Sort.class \
	org/gnu/pdapilot/memo/AppBlock.class \
	org/gnu/pdapilot/memo/Database.class \
	org/gnu/pdapilot/memo/Record.class \
	org/gnu/pdapilot/todo/AppBlock.class \
	org/gnu/pdapilot/todo/Database.class \
	org/gnu/pdapilot/todo/Record.class \
	org/gnu/pdapilot/Char4.class \
	org/gnu/pdapilot/Util.class \
	org/gnu/pdapilot/Constants.class \
	org/gnu/pdapilot/Calls.class

java_write: doConstants
	test -d org || mkdir org
	test -d org/gnu || mkdir org/gnu
	test -d org/gnu/pdapilot || mkdir org/gnu/pdapilot
	./doConstants

java_compile: $(java_files)
	$(JAVAC) -d . $(java_files)

java_lib: Jni.c
	$(CXX) $(INCLUDES) $(DEFS) -L $(top_builddir)/libsock/.libs -lpisock -shared -o libPiJni.so $(srcdir)/Jni.c

java_jar: $(java_classes)
	$(JAR) cvf pilot-link.jar $(java_classes)

java_pisock: java_write java_compile java_lib java_jar

clean-local:
	rm -f libPiJni.so
	rm -f test.class
	rm -f $(java_classes)
	rm -f pilot-link.jar
