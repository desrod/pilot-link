c_headers = \
	pi-address.h		\
	pi-appinfo.h		\
	pi-args.h		\
	pi-cmp.h		\
	pi-datebook.h		\
	pi-debug.h		\
	pi-dlp.h		\
	pi-expense.h		\
	pi-file.h		\
	pi-header.h		\
	pi-hinote.h		\
	pi-iambicExpense.h	\
	pi-inet.h		\
	pi-macros.h		\
	pi-mail.h		\
	pi-md5.h		\
	pi-memo.h		\
	pi-money.h		\
	pi-net.h		\
	pi-padp.h		\
	pi-serial.h		\
	pi-slp.h		\
	pi-sockaddr.h		\
	pi-socket.h		\
	pi-source.h		\
	pi-sync.h		\
	pi-sys.h		\
	pi-syspkt.h		\
	pi-todo.h		\
	pi-util.h		\
	pi-version.h

c_privheaders = 		\
	pi-version.h.in

c_header_DATA = $(c_headers)
c_headerdir=$(includedir)

cxx_header_DATA = \
	pi-address.hxx		\
	pi-appinfo.hxx		\
	pi-datebook.hxx		\
	pi-dlp.hxx		\
	pi-iambicExpense.hxx	\
	pi-memo.hxx		\
	pi-todo.hxx
if WITH_CXX
cxx_headerdir=$(includedir)
endif


EXTRA_DIST = 			\
	$(c_headers)		\
	$(c_privheaders)	\
	$(cxx_header_DATA)

