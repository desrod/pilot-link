SUBDIRS=prc pix

if ENABLE_CONDUITS

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

noinst_LTLIBRARIES = libpiuserland.la

bin_PROGRAMS =			\
	install-datebook	\
	install-expenses	\
	install-hinote		\
	install-memo		\
	install-netsync		\
	install-todo		\
	install-todos		\
	install-user		\
	pilot-addresses		\
	pilot-clip		\
	pilot-csd		\
	pilot-debugsh		\
	pilot-dedupe		\
	pilot-dlpsh		\
	pilot-file		\
	pilot-foto		\
	pilot-getram		\
	pilot-getrom		\
	pilot-getromtoken	\
	pilot-hinotes		\
	pilot-memos		\
	pilot-nredir		\
	pilot-reminders		\
	pilot-schlep		\
	pilot-treofoto		\
	pilot-wav		\
	pilot-xfer		\
	read-expenses		\
	read-ical		\
	read-notepad		\
	read-palmpix		\
	read-screenshot		\
	read-todos		\
	read-veo

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

#getrom_SOURCES = 		\
#	getrom.c
#getrom_LDADD = 		\
#	$(top_builddir)/libpisock/libpisock.la

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

pilot_memos_SOURCES = 		\
	pilot-memos.c
pilot_memos_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_csd_SOURCES = 		\
	pilot-csd.c
pilot_csd_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_debugsh_SOURCES = 	\
	pilot-debugsh.c
pilot_debugsh_LDADD = 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_dlpsh_SOURCES = 		\
	pilot-dlpsh.c
pilot_dlpsh_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(RL_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_getram_SOURCES = 		\
	pilot-getram.c
pilot_getram_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_getrom_SOURCES = 		\
	pilot-getrom.c
pilot_getrom_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_getromtoken_SOURCES = 	\
	pilot-getromtoken.c
pilot_getromtoken_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_hinotes_SOURCES = 	\
	pilot-hinotes.c
pilot_hinotes_LDADD = 		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_nredir_SOURCES = 		\
	pilot-nredir.c
pilot_nredir_LDADD = 		\
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

pilot_reminders_SOURCES =	\
	pilot-reminders.c
pilot_reminders_LDADD = 	\
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
	pilot-debugsh.c		\
	pilot-dlpsh.c		\
	pilot-hinotes.c		\
	install-datebook.c	\
	install-expenses.c	\
	install-hinote.c	\
	install-memo.c		\
	install-netsync.c	\
	install-todo.c		\
	install-todos.c		\
	install-user.c		\
	kittykiller.c		\
	pilot-memos.c		\
	parsedate.y		\
	pd-tty.c		\
	pilot-csd.c		\
	pilot-getram.c		\
	pilot-getrom.c		\
	pilot-getromtoken.c	\
	pilot-nredir.c		\
	pilot-addresses.c	\
	pilot-clip.c		\
	pilot-debug.c		\
	pilot-dedupe.c		\
	pilot-file.c		\
	pilot-foto.c		\
	pilot-port.c		\
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
	pilot-reminders.c	\
	validate.cc

# Magic to update Makefile from Makefile.am
$(srcdir)/Makefile.in : $(srcdir)/Makefile.am
	( cd $(top_srcdir) && automake src/Makefile )

