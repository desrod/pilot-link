SUBDIRS=prc pix

if ENABLE_CONDUITS

if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

INCLUDES = 			\
	-I/usr/include		\
	-I$(top_srcdir)/include \
	$(POPT_INCLUDES)	\
	$(PNG_CFLAGS)		\
	$(TCL_INCLUDES)		\
	$(RL_CFLAGS)

# Pedantic CFLAGS -std=c99 -g -Wshadow -Wall 		\
#	-W -Waggregate-return -Wcast-align 		\
#	-Wcast-qual -Wnested-externs 			\
#	-Wpointer-arith -Wundef -Wbad-function-cast 	\
#	-Winline -pedantic -O0

# Commented out for now, will fix later
#if WITH_CXX
#CCEXECS = ccexample
#else
CCEXECS =
#endif

noinst_LTLIBRARIES = libpiuserland.la

bin_PROGRAMS = 		\
	$(CCEXECS)	\
	debugsh		\
	dlpsh		\
	hinotes		\
	install-datebook\
	install-expenses\
	install-hinote	\
	install-memo	\
	install-netsync	\
	install-todos	\
	install-todo	\
	install-user	\
	memos		\
	money2qif	\
	pi-csd		\
	pi-getram	\
	pi-getrom	\
	pi-getromtoken	\
	pi-nredir	\
	pilot-addresses	\
	pilot-clip	\
	pilot-dedupe	\
	pilot-file	\
	pilot-foto	\
	pilot-schlep	\
	pilot-treofoto	\
	pilot-wav	\
	pilot-xfer	\
	read-expenses	\
	read-ical	\
	read-notepad	\
	read-palmpix	\
	read-screenshot	\
	read-todos	\
	read-veo	\
	reminders

if WITH_PERL
PERLSCRIPTS = 		\
	ietf2datebook	\
	pilot-undelete	\
	sync-plan
else
PERLSCRIPTS =
endif

bin_SCRIPTS =		\
	$(PERLSCRIPTS)

#ccexample_SOURCES = 		\
#	ccexample.cc
#ccexample_LDADD = 		\
#	$(top_builddir)/libpisock++/libpisock++.la	\
#	$(top_builddir)/libpisock/libpisock.la

libpiuserland_la_SOURCES =	\
	plu_args.c		\
	userland.c
libpiuserland_la_LDFLAGS =	\
	-static

debugsh_SOURCES = 		\
	debugsh.c
debugsh_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

dlpsh_SOURCES = 		\
	dlpsh.c
dlpsh_LDADD = 			\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(RL_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

#getrom_SOURCES = 		\
#	getrom.c
#getrom_LDADD = 		\
#	$(top_builddir)/libpisock/libpisock.la

hinotes_SOURCES = 		\
	hinotes.c
hinotes_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_datebook_SOURCES = 	\
	install-datebook.c 	\
	parsedate.y
install_datebook_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_expenses_SOURCES = 	\
	install-expenses.c
install_expenses_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_hinote_SOURCES = 	\
	install-hinote.c
install_hinote_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_memo_SOURCES = 		\
	install-memo.c
install_memo_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_netsync_SOURCES = 	\
	install-netsync.c
install_netsync_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_todos_SOURCES = 	\
	install-todos.c
install_todos_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_todo_SOURCES =		\
        install-todo.c  
install_todo_LDADD =            \
	libpiuserland.la	\
	$(POPT_LIBS)		\
        $(top_builddir)/libpisock/libpisock.la

install_user_SOURCES = 		\
	install-user.c
install_user_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

memos_SOURCES = 		\
	memos.c
memos_LDADD = 			\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

money2qif_SOURCES = 		\
	money2qif.c
money2qif_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pi_csd_SOURCES = 		\
	pi-csd.c
pi_csd_LDADD = 			\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pi_getram_SOURCES = 		\
	pi-getram.c
pi_getram_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pi_getrom_SOURCES = 		\
	pi-getrom.c
pi_getrom_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pi_getromtoken_SOURCES = 	\
	pi-getromtoken.c
pi_getromtoken_LDADD =		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pi_nredir_SOURCES = 		\
	pi-nredir.c
pi_nredir_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_addresses_SOURCES = 	\
	pilot-addresses.c
pilot_addresses_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_clip_SOURCES = 		\
	pilot-clip.c
pilot_clip_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_dedupe_SOURCES = 		\
	pilot-dedupe.c
pilot_dedupe_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_file_SOURCES = 		\
	pilot-file.c
pilot_file_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_foto_SOURCES = 		\
	pilot-foto.c
pilot_foto_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_schlep_SOURCES = 		\
	pilot-schlep.c
pilot_schlep_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_treofoto_SOURCES = 		\
	pilot-treofoto.c
pilot_treofoto_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_wav_SOURCES = 		\
	pilot-wav.c
pilot_wav_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_xfer_SOURCES = 		\
	pilot-xfer.c
pilot_xfer_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

read_expenses_SOURCES = 	\
	read-expenses.c
read_expenses_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

read_ical_SOURCES = 		\
	read-ical.c
read_ical_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

read_notepad_SOURCES = 		\
	read-notepad.c
read_notepad_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

read_palmpix_SOURCES = 		\
	read-palmpix.c
read_palmpix_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

read_screenshot_SOURCES =	\
        read-screenshot.c
read_screenshot_LDADD =		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
        $(PNG_LIBS)		\
        $(top_builddir)/libpisock/libpisock.la

read_todos_SOURCES = 		\
	read-todos.c
read_todos_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

read_veo_SOURCES = 		\
	read-veo.c
read_veo_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

reminders_SOURCES = 		\
	reminders.c
reminders_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

ietf2datebook: ietf2datebook.PL
	$(PERL) $< > $@
	chmod +x $@

pilot-undelete: pilot-undelete.PL
	$(PERL) $< > $@
	chmod +x $@

sync-plan: sync-plan.PL
	$(PERL) $< > $@
	chmod +x $@

CLEANFILES = $(PERLSCRIPTS)
endif

EXTRA_DIST = 			\
	ietf2datebook.PL	\
	pilot-undelete.PL	\
	sync-plan.PL		\
	ccexample.cc		\
	debugsh.c		\
	dlpsh.c			\
	hinotes.c		\
	install-datebook.c	\
	install-expenses.c	\
	install-hinote.c	\
	install-memo.c		\
	install-netsync.c	\
	install-todo.c		\
	install-todos.c		\
	install-user.c		\
	kittykiller.c		\
	memos.c			\
	money2qif.c		\
	parsedate.y		\
	pd-tty.c		\
	pi-csd.c		\
	pi-getram.c		\
	pi-getrom.c		\
	pi-getromtoken.c	\
	pi-nredir.c		\
	pi-port.c		\
	pilot-addresses.c	\
	pilot-clip.c		\
	pilot-debug.c		\
	pilot-dedupe.c		\
	pilot-file.c		\
	pilot-foto.c		\
	pilot-schlep.c		\
	pilot-treofoto.c	\
	pilot-wav.c		\
	pilot-xfer.c		\
	read-expenses.c		\
	read-ical.c		\
	read-notepad.c		\
	read-palmpix.c		\
	read-screenshot.c	\
	read-todos.c		\
	read-veo.c		\
	reminders.c		\
	validate.cc

# Magic to update Makefile from Makefile.am
$(srcdir)/Makefile.in : $(srcdir)/Makefile.am
	( cd $(top_srcdir) && automake src/Makefile )

