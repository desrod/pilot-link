if WITH_CXX
CXX_DIR = libcc
else
CXX_DIR =
endif

SUBDIRS=libsock $(CXX_DIR) include man tests tools

INCLUDES = 			\
	-I$(top_srcdir)/include \
	$(RL_CFLAGS)

if WITH_CXX
CCEXECS = ccexample iambicexample
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
	read-todos	\
	reminders

if WITH_PERL5
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
ccexample_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libcc/libpicc.a		\
	$(top_builddir)/libsock/libpisock.la

iambicexample_SOURCES = \
	iambicexample.cc
iambicexample_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libcc/libpicc.a		\
	$(top_builddir)/libsock/libpisock.la

addresses_SOURCES = \
	addresses.c
addresses_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

debugsh_SOURCES = \
	debugsh.c
debugsh_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

dlpsh_SOURCES = \
	dlpsh.c
dlpsh_LDADD = 					\
	@LIBOBJS@				\
	$(RL_LIBS)				\
	$(top_builddir)/libsock/libpisock.la

#getrom_SOURCES = \
#	getrom.c
#getrom_LDADD = 					\
#	@LIBOBJS@				\
#	$(top_builddir)/libsock/libpisock.la

hinotes_SOURCES = \
	hinotes.c
hinotes_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_datebook_SOURCES = \
	install-datebook.c \
	parsedate.y
install_datebook_LDADD = 			\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_expenses_SOURCES = \
	install-expenses.c
install_expenses_LDADD = 			\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_hinote_SOURCES = \
	install-hinote.c
install_hinote_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_memo_SOURCES = \
	install-memo.c
install_memo_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_netsync_SOURCES = \
	install-netsync.c
install_netsync_LDADD = 			\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_todos_SOURCES = \
	install-todos.c
install_todos_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

install_user_SOURCES = \
	install-user.c
install_user_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

memos_SOURCES = \
	memos.c
memos_LDADD = 					\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

money2qif_SOURCES = \
	money2qif.c
money2qif_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pi_csd_SOURCES = \
	pi-csd.c
pi_csd_LDADD = 					\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pi_getram_SOURCES = \
	pi-getram.c
pi_getram_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pi_getrom_SOURCES = \
	pi-getrom.c
pi_getrom_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pi_nredir_SOURCES = \
	pi-nredir.c
pi_nredir_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

#pi_port_SOURCES = \
#	pi-port.c
#pi_port_LDADD = 				\
#	@LIBOBJS@				\
#	$(top_builddir)/libsock/libpisock.la

pilot_addresses_SOURCES = \
	pilot-addresses.c
pilot_addresses_LDADD = 			\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_clip_SOURCES = \
	pilot-clip.c
pilot_clip_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_debug_SOURCES = 	\
	pd-tty.c	\
	pilot-debug.c
pilot_debug_LDADD = 				\
	@LIBOBJS@				\
	$(TCLTKLIBS)				\
	$(RL_LIBS)				\
	$(top_builddir)/libsock/libpisock.la

pilot_dedupe_SOURCES = \
	pilot-dedupe.c
pilot_dedupe_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_file_SOURCES = \
	pilot-file.c
pilot_file_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_mail_SOURCES = 	\
	parsedate.y	\
	pilot-mail.c
pilot_mail_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_prc_SOURCES = \
	pilot-prc.c
pilot_prc_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_schlep_SOURCES = \
	pilot-schlep.c
pilot_schlep_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

pilot_xfer_SOURCES = \
	pilot-xfer.c
pilot_xfer_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

read_expenses_SOURCES = \
	read-expenses.c
read_expenses_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

read_ical_SOURCES = \
	read-ical.c
read_ical_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

read_todos_SOURCES = \
	read-todos.c
read_todos_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

reminders_SOURCES = \
	reminders.c
reminders_LDADD = 				\
	@LIBOBJS@				\
	$(top_builddir)/libsock/libpisock.la

ietf2datebook: ietf2datebook.PL
	$(PERL5) $< > $@
	chmod +x $@

pilot-undelete: pilot-undelete.PL
	$(PERL5) $< > $@
	chmod +x $@

sync-plan: sync-plan.PL
	$(PERL5) $< > $@
	chmod +x $@

prc_DATA = getrom.prc getrom2.prc
prcdir = $(datadir)/pilot-link

m4dir = $(datadir)/aclocal
m4_DATA = pilot-link.m4

CLEANFILES = $(PERLSCRIPTS)

EXTRA_DIST = 			\
	getopt.h		\
	$(prc_DATA)		\
	$(m4_DATA)		\
	ietf2datebook.PL	\
	pilot-undelete.PL	\
	sync-plan.PL

