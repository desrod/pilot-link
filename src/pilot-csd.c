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

#include "getopt.h"
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
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-serial.h"
#include "pi-slp.h"
#include "pi-header.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"hostname",    required_argument, NULL, 'H'},
	{"address",     required_argument, NULL, 'a'},
	{"netmask",     required_argument, NULL, 'n'},
	{"quiet",       required_argument, NULL, 'q'},
	{NULL,          0,                 NULL, 0}
};

static const char *optstring = "hvp:H:a:n:q";

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
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
fetch_host(char *hostname, int hostlen, struct in_addr *address,
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
#endif				/* defined(IFF_POINTOPOINT) && defined(SIOCGIFDSTADDR) */
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

/***********************************************************************
 *
 * Function:    display_help
 *
 * Summary:     Uh, the -help, of course
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void display_help(char *progname)
{
	printf("   Connection Service Daemon for Palm Devices\n\n");
	printf("   Usage: %s -H <hostname> -a <ip> -n <subnet>\n\n", progname);
	printf("   Options:\n");
	printf("     -H <hostname>     The hostname used for verification\n");
	printf("     -a <ip address>   IP address of the host\n");
	printf("     -n <netmask>      The subnet mask of the address\n");
	printf("     -q, --quiet       Turn off status messages\n");
	printf("     -h, --help        Display this information\n");
	printf("     -v, --version     Display version information\n\n");
	printf("   Examples: %s -H \"localhost\" -a 127.0.0.1 -n 255.255.255.0\n\n", progname);

	exit(0);
}

int main(int argc, char *argv[])
{
        int 	c,		/* switch */
		n,
		quiet 		= 0,
		sockfd;
	
        struct	hostent *hent;
	struct 	in_addr raddress;
	struct 	sockaddr_in serv_addr, cli_addr; 

        char 	*progname 	= argv[0];

        fd_set 	rset;
        unsigned char mesg[1026];
        unsigned int clilen; 

	memset(&address, 0, sizeof(address));
	memset(&netmask, 0, sizeof(netmask));
	hostname[0] = 0;

	fetch_host(hostname, 128, &address, &netmask);

	while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (c) {

		case 'h':
			display_help(progname);
			return 0;
		case 'v':
			print_splash(progname);
			return 0;
		case 'H':
			strcpy(hostname, optarg);
			break;
		case 'a':
			if (!inet_aton(optarg, &address)) {
				if ((hent = gethostbyname(optarg))) {
					memcpy(&address.s_addr,
					       hent->h_addr,
					       sizeof(address));
				} else {
					fprintf(stderr,
						"Invalid address '%s'\n\n",
						optarg);
					display_help(progname);
				}
			}
			break;
		case 'n':
			if (!inet_aton(optarg, &netmask))
				display_help(progname);
			break;
		case 'q':
			quiet = 1;
			break;
		}
	}

	/* cannot execute without address and hostname */
	if ((address.s_addr == 0) || (strlen(hostname) == 0)) {
		display_help(progname);
		return -1;
	}
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Unable to get socket");
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family 		= AF_INET;
	serv_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
	serv_addr.sin_port 		= htons(14237);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
	    < 0) {
		perror("Unable to bind socket");
		exit(1);
	}

	if (!quiet) {
		fprintf(stderr,
			"%s(%d): Connection Service Daemon for Palm Computing(tm) device active.\n",
			progname, getpid());
		fprintf(stderr,
			"%s(%d): Accepting connection requests for '%s' at %s",
			progname, getpid(), hostname, inet_ntoa(address));
		fprintf(stderr, " with mask %s.\n", inet_ntoa(netmask)
		    );
	}
	for (;;) {
		clilen = sizeof(cli_addr);
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		if (select(sockfd + 1, &rset, 0, 0, 0) < 0) {
			perror("select failure");
			exit(1);
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

			fprintf(stderr, "%s(%d): Connection from %s[%s], ",
				progname,
				getpid(), hent ? hent->h_name : "",
				inet_ntoa(raddress));
		}

		if (get_short(mesg) != 0xFADE)
			goto invalid;

		if ((get_byte(mesg + 2) == 0x01) && (n > 12)) {
			struct 	in_addr ip, mask;
			unsigned char *name = mesg + 12;

			memcpy(&ip, mesg + 4, 4);
			memcpy(&mask, mesg + 8, 4);

			if (!quiet) {
				fprintf(stderr, "req '%s', %s", name,
					inet_ntoa(ip));
				fprintf(stderr, ", %s", inet_ntoa(mask));
			}

			if (strcmp(hostname, name) == 0) {
				if (!quiet)
					fprintf(stderr, " = accept.\n");

				set_byte(mesg + 2, 0x02);
				memcpy(mesg + 4, &address, 4);	/* address is already in Motorola byte order */
				n = sendto(sockfd, mesg, n, 0,
					   (struct sockaddr *) &cli_addr,
					   clilen);
				if (n < 0) {
					perror("sendto error");
				}
				continue;
			}
			if (!quiet)
				fprintf(stderr, " = reject.\n");
			continue;
		}

	      invalid:
		if (!quiet)
			fprintf(stderr, "invalid packet of %d bytes:\n", n);
		dumpdata(mesg, n);
	}

	exit(0);
}

