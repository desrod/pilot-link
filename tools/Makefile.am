if WITH_CCLIB
CC_TARGETS = ccexample iambicexample validate
else
CC_TARGETS =
endif

INCLUDES = -I$(top_srcdir)/libsock -I$(top_srcdir)/libcc

LIBPISOCK = ../libsock/libpisock.la
LIBPICC = ../libcc/libpicc.la

bin_PROGRAMS = \
	addresses 	\
	debugsh		\
	dlpsh 		\
	getrom 		\
	hinotes		\
	install-datebook\
	install-hinote 	\
	install-memo 	\
	install-todos	\
	install-user 	\
	memos 		\
	money2qif 	\
	pilot-addresses	\
	pilot-clip	\
	pilot-dedupe 	\
	pilot-file 	\
	pilot-mail 	\
	pilot-schlep 	\
	pilot-xfer 	\
	pi-csd		\
	pi-getram 	\
	pi-getrom 	\
	pi-nredir 	\
	pi-port 	\
	read-expenses	\
	read-ical 	\
	read-todos	\
	reminders 	\
	$(CC_TARGETS)

addresses_SOURCES = addresses.c
addresses_LDADD = $(LIBPISOCK)

debugsh_SOURCES = debugsh.c
debugsh_LDADD = $(LIBPISOCK)

dlpsh_SOURCES = dlpsh.c
dlpsh_LDADD = $(LIBPISOCK)

getrom_SOURCES = getrom.c
getrom_LDADD = $(LIBPISOCK)

hinotes_SOURCES = hinotes.c
hinotes_LDADD = $(LIBPISOCK)

install_datebook_SOURCES = install-datebook.c parsedate.y
install_datebook_LDADD = $(LIBPISOCK)

install_hinote_SOURCES = install-hinote.c
install_hinote_LDADD = $(LIBPISOCK)

install_memo_SOURCES = install-memo.c
install_memo_LDADD = $(LIBPISOCK)

install_todos_SOURCES = install-todos.c
install_todos_LDADD = $(LIBPISOCK)

install_user_SOURCES = install-user.c
install_user_LDADD = $(LIBPISOCK)

memos_SOURCES = memos.c
memos_LDADD = $(LIBPISOCK)

money2qif_SOURCES = money2qif.c
money2qif_LDADD = $(LIBPISOCK)

pilot_addresses_SOURCES = pilot-addresses.c
pilot_addresses_LDADD = $(LIBPISOCK)

pilot_clip_SOURCES = pilot-clip.c
pilot_clip_LDADD = $(LIBPISOCK)

pilot_dedupe_SOURCES = pilot-dedupe.c
pilot_dedupe_LDADD = $(LIBPISOCK)

pilot_file_SOURCES = pilot-file.c
pilot_file_LDADD = $(LIBPISOCK)

pilot_mail_SOURCES = pilot-mail.c parsedate.y
pilot_mail_LDADD = $(LIBPISOCK)

pilot_schlep_SOURCES = pilot-schlep.c
pilot_schlep_LDADD = $(LIBPISOCK)

pilot_xfer_SOURCES = pilot-xfer.c
pilot_xfer_LDADD = $(LIBPISOCK)

pi_csd_SOURCES = pi-csd.c
pi_csd_LDADD = $(LIBPISOCK)

pi_getram_SOURCES = pi-getram.c
pi_getram_LDADD = $(LIBPISOCK)

pi_getrom_SOURCES = pi-getrom.c
pi_getrom_LDADD = $(LIBPISOCK)

pi_nredir_SOURCES = pi-nredir.c
pi_nredir_LDADD = $(LIBPISOCK)

pi_port_SOURCES = pi-port.c
pi_port_LDADD = $(LIBPISOCK)

read_expenses_SOURCES = read-expenses.c
read_expenses_LDADD = $(LIBPISOCK)

read_ical_SOURCES = read-ical.c
read_ical_LDADD = $(LIBPISOCK)

read_todos_SOURCES = read-todos.c
read_todos_LDADD = $(LIBPISOCK)

reminders_SOURCES = reminders.c
reminders_LDADD = $(LIBPISOCK)

if WITH_CCLIB
ccexample_SOURCES = ccexample.cc
ccexample_LDADD = $(LIBPISOCK) $(LIBPICC)

iambicexample_SOURCES = iambicexample.cc
iambicexample_LDADD = $(LIBPISOCK) $(LIBPICC)

validate_SOURCES = validate.cc
validate_LDADD = $(LIBPISOCK) $(LIBPICC)
endif
