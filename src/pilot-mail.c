/*
 * pilot-mail.c:  Synchronize mail between palm and active network 
 *                connection
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * Modifications by Diego Zamboni to allow it to read mail from an MH-style
 * mailbox into the Palm.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define PILOTPORT	""
#define POPHOST 	""
#define POPUSER		""
#define POPPASS		""
#define PILOTFROM	""
#define SENDMAIL	"/usr/lib/sendmail -t -i"
#define POPKEEP 	"keep"
#define DISPOSE		"keep"
#define TOPILOT_MHDIR   ""

/* Todo: truncation, filtering, priority, notification */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-mail.h"
#include "pi-dlp.h"
#include "pi-header.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern time_t parsedate(char *p);

void markline(char *msg)
{
   while ((*msg) != '\n' && (*msg) != 0) {
      msg++;
   }
   (*msg) = 0;
}

int openmhmsg(char *dir, int num)
{
   char filename[1000];


   sprintf(filename, "%s/%d", dir, num);

   return open(filename, O_RDONLY);
}

int getpopchar(int socket)
{
   unsigned char buf;
   int l;

   do {
      l = read(socket, &buf, 1);
      if (l < 0)
	 return l;
   } while ((l == 0) || (buf == '\r'));

   return buf;
}

int getpopstring(int socket, char *buf)
{
   int c;

   while ((c = getpopchar(socket)) >= 0) {
      *buf++ = c;
      if (c == '\n')
	 break;
   }
   *buf = '\0';
   return c;
}

int getpopresult(int socket, char *buf)
{
   int c = getpopstring(socket, buf);

   if (c < 0)
      return c;

   if (buf[0] == '+')
      return 0;
   else
      return 1;
}

char *skipspace(char *c)
{
   while (c && ((*c == ' ') || (*c == '\t')))
      c++;
   return c;
}

void header(struct Mail *m, char *t)
{
   static char holding[4096];

   if (t && strlen(t) && t[strlen(t) - 1] == '\n')
      t[strlen(t) - 1] = 0;

   if (t && ((t[0] == ' ') || (t[0] == '\t'))) {
      if ((strlen(t) + strlen(holding)) > 4096)
	 return;		/* Just discard approximate overflow */
      strcat(holding, t + 1);
      return;
   }

   /* Decide on what we do with m->sendTo */

   if (strncmp(holding, "From:", 5) == 0) {
      m->from = strdup(skipspace(holding + 5));
   } else if (strncmp(holding, "To:", 3) == 0) {
      m->to = strdup(skipspace(holding + 3));
   } else if (strncmp(holding, "Subject:", 8) == 0) {
      m->subject = strdup(skipspace(holding + 8));
   } else if (strncmp(holding, "Cc:", 3) == 0) {
      m->cc = strdup(skipspace(holding + 3));
   } else if (strncmp(holding, "Bcc:", 4) == 0) {
      m->bcc = strdup(skipspace(holding + 4));
   } else if (strncmp(holding, "Reply-To:", 9) == 0) {
      m->replyTo = strdup(skipspace(holding + 9));
   } else if (strncmp(holding, "Date:", 4) == 0) {
      time_t d = parsedate(skipspace(holding + 5));

      if (d != -1) {
	 struct tm *d2;

	 m->dated = 1;
	 d2 = localtime(&d);
	 m->date = *d2;
      }
   }

   holding[0] = 0;

   if (t)
      strcpy(holding, t);
}

void Help(char *progname)
{
   PalmHeader(progname);

   fprintf(stderr, "   Usage: %s [options]\n\n", progname);
   fprintf(stderr, "     -p port             = Serial port to use $(PILOTPORT)\n"); 
   fprintf(stderr, "                           (defaults to '%s')\n", PILOTPORT);
   fprintf(stderr, "     -h host             = POP3 host (if empty, mail won't be received)\n");
   fprintf(stderr, "                           $(POPHOST), (defaults to '%s')\n", POPHOST);
   fprintf(stderr, "     -u user             = POP3 user name $(POPUSER)\n");
   fprintf(stderr, "                           (defaults to '%s')\n", POPUSER);
   fprintf(stderr, "     -P pass             = POP3 password $(POPPASS)\n");
   fprintf(stderr, "                           (default value set)\n");
   fprintf(stderr, "     -f address          = Outgoing 'From:' line $(PILOTFROM)\n");
   fprintf(stderr, "                           (defaults to '%s')\n", PILOTFROM);
   fprintf(stderr, "     -s command          = Sendmail command (if empty, mail won't be sent)\n");
   fprintf(stderr, "                           $(SENDMAIL), (defaults to '%s')\n", SENDMAIL);
   fprintf(stderr, "     -k keep|delete      = Keep mail on POP server $(POPKEEP)\n");
   fprintf(stderr, "                           (defaults to '%s')\n", POPKEEP);
   fprintf(stderr, "     -d keep|delete|file = Disposition of sent mail $(PILOTDISPOSE)\n");
   fprintf(stderr, "                           (defaults to '%s')\n", DISPOSE);
   fprintf(stderr, "     -m mhdir            = MH directory to download to Palm $(TOPILOT_MHDIR)\n");
   fprintf(stderr, "                           (defaults to '%s')\n", TOPILOT_MHDIR);
   fprintf(stderr, "\n   All options may be specified by setting the environment variable named\n   in brackets.\n\n");
   fprintf(stderr, "      ********************************************* **\n");
   fprintf(stderr, "      ** NOTE!! This is being deprecated soon. This **\n");
   fprintf(stderr, "      ** means that this application is going away! **\n");
   fprintf(stderr, "      ** Please use pilot-mailsync instead. Consult **\n");
   fprintf(stderr, "      ** the manpages for additional information... **\n");
   fprintf(stderr, "      ************************************************\n\n");
      
   exit(0);
}

void sigint(int num)
{
   *((char *) 0) = 0;
}

int main(int argc, char *argv[])
{
   struct pi_sockaddr addr;
   int db;
   int sd;
   int i, l = 0;
   int lost, dupe, rec, sent;
   struct PilotUser U;
   int ret;
   unsigned char buffer[0xffff];
   struct MailAppInfo tai;
   struct MailSyncPref p;
   struct MailSignaturePref sig;
   struct sockaddr_in pop_addr;
   int popfd;
   struct hostent *hostent;
   int c;
   char *progname = argv[0];
   char *device = argv[1];


   char *from_address =
#ifdef PILOTFROM
       PILOTFROM;
#else
       "";
#endif
   char *pop_host =
#ifdef POPHOST
       POPHOST;
#else
       "";
#endif
   char *pop_user =
#ifdef POPUSER
       POPUSER;
#else
       "";
#endif
   char *pop_pass =
#ifdef POPPASS
       POPPASS;
#else
       "";
#endif
   char *sendmail =
#ifdef SENDMAIL
       SENDMAIL;
#else
       "";
#endif
   char *pop_keep =
#ifdef POPKEEP
       POPKEEP;
#else
       "";
#endif
   char *dispose =
#ifdef DISPOSE
       DISPOSE;
#else
       "";
#endif
   char *port =
#ifdef PILOTPORT
       PILOTPORT;
#else
       "";
#endif
   char *mh_dir =
#ifdef TOPILOT_MHDIR
       TOPILOT_MHDIR;
#else
       "";
#endif

   extern char *optarg;
   extern int optind;


   if (getenv("SENDMAIL"))
      sendmail = getenv("SENDMAIL");
   if (getenv("POPKEEP"))
      pop_keep = getenv("POPKEEP");
   if (getenv("PILOTDISPOSE"))
      dispose = getenv("PILOTDISPOSE");
   if (getenv("PILOTFROM"))
      from_address = getenv("PILOTFROM");
   if (getenv("POPHOST"))
      pop_host = getenv("POPHOST");
   if (getenv("POPUSER"))
      pop_user = getenv("POPUSER");
   if (getenv("POPPASS"))
      pop_pass = getenv("POPPASS");
   if (getenv("PILOTPORT"))
      port = getenv("PILOTPORT");
   if (getenv("TOPILOT_MHDIR"))
      mh_dir = getenv("TOPILOT_MHDIR");

   signal(SIGINT, sigint);
 
   if (argc < 2)
      Help(progname);
         

   while ((c = getopt(argc, argv, "s:p:d:f:h:u:p:h:P:k:m:")) != -1) {
      switch (c) {
      case 's':
	 sendmail = optarg;
	 break;
      case 'p':
	 port = optarg;
	 break;
      case 'h':
	 pop_host = optarg;
	 break;
      case 'u':
	 pop_user = optarg;
	 break;
      case 'P':
	 pop_pass = optarg;
	 break;
      case 'f':
	 from_address = optarg;
	 break;
      case 'd':
	 dispose = optarg;
	 break;
      case 'k':
	 pop_keep = optarg;
	 break;
      case 'm':
	 mh_dir = optarg;
	 break;
      case 'H':
      case '?':
      default:
	 Help(progname);
      }
   }
   argc -= optind;
   argv += optind;

   if (!strlen(port)) {
      fprintf(stderr, "\n   Port must be set before %s will function properly.\n\n", progname);
      Help(progname);
   }


   if (!strlen(pop_host) && !strlen(sendmail)) {
      fprintf(stderr, "%s requires that at least one of -h or -s must be set.\n\n", progname);
      Help(progname);
   }

   if (strlen(pop_host)) {
      if (!strlen(pop_user)) {
	 fprintf(stderr, "\nUndeclared option -u must be set to receive mail.\n\n");
	 Help(progname);
      } else if (!strlen(pop_keep)
		 || (strcmp(pop_keep, "keep")
		     && strcmp(pop_keep, "delete"))) {
	 fprintf(stderr, "-k must have an argument of 'keep' or 'delete'.\n\n");
	 Help(progname);
      }
   }

   if (!strlen(pop_pass) && strlen(pop_host)) {
      pop_pass = getpass("POP password: ");
   }

   if (strlen(sendmail)) {
      if (!strlen(dispose) || (strcmp(dispose, "keep") &&
			       strcmp(dispose, "delete") &&
			       strcmp(dispose, "file"))) {
	 fprintf(stderr, "-d must have an argument of 'keep', 'delete', or 'file'.\n\n");
	 Help(progname);
      }
   }

   if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP))) {
      perror("pi_socket");
      exit(1);
   }

   lost = dupe = rec = sent = 0;

   addr.pi_family = PI_AF_SLP;
   strcpy(addr.pi_device, device);

   PalmHeader(progname);

   ret = pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr));
   if (ret == -1)
   {
      fprintf(stderr, "\n   Unable to bind to port %s\n", device);
      perror("   pi_bind");
      fprintf(stderr, "\n");
      exit (1);
   }
   
   printf("   Port: %s\n\n   Please press the HotSync button now...\n", device);

   ret = pi_listen(sd, 1);
   if (ret == -1) {
      fprintf(stderr, "\n   Error listening on %s\n", device);
      perror("   pi_listen");
      fprintf(stderr, "\n");
      exit(1);
   }

   sd = pi_accept(sd, 0, 0);
   if (sd == -1) {
      fprintf(stderr, "\n   Error accepting data on %s\n", device);
      perror("   pi_accept");
      fprintf(stderr, "\n");
      exit(1);
   }

   fprintf(stderr, "Connected...\n");
   memset(&tai, '\0', sizeof(struct MailAppInfo));
   memset(&p, '\0', sizeof(struct MailSyncPref));

   /* Ask the pilot who it is. */
   dlp_ReadUserInfo(sd, &U);

   /* Tell user (via Palm) that we are starting things up */
   dlp_OpenConduit(sd);

   /* Open the Mail database, store access handle in db */
   if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MailDB", &db) < 0) {
      fprintf(stderr, "Unable to open MailDB\n");
      dlp_AddSyncLogEntry(sd, "Unable to open MailDB.\n");
      exit(1);
   }

   dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
   unpack_MailAppInfo(&tai, buffer, 0xffff);

   setbuf(stderr, 0);

   p.syncType = 0;
   p.getHigh = 0;
   p.getContaining = 0;
   p.truncate = 8 * 1024;
   p.filterTo = 0;
   p.filterFrom = 0;
   p.filterSubject = 0;

   if (pi_version(sd) > 0x0100) {
      if (dlp_ReadAppPreference
	  (sd, makelong("mail"), 1, 1, 0xffff, buffer, 0, 0) >= 0) {
	 printf("Got local backup mail preferences\n");	/* 2 for remote prefs */
	 unpack_MailSyncPref(&p, buffer, 0xffff);
      } else {
	 printf("Unable to get mail preferences, trying current\n");
	 if (dlp_ReadAppPreference
	     (sd, makelong("mail"), 1, 1, 0xffff, buffer, 0, 0) >= 0) {
	    printf("Got local current mail preferences\n");	/* 2 for remote prefs */
	    unpack_MailSyncPref(&p, buffer, 0xffff);
	 } else
	    printf("Couldn't get any mail preferences.\n");
      }

      if (dlp_ReadAppPreference
	  (sd, makelong("mail"), 3, 1, 0xffff, buffer, 0, 0) > 0) {
	 unpack_MailSignaturePref(&sig, buffer, 0xffff);
      }

   }

   printf
       ("Local Prefs: Sync=%d, High=%d, getc=%d, trunc=%d, to=|%s|, from=|%s|, subj=|%s|\n",
	p.syncType, p.getHigh, p.getContaining, p.truncate,
	p.filterTo ? p.filterTo : "<none>",
	p.filterFrom ? p.filterFrom : "<none>",
	p.filterSubject ? p.filterSubject : "<none>");

   printf("AppInfo Signature: |%s|\n\n", sig.signature ? sig.signature : "<None>");

#if 0
   for (i = 0; 1; i++) {
      struct Mail t;
      int attr, category;

      int len =

	  dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr, &category);
      if (len < 0)
	 break;

      /* Skip deleted records */
      if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	 continue;

      unpack_Mail(&t, buffer, len);

      printf("Category: %s\n", tai.CategoryName[category]);
      printf("Read: %d, Signature: %d, confirmRead: %d, confirmDeliver: %d, priority: %d, address: %d\n",
	   t.read, t.signature, t.confirmRead, t.confirmDelivery,
	   t.priority, t.addressing);
      printf("Time: %s", t.dated ? asctime(&t.date) : "<Undated>");
      printf("Subject: |%s|\n", t.subject ? t.subject : "<None>");
      printf("From: |%s|\n", t.from ? t.from : "<None>");
      printf("To: |%s|\n", t.to ? t.to : "<None>");
      printf("Cc: |%s|\n", t.cc ? t.cc : "<None>");
      printf("Bcc: |%s|\n", t.bcc ? t.bcc : "<None>");
      printf("ReplyTo: |%s|\n", t.replyTo ? t.replyTo : "<None>");
      printf("SendTo: |%s|\n", t.sentTo ? t.sentTo : "<None>");
      printf("Body: |%s|\n", t.body ? t.body : "<None>");
      printf("\n");

      free_Mail(&t);
   }
#endif

   if (strlen(sendmail)) {
      /* sendmail transmission section */

      /* Iterate over messages in Outbox */
      for (i = 0;; i++) {
	 struct Mail t;
	 int attr;
	 int size;
	 recordid_t id;
	 FILE *sendf;

	 int len =
	     dlp_ReadNextRecInCategory(sd, db, 1, buffer, &id, 0, &size,

				       &attr);
	 if (len < 0)
	    break;

	 /* Skip deleted records */
	 if ((attr & dlpRecAttrDeleted) || (attr & dlpRecAttrArchived))
	    continue;

	 unpack_Mail(&t, buffer, len);

	 sendf = popen(sendmail, "w");

	 if (!sendf) {
	    fprintf(stderr, "Error launching '%s' to transmit mail! (No mail lost, %d received, %d sent)\n", sendmail, rec, sent);
	    pi_close(sd);
	    exit(1);
	 }

	 if (from_address && from_address[0]) {
	    fprintf(sendf, "From: ");
	    fprintf(sendf, from_address);
	    fprintf(sendf, "\n");
	 }

	 if (t.to) {
	    fprintf(sendf, "To: %s\n", t.to);
	 }
	 if (t.cc) {
	    fprintf(sendf, "Cc: %s\n", t.cc);
	 }
	 if (t.bcc) {
	    fprintf(sendf, "Bcc: %s\n", t.bcc);
	 }
	 if (t.replyTo) {
	    fprintf(sendf, "Reply-To: %s\n", t.replyTo);
	 }
	 if (t.subject) {
	    fprintf(sendf, "Subject: %s\n", t.subject);
	 }
	 fprintf(sendf, "X-mailer: pilot-mail-%d.%d.%d\n", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
	 fprintf(sendf, "\n");	/* Separate header */

	 if (t.body) {
	    fputs(t.body, sendf);
	    fprintf(sendf, "\n");
	 }
	 if (t.signature && sig.signature) {
	    char *c = sig.signature;

	    while ((*c == '\r') || (*c == '\n'))
	       c++;
	    if (strncmp(c, "--", 2) && strncmp(c, "__", 2)
		) {
	       fprintf(sendf, "\n-- \n");
	    }
	    fputs(sig.signature, sendf);
	    fprintf(sendf, "\n");
	 }

	 if (pclose(sendf) == 0) {
	    sent++;
	    if (strcmp(dispose, "keep") == 0) {
	       dupe++;
	    } else if (strcmp(dispose, "delete") == 0) {
	       dlp_DeleteRecord(sd, db, 0, id);
	    } else {
	       /* Rewrite into Filed category */
	       dlp_WriteRecord(sd, db, attr, id, 3, buffer, size, 0);
	    }
	 }

	 free_Mail(&t);
      }
   }

   if (strlen(pop_host)) {

      /* POP retrieval section */

      memset((char *) &pop_addr, 0, sizeof(pop_addr));
      if ((pop_addr.sin_addr.s_addr = inet_addr(pop_host)) == -1) {
	 hostent = gethostbyname(pop_host);
	 if (!hostent) {
	    fprintf(stderr, "Unable to resolve POP host '%s'", pop_host);
	    goto end;
	 }
	 memcpy((char *) &pop_addr.sin_addr.s_addr, hostent->h_addr,
		hostent->h_length);
      }
      pop_addr.sin_family = AF_INET;
      pop_addr.sin_port = htons(110);

      if ((popfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	 perror("Unable to obtain socket");
	 goto end;
      }

      if (connect(popfd, (struct sockaddr *) &pop_addr, sizeof(pop_addr)) <
	  0) {
	 fprintf(stderr, "Unable to connect to POP server '%s'\n", pop_host);
	 close(popfd);
	 goto end;
      }

      read(popfd, buffer, 1024);
      if (buffer[0] != '+') {
	 fprintf(stderr, "POP server failed to announce itself\n");
	 goto endpop;
      }

      sprintf(buffer, "USER %s\r\n", pop_user);
      write(popfd, buffer, strlen(buffer));
      read(popfd, buffer, 1024);
      if (buffer[0] != '+') {
	 fprintf(stderr, "USER command to POP server failed\n");
	 goto endpop;
      }

      sprintf(buffer, "PASS %s\r\n", pop_pass);
      write(popfd, buffer, strlen(buffer));
      read(popfd, buffer, 1024);
      if (buffer[0] != '+') {
	 fprintf(stderr, "PASS command to POP server failed\n");
	 close(popfd);
	 goto endpop;
      }

      for (i = 1;; i++) {
	 int len;
	 char *msg;
	 int h;
	 struct Mail t;

	 t.to = 0;
	 t.from = 0;
	 t.cc = 0;
	 t.bcc = 0;
	 t.subject = 0;
	 t.replyTo = 0;
	 t.sentTo = 0;
	 t.body = 0;
	 t.dated = 0;

	 sprintf(buffer, "LIST %d\r\n", i);
	 write(popfd, buffer, strlen(buffer));
	 l = read(popfd, buffer, 1024);
	 if (l < 0) {
	    perror("Unable to read from socket");
	    goto endpop;
	 }
	 buffer[l] = 0;
	 if (buffer[0] != '+')
	    break;

	 sscanf(buffer, "%*s %*d %d", &len);

	 if (len > 16000)
	    continue;

	 sprintf(buffer, "RETR %d\r\n", i);
	 write(popfd, buffer, strlen(buffer));
	 l = getpopstring(popfd, buffer);
	 if ((l < 0) || (buffer[0] != '+')) {
	    /* Weird */
	    continue;
	 } else
	    buffer[l] = 0;

	 msg = (char *) buffer;
	 h = 1;
	 for (;;) {
	    if (getpopstring(popfd, msg) < 0) {
	       fprintf(stderr, "Error reading message\n");
	       goto endpop;
	    }

	    if (h == 1) {	/* Header mode */
	       if ((msg[0] == '.') && (msg[1] == '\n') && (msg[2] == 0)) {
		  break;	/* End of message */
	       }

	       if (msg[0] == '\n') {
		  h = 0;
		  header(&t, 0);
	       } else
		  header(&t, msg);
	       continue;
	    }

	    if ((msg[0] == '.') && (msg[1] == '\n') && (msg[2] == 0)) {
	       msg[0] = 0;
	       break;		/* End of message */
	    }

	    if (msg[0] == '.') {
	       /* Must be escape */
	       memmove(msg, msg + 1, strlen(msg));
	    }

	    msg += strlen(msg);
	 }

	 /* Well, we've now got the message. I bet _you_ feel happy with yourself. */

	 if (h) {
	    /* Oops, incomplete message, still reading headers */
	    fprintf(stderr, "Incomplete message %d\n", i);
	    free_Mail(&t);
	    continue;
	 }

	 if (strlen(msg) > p.truncate) {
	    /* We could truncate it, but we won't for now */
	    fprintf(stderr, "Message %d too large (%ld bytes)\n", i, (long) strlen(msg));
	    free_Mail(&t);
	    continue;
	 }

	 t.body = strdup(buffer);

	 len = pack_Mail(&t, buffer, 0xffff);

	 if (dlp_WriteRecord(sd, db, 0, 0, 0, buffer, len, 0) > 0) {
	    rec++;
	    if (strcmp(pop_keep, "delete") == 0) {
	       sprintf(buffer, "DELE %d\r\n", i);
	       write(popfd, buffer, strlen(buffer));
	       read(popfd, buffer, 1024);
	       if (buffer[0] != '+') {
		  fprintf(stderr, "Error deleting message %d\n", i);
		  dupe++;
	       }
	    } else
	       dupe++;
	 } else {
	    fprintf(stderr, "Error writing message to Palm\n");
	 }

	 free_Mail(&t);
      }

      sprintf(buffer, "QUIT\r\n");
      write(popfd, buffer, strlen(buffer));
      read(popfd, buffer, 1024);
      if (buffer[0] != '+') {
	 fprintf(stderr, "QUIT command to POP server failed\n");
      }

    endpop:
      close(popfd);

   }

   if (strlen(mh_dir)) {

      fprintf(stderr, "Reading directory %s... ", mh_dir);
      fflush(stderr);

      /* MH directory reading section */

      for (i = 1;; i++) {
	 int len;
	 char *msg;
	 int h;
	 struct Mail t;
	 int mhmsg;

	 t.to = 0;
	 t.from = 0;
	 t.cc = 0;
	 t.bcc = 0;
	 t.subject = 0;
	 t.replyTo = 0;
	 t.sentTo = 0;
	 t.body = 0;
	 t.dated = 0;

	 if ((mhmsg = openmhmsg(mh_dir, i)) < 0) {
	    break;
	 }

	 fprintf(stderr, "%d ", i);
	 fflush(stderr);

	 /* Read the message */
	 len = 0;
	 while ((len < sizeof(buffer)) &&
		((l
		  =
		  read(mhmsg, (char *) (buffer + len),
		       (sizeof(buffer) - len))) > 0)) {
	    len += l;
	 }
	 buffer[len] = 0;

	 if (l < 0) {
	    perror("Error while reading message");
	    goto endmh;
	 }

	 msg = (char *) buffer;
	 h = 1;
	 while (h == 1) {
	    markline(msg);

	    if ((msg[0] == 0) && (msg[1] == 0)) {
	       break;		/* End of message */
	    }

	    if (msg[0] == 0) {
	       h = 0;
	       header(&t, 0);
	    } else
	       header(&t, msg);
	    msg += strlen(msg) + 1;
	 }

	 /* When we get here, we are done with the headers */
	 if ((*msg) == 0) {
	    /* Empty message */
	    h = 1;
	 }

	 /* Well, we've now got the message. I bet _you_ feel happy with yourself. */

	 if (h) {
	    /* Oops, incomplete message, still reading headers */
	    fprintf(stderr, "Incomplete message %d\n", i);
	    free_Mail(&t);
	    continue;
	 }

	 if (strlen(msg) > p.truncate) {
	    /* We could truncate it, but we won't for now */
	    fprintf(stderr, "Message %d too large (%ld bytes)\n", i, (long) strlen(msg));
	    free_Mail(&t);
	    continue;
	 }

	 t.body = strdup(msg);

	 len = pack_Mail(&t, buffer, 0xffff);

	 if (dlp_WriteRecord(sd, db, 0, 0, 0, buffer, len, 0) > 0) {
	    rec++;
	    if (strcmp(pop_keep, "delete") == 0) {
	       char filename[1000];

	       sprintf(filename, "%s/%d", mh_dir, i);
	       close(mhmsg);
	       if (unlink(filename)) {
		  fprintf(stderr, "Error deleting message %d\n", i);
		  dupe++;
	       }
	       continue;
	    } else
	       dupe++;
	 } else {
	    fprintf(stderr, "Error writing message to Palm\n");
	 }

	 free_Mail(&t);

	 close(mhmsg);
      }

    endmh:
      fprintf(stderr, "\n");

   }

 end:
   free_MailSyncPref(&p);
   free_MailSignaturePref(&sig);
   free_MailAppInfo(&tai);

   dlp_ResetLastSyncPC(sd);

   /* Close the database */
   dlp_CloseDB(sd, db);

   sprintf(buffer, "Finished transferring mail. %d message%s sent, %d message%s received.\n", sent, (sent == 1) ? "" : "s", rec, (rec == 1) ? "" : "s");
   if (lost || dupe)
      sprintf(buffer + strlen(buffer), "(And %d lost, %d duplicated. Sorry.)\n", lost, dupe);
   fprintf(stderr, buffer);
   dlp_AddSyncLogEntry(sd, buffer);

   pi_close(sd);
   return 0;
}
