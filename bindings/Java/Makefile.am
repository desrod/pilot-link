PORT=/dev/pilot

all: java_pisock

INCLUDES= 				\
	-I../../include			\
	-I $(top_srcdir)/include 	\
	-I $(top_builddir)/include	\
	-I $(JAVABASE)/include		\
	-I $(JAVABASE)/include/linux

java_files= \
	$(srcdir)/org/gnu/pilotlink/AddressAppInfo.java              \
	$(srcdir)/org/gnu/pilotlink/AddressRecord.java               \
	$(srcdir)/org/gnu/pilotlink/AppInfo.java                     \
	$(srcdir)/org/gnu/pilotlink/DBInfo.java                      \
	$(srcdir)/org/gnu/pilotlink/DatabaseExistsException.java     \
	$(srcdir)/org/gnu/pilotlink/DatabaseNotFoundException.java   \
	$(srcdir)/org/gnu/pilotlink/DatebookAppInfo.java             \
	$(srcdir)/org/gnu/pilotlink/DatebookRecord.java              \
	$(srcdir)/org/gnu/pilotlink/FTB3CostCatRecord.java           \
	$(srcdir)/org/gnu/pilotlink/FTB3TripRecord.java              \
	$(srcdir)/org/gnu/pilotlink/FTB3VehicleRecord.java           \
	$(srcdir)/org/gnu/pilotlink/MemoRecord.java                  \
	$(srcdir)/org/gnu/pilotlink/PilotLink.java                   \
	$(srcdir)/org/gnu/pilotlink/PilotLinkException.java          \
	$(srcdir)/org/gnu/pilotlink/RawAppInfo.java                  \
	$(srcdir)/org/gnu/pilotlink/RawRecord.java                   \
	$(srcdir)/org/gnu/pilotlink/Record.java                      \
	$(srcdir)/org/gnu/pilotlink/SysInfo.java                     \
	$(srcdir)/org/gnu/pilotlink/User.java                        \
	$(srcdir)/test.java

java_classes= \
	org/gnu/pilotlink/AddressAppInfo.class              \
	org/gnu/pilotlink/AddressRecord.class               \
	org/gnu/pilotlink/AppInfo.class                     \
	org/gnu/pilotlink/DBInfo.class                      \
	org/gnu/pilotlink/DatabaseExistsException.class     \
	org/gnu/pilotlink/DatabaseNotFoundException.class   \
	org/gnu/pilotlink/DatebookAppInfo.class             \
	org/gnu/pilotlink/DatebookRecord.class              \
	org/gnu/pilotlink/FTB3CostCatRecord.class           \
	org/gnu/pilotlink/FTB3TripRecord.class              \
	org/gnu/pilotlink/FTB3VehicleRecord.class           \
	org/gnu/pilotlink/MemoRecord.class                  \
	org/gnu/pilotlink/PilotLink.class                   \
	org/gnu/pilotlink/PilotLinkException.class          \
	org/gnu/pilotlink/RawAppInfo.class                  \
	org/gnu/pilotlink/RawRecord.class                   \
	org/gnu/pilotlink/Record.class                      \
	org/gnu/pilotlink/SysInfo.class                     \
	org/gnu/pilotlink/User.class                        

java_compile: $(java_files)
	$(JAVAC) -d . $(java_files)

java_lib: libjpisock.c
	$(CXX) $(INCLUDES) $(DEFS) -L $(top_builddir)/libpisock/.libs -lpisock -shared -o libjpisock.so $(srcdir)/libjpisock.c

java_jar: $(java_classes)
	$(JAR) cvf pilot-link.jar $(java_classes)

java_pisock: java_compile java_lib java_jar

install: all
	cp libjpisock.so ${JAVABASE}/jre/lib/i386/

test: install
	${JAVABASE}/bin/java test ${PORT}

clean-local:
	rm -f libjpisock.so
	rm -f test.class
	rm -f $(java_classes)
	rm -f pilot-link.jar
