/* pi-csd.c: Connection Service Daemon, required for accepting logons via NetSync(tm)
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
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


char hostname[130];
struct in_addr address, netmask;

void Help(char *argv[])
{
   fprintf(stderr, "usage:%s [options]\n", argv[0]);
   fprintf(stderr,
	   "\n      <-h hostname> Name of host, used for verification");
   if (strlen(hostname))
      fprintf(stderr, "\n                   (defaults to '%s')", hostname);
   else
      fprintf(stderr, "\n                   (no default)");
   fprintf(stderr, "\n      <-a address> IP address of host");
   if (address.s_addr)
      fprintf(stderr, "\n                   (defaults to '%s')",
	      inet_ntoa(address));
   else
      fprintf(stderr, "\n                   (no default)");
   fprintf(stderr, "\n      <-s address> Subnet mask of IP address");
   if (netmask.s_addr)
      fprintf(stderr, "\n                   (defaults to '%s')",
	      inet_ntoa(netmask));
   else
      fprintf(stderr, "\n                   (no default)");
   fprintf(stderr, "\n      <-q> Quiet: turn off status messages\n");
   fprintf(stderr,
	   "\nNote: Currently the subnet mask is not used by %s.\n",
	   argv[0]);
   exit(0);
}

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


/* While this function is useful in pi-csd, it is intended also to
   be a demonstration of the proper (or improper, if I'm unlucky) techniques
   to retrieve networking information.*/

void fetch_host(char *hostname, int hostlen, struct in_addr *address,
		struct in_addr *mask)
{
   int s, n, i;
   struct ifconf ifc;
   struct ifreq *ifr, ifreqaddr, ifreqmask;
   struct hostent *hent;

#ifdef HAVE_GETHOSTNAME
   /* Get host name the easy way */

   gethostname(hostname, hostlen);
#else
# ifdef HAVE_UNAME
   struct utsname uts;

   if (uname(&uts) == 0) {
      strncpy(hostname, uts.nodename, hostlen - 1);
      hostname[hostlen - 1] = '\0';
   }
# endif				/*def HAVE_UNAME */
#endif				/*def HAVE_GETHOSTNAME */

   /* Get host address through DNS */
   hent = gethostbyname(hostname);

   if (hent) {
      while (*hent->h_addr_list) {
	 struct in_addr haddr;

	 memcpy(&haddr, *(hent->h_addr_list++), sizeof(haddr));
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

      ifr = (struct ifreq *) ((caddr_t) ifc.ifc_buf + i);
      a = (struct sockaddr_in *) &ifr->ifr_addr;
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
#endif				/*def IFF_LOOPBACK */

#ifdef IFF_UP
      /* Reject down devices */
      if (!(ifreqaddr.ifr_flags & IFF_UP))
	 continue;
#endif				/*def IFF_UP */

      if (ifr->ifr_addr.sa_family != AF_INET)
	 continue;

      /* If it is a point-to-point device, use the dest address */
#if defined(IFF_POINTOPOINT) && defined(SIOCGIFDSTADDR)
      if (ifreqaddr.ifr_flags & IFF_POINTOPOINT) {
	 if (ioctl(s, SIOCGIFDSTADDR, (char *) &ifreqaddr) < 0)
	    break;

	 a = (struct sockaddr_in *) &ifreqaddr.ifr_dstaddr;

	 if (address->s_addr == 0) {
	    memcpy(address, &a->sin_addr, sizeof(struct in_addr));
	 }
      } else
#endif				/*defined(IFF_POINTOPOINT) && defined(SIOCGIFDSTADDR) */
	 /* If it isn't a point-to-point device, use the address */
#ifdef SIOCGIFADDR
      {
	 if (ioctl(s, SIOCGIFADDR, (char *) &ifreqaddr) < 0)
	    break;

	 a = (struct sockaddr_in *) &ifreqaddr.ifr_addr;

	 if (address->s_addr == 0) {
	    memcpy(address, &a->sin_addr, sizeof(struct in_addr));
	 }
      }
#endif				/*def SIOCGIFADDR */
      /* OK, we've got an address */

      /* Compare netmask against the current address and see if it seems to match. */
#ifdef SIOCGIFNETMASK
      if (ioctl(s, SIOCGIFNETMASK, (char *) &ifreqmask) < 0)
	 break;

/*
 * Is there any system where we need to use ifr_netmask?
 */
#if 1
      b = (struct sockaddr_in *) &ifreqmask.ifr_addr;
#else
      b = (struct sockaddr_in *) &ifreqmask.ifr_netmask;
#endif

      if ((mask->s_addr == 0) && (address->s_addr != 0)) {
	 if ((b->sin_addr.s_addr & a->sin_addr.s_addr) ==
	     (b->sin_addr.s_addr & address->s_addr)) {
	    memcpy(mask, &b->sin_addr, sizeof(struct in_addr));

	    /* OK, we've got a netmask */

	    break;
	 }
      }
#endif				/*def SIOCGIFNETMASK */

   }

 done:
   free(ifc.ifc_buf);
   close(s);
#endif				/*defined(SIOCGIFCONF) && defined(SIOCGIFFLAGS) */
}

void main(int argc, char *argv[])
{
   int sockfd;
   struct sockaddr_in serv_addr, cli_addr;
   int clilen;
   char mesg[1026];
   fd_set rset;
   int n;
   struct in_addr raddress;

   char *progname;
   extern char *optarg;
   extern int optind;

   struct hostent *hent;
   int quiet = 0;

   progname = argv[0];

   memset(&address, 0, sizeof(address));
   memset(&netmask, 0, sizeof(netmask));
   hostname[0] = 0;

   fetch_host(hostname, 128, &address, &netmask);

   while ((n = getopt(argc, argv, "h:s:a:q")) != EOF) {
      switch (n) {
      case 'h':
	 strcpy(hostname, optarg);
	 break;
      case 'a':
	 if (!inet_aton(optarg, &address)) {
	    if ((hent = gethostbyname(optarg))) {
	       memcpy(&address.s_addr, hent->h_addr, sizeof(address));
	    } else {
	       fprintf(stderr, "Invalid address '%s'\n\n", optarg);
	       Help(argv);
	    }
	 }
	 break;
      case 's':
	 if (!inet_aton(optarg, &netmask))
	    Help(argv);
	 break;
      case 'q':
	 quiet = 1;
	 break;
      case 'H':
      case '?':
      default:
	 Help(argv);
      }
   }

   if ((address.s_addr == 0) || (strlen(hostname) == 0))
      Help(argv);		/* cannot execute without address and hostname */

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0) {
      perror("Unable to get socket");
      exit(1);
   }

   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(14237);

   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("Unable to bind socket");
      exit(1);
   }

   if (!quiet) {
      fprintf(stderr,
	      "%s(%d): Connection Service Daemon for Palm Computing(tm) device active.\n",
	      progname, getpid());
      fprintf(stderr,
	      "%s(%d): Accepting connection requests for '%s' at %s",
	      progname, getpid(), hostname, inet_ntoa(address)
	  );
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
      n =
	  recvfrom(sockfd, mesg, 1024, 0, (struct sockaddr *) &cli_addr,
		   &clilen);

      if (n < 0) {
	 continue;
      }

      mesg[n] = 0;

      if (!quiet) {
	 hent =
	     gethostbyaddr((char *) &cli_addr.sin_addr.s_addr, 4, AF_INET);
	 memcpy(&raddress, &cli_addr.sin_addr.s_addr, 4);

	 fprintf(stderr, "%s(%d): Connection from %s[%s], ",
		 progname,
		 getpid(), hent ? hent->h_name : "", inet_ntoa(raddress));
      }

      if (get_short(mesg) != 0xFADE)
	 goto invalid;

      if ((get_byte(mesg + 2) == 0x01) && (n > 12)) {
	 struct in_addr ip, mask;
	 char *name = mesg + 12;

	 memcpy(&ip, mesg + 4, 4);
	 memcpy(&mask, mesg + 8, 4);

	 if (!quiet) {
	    fprintf(stderr, "req '%s', %s", name, inet_ntoa(ip));
	    fprintf(stderr, ", %s", inet_ntoa(mask));
	 }

	 if (strcmp(hostname, name) == 0) {
	    if (!quiet)
	       fprintf(stderr, " = accept.\n");

	    set_byte(mesg + 2, 0x02);
	    memcpy(mesg + 4, &address, 4);	/* address is already in motorola byte order */
	    n =
		sendto(sockfd, mesg, n, 0, (struct sockaddr *) &cli_addr,
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
