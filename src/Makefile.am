SUBDIRS=prc pix

if ENABLE_CONDUITS

INCLUDES = 			\
	-I/usr/include		\
	-I$(top_srcdir)/include \
	$(POPT_INCLUDES)	\
	$(PNG_CFLAGS)		\
	$(TCL_INCLUDES)		\
	$(RL_CFLAGS)

noinst_LTLIBRARIES = libpiuserland.la

bin_PROGRAMS =			\
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
	pilot-install-datebook	\
	pilot-install-expenses	\
	pilot-install-hinote	\
	pilot-install-memo	\
	pilot-install-netsync	\
	pilot-install-todo	\
	pilot-install-todos	\
	pilot-install-user	\
	pilot-memos		\
	pilot-nredir		\
	pilot-read-expenses	\
	pilot-read-ical		\
	pilot-read-notepad	\
	pilot-read-palmpix	\
	pilot-read-screenshot	\
	pilot-read-todos	\
	pilot-read-veo		\
	pilot-reminders		\
	pilot-schlep		\
	pilot-foto-treo600	\
	pilot-foto-treo650	\
	pilot-wav		\
	pilot-xfer

if WITH_PERL
PERLSCRIPTS =			\
	pilot-ietf2datebook	\
	pilot-undelete		\
	pilot-sync-plan
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

pilot_install_datebook_SOURCES = 	\
	pilot-install-datebook.c 	\
	parsedate.y
pilot_install_datebook_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_install_hinote_SOURCES = 	\
	pilot-install-hinote.c
pilot_install_hinote_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_install_memo_SOURCES = 	\
	pilot-install-memo.c
pilot_install_memo_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_install_todos_SOURCES = 	\
	pilot-install-todos.c
pilot_install_todos_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_install_todo_SOURCES =	\
	pilot-install-todo.c  
pilot_install_todo_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
        $(top_builddir)/libpisock/libpisock.la

pilot_install_user_SOURCES =	\
	pilot-install-user.c
pilot_install_user_LDADD =	\
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

pilot_install_expenses_SOURCES = 	\
	pilot-install-expenses.c
pilot_install_expenses_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_install_netsync_SOURCES =	\
	pilot-install-netsync.c
pilot_install_netsync_LDADD = 	\
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

pilot_read_screenshot_SOURCES =	\
        pilot-read-screenshot.c
pilot_read_screenshot_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
        $(PNG_LIBS)		\
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

pilot_foto_treo600_SOURCES = 	\
	pilot-foto-treo600.c
pilot_foto_treo600_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_foto_treo650_SOURCES = 	\
	pilot-foto-treo650.c
pilot_foto_treo650_LDADD = 	\
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

pilot_read_expenses_SOURCES = 	\
	pilot-read-expenses.c
pilot_read_expenses_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_read_ical_SOURCES = 	\
	pilot-read-ical.c
pilot_read_ical_LDADD = 	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_read_notepad_SOURCES =	\
	pilot-read-notepad.c
pilot_read_notepad_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_read_palmpix_SOURCES =	\
	pilot-read-palmpix.c
pilot_read_palmpix_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

pilot_read_todos_SOURCES =	\
	pilot-read-todos.c
pilot_read_todos_LDADD =	\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(top_builddir)/libpisock/libpisock.la

pilot_read_veo_SOURCES =	\
	pilot-read-veo.c
pilot_read_veo_LDADD =		\
	libpiuserland.la	\
	$(POPT_LIBS)		\
	$(PNG_LIBS) 		\
	$(top_builddir)/libpisock/libpisock.la

pilot-ietf2datebook: pilot-ietf2datebook.pl
	$(PERL) $< > $@
	chmod +x $@

pilot-undelete: pilot-undelete.pl
	$(PERL) $< > $@
	chmod +x $@

pilot-sync-plan: pilot-sync-plan.pl
	$(PERL) $< > $@
	chmod +x $@

CLEANFILES = $(PERLSCRIPTS)
endif

EXTRA_DIST = 				\
	ccexample.cc			\
	pilot-ietf2datebook.pl		\
	kittykiller.c			\
	parsedate.y			\
	pd-tty.c			\
	pilot-addresses.c		\
	pilot-clip.c			\
	pilot-csd.c			\
	pilot-debug.c			\
	pilot-debugsh.c			\
	pilot-dedupe.c			\
	pilot-dlpsh.c			\
	pilot-file.c			\
	pilot-foto.c			\
	pilot-getram.c			\
	pilot-getrom.c			\
	pilot-getromtoken.c		\
	pilot-hinotes.c			\
	pilot-install-datebook.c	\
	pilot-install-expenses		\
	pilot-install-hinote.c		\
	pilot-install-memo.c		\
	pilot-install-netsync.c		\
	pilot-install-todo.c		\
	pilot-install-todos.c		\
	pilot-install-user.c		\
	pilot-memos.c			\
	pilot-nredir.c			\
	pilot-port.c			\
	pilot-read-expenses.c		\
	pilot-read-ical.c		\
	pilot-read-notepad.c		\
	pilot-read-palmpix.c		\
	pilot-read-screenshot.c		\
	pilot-read-todos.c		\
	pilot-read-veo.c		\
	pilot-reminders.c		\
	pilot-schlep.c			\
	pilot-foto-treo600.c		\
	pilot-foto-treo650.c		\
	pilot-undelete.pl		\
	pilot-wav.c			\
	pilot-xfer.c			\
	pilot-sync-plan.pl		\
	validate.cc

# Magic to update Makefile from Makefile.am
$(srcdir)/Makefile.in : $(srcdir)/Makefile.am
	( cd $(top_srcdir) && automake src/Makefile )
