if WITH_CXX
CXX_DIR = libpisock++
else
CXX_DIR =
endif

SUBDIRS=prc pix pilot-datebook

INCLUDES = 			\
	-Iinclude		\
	-I$(top_srcdir)/include \
	$(TCL_INC)		\
	$(TK_INC)		\
	$(RL_CFLAGS)

if WITH_CXX
CCEXECS = ccexample
else
CCEXECS =
endif

if WITH_TK
TKEXECS = pilot-debug
else
TKEXECS =
endif

bin_PROGRAMS = 		\
	$(CCEXECS)	\
	$(TKEXECS)	\
	addresses	\
	debugsh		\
	dlpsh		\
	hinotes		\
	install-datebook\
	install-expenses \
	install-hinote	\
	install-memo	\
	install-netsync	\
	install-todos	\
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
	pilot-mail	\
	pilot-prc	\
	pilot-schlep	\
	pilot-xfer	\
	read-expenses	\
	read-ical	\
	read-notepad	\
	read-palmpix	\
	read-todos	\
	reminders

if WITH_PERL
PERLSCRIPTS = 		\
	ietf2datebook	\
	pilot-undelete	\
	sync-plan
else
PERLSCRIPTS =
endif

bin_SCRIPTS =
	$(PERLSCRIPTS)

ccexample_SOURCES = \
	ccexample.cc
ccexample_LDADD = 					\
	$(top_builddir)/libpisock++/libpisock++.la	\
	$(top_builddir)/libpisock/libpisock.la

addresses_SOURCES = \
	addresses.c
addresses_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

debugsh_SOURCES = \
	debugsh.c
debugsh_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

dlpsh_SOURCES = \
	dlpsh.c
dlpsh_LDADD = 					\
	$(RL_LIBS)				\
	$(top_builddir)/libpisock/libpisock.la

#getrom_SOURCES = \
#	getrom.c
#getrom_LDADD = 				\
#	$(top_builddir)/libpisock/libpisock.la

hinotes_SOURCES = \
	hinotes.c
hinotes_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

install_datebook_SOURCES = \
	install-datebook.c \
	parsedate.y
install_datebook_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

install_expenses_SOURCES = \
	install-expenses.c
install_expenses_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

install_hinote_SOURCES = \
	install-hinote.c
install_hinote_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

install_memo_SOURCES = \
	install-memo.c
install_memo_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

install_netsync_SOURCES = \
	install-netsync.c
install_netsync_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

install_todos_SOURCES = \
	install-todos.c
install_todos_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

install_user_SOURCES = \
	install-user.c
install_user_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

memos_SOURCES = \
	memos.c
memos_LDADD = 					\
	$(top_builddir)/libpisock/libpisock.la

money2qif_SOURCES = \
	money2qif.c
money2qif_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pi_csd_SOURCES = \
	pi-csd.c
pi_csd_LDADD = 					\
	$(top_builddir)/libpisock/libpisock.la

pi_getram_SOURCES = \
	pi-getram.c
pi_getram_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pi_getrom_SOURCES = \
	pi-getrom.c
pi_getrom_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pi_getromtoken_SOURCES = \
	pi-getromtoken.c
pi_getromtoken_LDADD =				\
	$(top_builddir)/libpisock/libpisock.la

pi_nredir_SOURCES = \
	pi-nredir.c
pi_nredir_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

#pi_port_SOURCES = \
#	pi-port.c
#pi_port_LDADD = 				\
#	$(top_builddir)/libpisock/libpisock.la

pilot_addresses_SOURCES = \
	pilot-addresses.c
pilot_addresses_LDADD = 			\
	$(top_builddir)/libpisock/libpisock.la

pilot_clip_SOURCES = \
	pilot-clip.c
pilot_clip_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_debug_SOURCES = 	\
	pd-tty.c	\
	pilot-debug.c
pilot_debug_LDADD = 				\
	$(TCLTKLIBS)				\
	$(RL_LIBS)				\
	$(top_builddir)/libpisock/libpisock.la

pilot_dedupe_SOURCES = \
	pilot-dedupe.c
pilot_dedupe_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_file_SOURCES = \
	pilot-file.c
pilot_file_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_mail_SOURCES = 	\
	parsedate.y	\
	pilot-mail.c
pilot_mail_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_prc_SOURCES = \
	pilot-prc.c
pilot_prc_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_schlep_SOURCES = \
	pilot-schlep.c
pilot_schlep_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

pilot_xfer_SOURCES = \
	pilot-xfer.c
pilot_xfer_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

read_expenses_SOURCES = \
	read-expenses.c
read_expenses_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

read_ical_SOURCES = \
	read-ical.c
read_ical_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

read_notepad_SOURCES = \
	read-notepad.c
read_notepad_LDADD = 				\
	$(PNGLIBS) 				\
	$(top_builddir)/libpisock/libpisock.la

read_palmpix_SOURCES = \
	read-palmpix.c
read_palmpix_LDADD = 				\
	$(PNGLIBS) 				\
	$(top_builddir)/libpisock/libpisock.la

read_todos_SOURCES = \
	read-todos.c
read_todos_LDADD = 				\
	$(top_builddir)/libpisock/libpisock.la

reminders_SOURCES = \
	reminders.c
reminders_LDADD = 				\
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
	ietf2datebook.PL	\
	pilot-undelete.PL	\
	sync-plan.PL

