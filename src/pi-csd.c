/*
 * pi-csd.c: Connection Service Daemon, required for accepting logons via
 *           NetSync(tm)
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-serial.h"
#include "pi-slp.h"
#include "pi-header.h"
#include "pi-userland.h"

char hostname[130];
struct in_addr address, netmask;

#ifdef HAVE_SA_LEN
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define ifreq_size(i) max(sizeof(struct ifreq),\
     sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
#else
#define ifreq_size(i) sizeof(struct ifreq)
#endif				/* HAVE_SA_LEN */

/* What, me worry? */
#ifndef IFF_POINTOPOINT
# ifdef IFF_POINTTOPOINT
#  define IFF_POINTOPOINT IFF_POINTTOPOINT
# endif
#endif


/***********************************************************************
 *
 * Function:    fetch_host
 *
 * Summary:     Retrieve the networking information from the host
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
fetch_host(char *hostname, size_t hostlen, struct in_addr *address,
	   struct in_addr *mask)
{
	int 	i,
		n,
		s;

	struct 	ifconf ifc;
	struct 	ifreq *ifr, ifreqaddr, ifreqmask;
	struct 	hostent *hent;

#ifdef HAVE_GETHOSTNAME
	/* Get host name the easy way */
	gethostname(hostname, hostlen);
#else
#ifdef HAVE_UNAME
	struct 	utsname uts;

	if (uname(&uts) == 0) {
		strncpy(hostname, uts.nodename, hostlen - 1);
		hostname[hostlen - 1] = '\0';
	}
#endif				/* def HAVE_UNAME */
#endif				/* def HAVE_GETHOSTNAME */

	/* Get host address through DNS */
	hent = gethostbyname(hostname);

	if (hent) {
		while (*hent->h_addr_list) {
			struct in_addr haddr;

			memcpy(&haddr, *(hent->h_addr_list++),
			       sizeof(haddr));
			if (haddr.s_addr != inet_addr("127.0.0.1"))
				memcpy(address, &haddr, sizeof(haddr));
		}
	}
#if defined(SIOCGIFCONF) && defined(SIOCGIFFLAGS)
	s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0)
		return;

	ifc.ifc_buf = calloc(1024, 1);
	ifc.ifc_len = 1024;

	if (ioctl(s, SIOCGIFCONF, (char *) &ifc) < 0)
		goto done;

	n = ifc.ifc_len;
	for (i = 0; i < n; i += ifreq_size(*ifr)) {
		struct sockaddr_in *a;
		struct sockaddr_in *b;

		ifr 	= (struct ifreq *) ((caddr_t) ifc.ifc_buf + i);
		a 	= (struct sockaddr_in *) &ifr->ifr_addr;

		strncpy(ifreqaddr.ifr_name, ifr->ifr_name,
			sizeof(ifreqaddr.ifr_name));

		strncpy(ifreqmask.ifr_name, ifr->ifr_name,
			sizeof(ifreqmask.ifr_name));

		if (ioctl(s, SIOCGIFFLAGS, (char *) &ifreqaddr) < 0)
			continue;

		/* Reject loopback device */
#ifdef IFF_LOOPBACK
		if (ifreqaddr.ifr_flags & IFF_LOOPBACK)
			continue;
#endif				/* def IFF_LOOPBACK */

#ifdef IFF_UP
		/* Reject down devices */
		if (!(ifreqaddr.ifr_flags & IFF_UP))
			continue;
#endif				/* def IFF_UP */

		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;

		/* If it is a point-to-point device, use the dest address */
#if defined(IFF_POINTOPOINT) && defined(SIOCGIFDSTADDR)
		if (ifreqaddr.ifr_flags & IFF_POINTOPOINT) {
			if (ioctl(s, SIOCGIFDSTADDR, (char *) &ifreqaddr) <
			    0)
				break;

			a = (struct sockaddr_in *) &ifreqaddr.ifr_dstaddr;

			if (address->s_addr == 0) {
				memcpy(address, &a->sin_addr,
				       sizeof(struct in_addr));
			}
		} else
#endif			/* defined(IFF_POINTOPOINT) && defined(SIOCGIFDSTADDR) */
			/* If it isn't a point-to-point device, use the address */
#ifdef SIOCGIFADDR
		{
			if (ioctl(s, SIOCGIFADDR, (char *) &ifreqaddr) < 0)
				break;

			a = (struct sockaddr_in *) &ifreqaddr.ifr_addr;

			if (address->s_addr == 0) {
				memcpy(address, &a->sin_addr,
				       sizeof(struct in_addr));
			}
		}
#endif				/* def SIOCGIFADDR */
		/* OK, we've got an address */

		/* Compare netmask against the current address and see if it
		   seems to match. */
#ifdef SIOCGIFNETMASK
		if (ioctl(s, SIOCGIFNETMASK, (char *) &ifreqmask) < 0)
			break;

/* Is there any system where we need to use ifr_netmask?  */
#if 1
		b = (struct sockaddr_in *) &ifreqmask.ifr_addr;
#else
		b = (struct sockaddr_in *) &ifreqmask.ifr_netmask;
#endif

		if ((mask->s_addr == 0) && (address->s_addr != 0)) {
			if ((b->sin_addr.s_addr & a->sin_addr.s_addr) ==
			    (b->sin_addr.s_addr & address->s_addr)) {
				memcpy(mask, &b->sin_addr,
				       sizeof(struct in_addr));

				/* OK, we've got a netmask */

				break;
			}
		}
#endif				/* def SIOCGIFNETMASK */

	}

      done:
	free(ifc.ifc_buf);
	close(s);
#endif				/* defined(SIOCGIFCONF) && defined(SIOCGIFFLAGS) */
}

int main(int argc, const char *argv[])
{
	const char *progname = "pi-csd";

        int 	c,		/* switch */
		n,
		sockfd;
	int quiet = 0;

        struct	hostent *hent;
	struct 	in_addr raddress;
	struct 	sockaddr_in serv_addr, cli_addr;

        char    *addrarg	= NULL,
		*nmarg		= NULL;

        fd_set 	rset;
	unsigned char mesg[1026];
        unsigned int clilen;


	poptContext po;

	struct poptOption options[] = {
		/* Don't use USERLAND_RESERVED_OPTIONS, because this thing doesn't
		   connect to the Palm at all. */
		{ "version",  0 , POPT_ARG_NONE,    NULL, 'v', "Display version information", NULL},
		{ "quiet",   'q', POPT_ARG_NONE,  &quiet,  0 , "Suppress messages", NULL},
        	{"hostname",	'H', POPT_ARG_STRING, &hostname, 0, "The hostname used for verification"},
	        {"address",	'a', POPT_ARG_STRING, &addrarg, 0, "IP address of the host","name-or-IP"},
        	{"netmask",	'n', POPT_ARG_STRING, &nmarg, 0, "The subnet mask of the address","dotted-quad"},
		POPT_AUTOHELP
                POPT_TABLEEND
	};

	memset(&address, 0, sizeof(address));
	memset(&netmask, 0, sizeof(netmask));
	hostname[0] = 0;
	fetch_host(hostname, 128, &address, &netmask);

	po = poptGetContext(progname, argc, argv, options, 0);
	poptSetOtherOptionHelp(po,"\n\n"
		"   Connection Service Daemon for Palm Devices\n\n"
		"   Example arguments:\n"
		"      -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((c = poptGetNextOpt(po)) >= 0) {
		switch (c) {
		case 'v':
			print_splash(progname);
			return 0;
		default:
			fprintf(stderr,"   ERROR: Unhandled option %d.\n",c);
			return 1;
		}
	}

	if (!addrarg) {
		fprintf(stderr,"   ERROR: Must give an address with -a.\n");
		return 1;
	}

	if (!inet_pton(AF_INET, addrarg, &address)) {
		if ((hent = gethostbyname(addrarg))) {
			memcpy(&address.s_addr,
				hent->h_addr,
				sizeof(address));
		} else {
			fprintf(stderr,"   ERROR: Invalid address '%s'\n\n",addrarg);
			return 1;
		}
	}


	if (nmarg) {
		if (!inet_pton(AF_INET, nmarg, &netmask)) {
			fprintf(stderr,"   ERROR: Invalid netmask '%s'\n\n",nmarg);
			return 1;
		}
	}

	/* cannot execute without address and hostname */
	if ((address.s_addr == 0) || (strlen(hostname) == 0)) {
		fprintf(stderr,"   ERROR: Must give an address and a hostname.\n");
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		fprintf(stderr,"   ERROR: Unable to get socket: %s\n",strerror(errno));
		return 1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family 		= AF_INET;
	serv_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
	serv_addr.sin_port 		= htons(14237);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
	    < 0) {
		fprintf(stderr,"   ERROR: Unable to bind socket: %s\n",strerror(errno));
		return 1;
	}

	if (!quiet) {
		fprintf(stdout,
			"%s(%d): Connection Service Daemon for Palm Computing(tm) device active.\n",
			progname, getpid());
		fprintf(stdout,
			"%s(%d): Accepting connection requests for '%s' at %s",
			progname, getpid(), hostname, inet_ntoa(address));
		fprintf(stdout, " with mask %s.\n", inet_ntoa(netmask));
	}
	for (;;) {
		fflush(stdout);
		clilen = sizeof(cli_addr);
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		if (select(sockfd + 1, &rset, 0, 0, 0) < 0) {
			fprintf(stderr,"   ERROR: Select failure: %s\n",strerror(errno));
			close(sockfd);
			return 1;
		}
		n = recvfrom(sockfd, mesg, 1024, 0,
			     (struct sockaddr *) &cli_addr, &clilen);

		if (n < 0) {
			continue;
		}

		mesg[n] = 0;

		if (!quiet) {
			hent =
			    gethostbyaddr((char *) &cli_addr.sin_addr.
					  s_addr, 4, AF_INET);
			memcpy(&raddress, &cli_addr.sin_addr.s_addr, 4);

			fprintf(stdout, "%s(%d): Connection from %s[%s], ",
				progname,
				getpid(), hent ? hent->h_name : "",
				inet_ntoa(raddress));
		}

		if (get_short(mesg) != 0xFADE)
			goto invalid;

		if ((get_byte(mesg + 2) == 0x01) && (n > 12)) {
			struct 	in_addr ip, mask;
			char *name = mesg + 12;

			memcpy(&ip, mesg + 4, 4);
			memcpy(&mask, mesg + 8, 4);

			if (!quiet) {
				fprintf(stdout, "req '%s', %s", name,
					inet_ntoa(ip));
				fprintf(stdout, ", %s", inet_ntoa(mask));
			}

			if (strcmp(hostname, name) == 0) {
				if (!quiet)
					fprintf(stdout, " = accept.\n");

				set_byte(mesg + 2, 0x02);
				memcpy(mesg + 4, &address, 4);	/* address is already in Motorola byte order */
				n = sendto(sockfd, mesg, n, 0,
					   (struct sockaddr *) &cli_addr,
					   clilen);
				if (n < 0) {
					fprintf(stderr,"   ERROR: sendto() error: %s\n",
						strerror(errno));
				}
				continue;
			}
			if (!quiet) {
				fprintf(stdout, " = reject.\n");
			}
			continue;
		}

	      invalid:
		if (!quiet)
			fprintf(stdout, "invalid packet of %d bytes:\n", n);
		dumpdata(mesg, n);
	}

	return 0;
}
