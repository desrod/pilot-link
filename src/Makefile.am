if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

SUBDIRS=prc pix pilot-datebook

INCLUDES = 			\
	-I/usr/include		\
	-I$(top_srcdir)/include \
	$(POPT_FLAGS)		\
	$(PNG_CFLAGS)		\
	$(TCL_INCLUDES)		\
	$(RL_CFLAGS)

# Commented out for now, will fix later
#if WITH_CXX
#CCEXECS = ccexample
#else
CCEXECS =
#endif

bin_PROGRAMS = 		\
	$(CCEXECS)	\
	addresses	\
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
	pilot-archive	\
	pilot-clip	\
	pilot-dedupe	\
	pilot-file	\
	pilot-foto	\
	pilot-prc	\
	pilot-schlep	\
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

#
# Add in the getopt sources if we need them
#
if NEED_GETOPT
getopt_sources = getopt.h getopt.c getopt1.c
else
getopt_sources = 
endif

#ccexample_SOURCES = 		\
#	$(getopt_sources)	\
#	ccexample.cc
#ccexample_LDADD = 		\
#	$(top_builddir)/libpisock++/libpisock++.la	\
#	$(top_builddir)/libpisock/libpisock.la

addresses_SOURCES = 		\
	addresses.c
addresses_LDADD = 		\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

debugsh_SOURCES = 		\
	debugsh.c
debugsh_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

dlpsh_SOURCES = 		\
	$(getopt_sources)	\
	dlpsh.c
dlpsh_LDADD = 			\
	$(RL_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

#getrom_SOURCES = 		\
#	getrom.c
#getrom_LDADD = 		\
#	$(top_builddir)/libpisock/libpisock.la

hinotes_SOURCES = 		\
	$(getopt_sources)	\
	hinotes.c
hinotes_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

install_datebook_SOURCES = 	\
	$(getopt_sources)	\
	install-datebook.c 	\
	parsedate.y
install_datebook_LDADD = 	\
	$(top_builddir)/libpisock/libpisock.la

install_expenses_SOURCES = 	\
	$(getopt_sources)	\
	install-expenses.c
install_expenses_LDADD = 	\
	$(top_builddir)/libpisock/libpisock.la

install_hinote_SOURCES = 	\
	$(getopt_sources)	\
	install-hinote.c
install_hinote_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

install_memo_SOURCES = 		\
	$(getopt_sources)	\
	install-memo.c
install_memo_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

install_netsync_SOURCES = 	\
	install-netsync.c
install_netsync_LDADD = 	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

install_todos_SOURCES = 	\
	$(getopt_sources)	\
	install-todos.c
install_todos_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

install_todo_SOURCES =		\
        $(getopt_sources)       \
        install-todo.c  
install_todo_LDADD =            \
        $(top_builddir)/libpisock/libpisock.la

install_user_SOURCES = 		\
	install-user.c
install_user_LDADD = 		\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

memos_SOURCES = 		\
	$(getopt_sources)	\
	memos.c
memos_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

money2qif_SOURCES = 		\
	$(getopt_sources)	\
	money2qif.c
money2qif_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pi_csd_SOURCES = 		\
	$(getopt_sources)	\
	pi-csd.c
pi_csd_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

pi_getram_SOURCES = 		\
	$(getopt_sources)	\
	pi-getram.c
pi_getram_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pi_getrom_SOURCES = 		\
	$(getopt_sources)	\
	pi-getrom.c
pi_getrom_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pi_getromtoken_SOURCES = 	\
	$(getopt_sources)	\
	pi-getromtoken.c
pi_getromtoken_LDADD =		\
	$(top_builddir)/libpisock/libpisock.la

pi_nredir_SOURCES = 		\
	$(getopt_sources)	\
	pi-nredir.c
pi_nredir_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

#pi_port_SOURCES = 		\
#	$(getopt_sources)	\
#	pi-port.c
#pi_port_LDADD = 		\
#	$(top_builddir)/libpisock/libpisock.la

pilot_addresses_SOURCES = 	\
	$(getopt_sources)	\
	pilot-addresses.c
pilot_addresses_LDADD = 	\
	$(top_builddir)/libpisock/libpisock.la

pilot_archive_SOURCES =		\
        $(getopt_sources)       \
        pilot-archive.c
pilot_archive_LDADD =           \
        $(top_builddir)/libpisock/libpisock.la

pilot_clip_SOURCES = 		\
	$(getopt_sources)	\
	pilot-clip.c
pilot_clip_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

#pilot_debug_SOURCES = 		\
#	pd-tty.c		\
#	pilot-debug.c
#pilot_debug_LDADD = 		\
#	$(TCL_LIBS)		\
#	$(TCL_LIB_FLAG)		\
#	$(TCL_INCLUDES)		\
#	$(RL_LIBS)		\
#	$(top_builddir)/libpisock/libpisock.la

pilot_dedupe_SOURCES = 		\
	$(getopt_sources)	\
	pilot-dedupe.c
pilot_dedupe_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_file_SOURCES = 		\
	$(getopt_sources)	\
	pilot-file.c
pilot_file_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_foto_SOURCES = 		\
	$(getopt_sources)	\
	pilot-foto.c
pilot_foto_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

######################################################
# pilot-mail has been deprecated, and replaced with 
# pilot-mailsync. Please use that instead. 
# http://wissrech.iam.uni-bonn.de/people/garcke/pms/
######################################################
#pilot_mail_SOURCES = 		\
#	$(getopt_sources)	\
#	parsedate.y		\
#	pilot-mail.c
#pilot_mail_LDADD = 		\
#	$(top_builddir)/libpisock/libpisock.la
######################################################

pilot_prc_SOURCES = 		\
	$(getopt_sources)	\
	pilot-prc.c
pilot_prc_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_schlep_SOURCES = 		\
	$(getopt_sources)	\
	pilot-schlep.c
pilot_schlep_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_wav_SOURCES = 		\
	$(getopt_sources)	\
	pilot-wav.c
pilot_wav_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_xfer_SOURCES = 		\
	$(getopt_sources)	\
	pilot-xfer.c
pilot_xfer_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

read_expenses_SOURCES = 	\
	$(getopt_sources)	\
	read-expenses.c
read_expenses_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

read_ical_SOURCES = 		\
	$(getopt_sources)	\
	read-ical.c
read_ical_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

read_notepad_SOURCES = 		\
	$(getopt_sources)	\
	read-notepad.c
read_notepad_LDADD = 		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

read_palmpix_SOURCES = 		\
	$(getopt_sources)	\
	read-palmpix.c
read_palmpix_LDADD = 		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

read_screenshot_SOURCES =	\
        $(getopt_sources)	\
        read-screenshot.c
read_screenshot_LDADD =		\
	$(POPT_LIBS)		\
        $(PNG_LIBS)		\
        $(top_builddir)/libpisock/libpisock.la

read_todos_SOURCES = 		\
	$(getopt_sources)	\
	read-todos.c
read_todos_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

read_veo_SOURCES = 		\
	$(getopt_sources)	\
	read-veo.c
read_veo_LDADD = 		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

reminders_SOURCES = 		\
	$(getopt_sources)	\
	reminders.c
reminders_LDADD = 		\
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

EXTRA_DIST = 			\
	getopt.h		\
	getopt.c		\
	getopt1.c		\
	ietf2datebook.PL	\
	pilot-undelete.PL	\
	sync-plan.PL
