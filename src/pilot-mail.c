/*
 * pilot-mail.c:  Synchronize mail between palm and active network
 *                connection
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * Modifications by Diego Zamboni to allow it to read mail from an MH-style
 * mailbox into the Palm.
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
 * You should have received a copy of the GNU General Public License alon
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* Todo: truncation, filtering, priority, notification */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-mail.h"
#include "pi-dlp.h"
#include "pi-header.h"

/* Declare prototypes */
void print_splash(char *progname);
int pilot_connect(char *port);

void markline(char *msg);
int openmhmsg(char *dir, int num);
unsigned int getpopchar(int socket);
unsigned int getpopstring(int socket, char *buf);
int getpopresult(int socket, char *buf);
char *skipspace(char *c);
void header(struct Mail *m, char *t);
extern time_t parsedate(char *p);

char *getvars(char *name, char *xdefault);

/* This should be a struct, maybe later, we're killing this anyway. Passing
   this many things in is silly, but its cleaner than the previous. -DD 
 */
static void display_help(char *progname, char *port, char *from_address, char *pop_host,
		 char *pop_user, char *pop_pass, char *sendmail, char *pop_keep,
		 char *pilot_dispose, char *topilot_mhdir);

static const char *optstring = "s:p:d:f:H:u:p:P:k:m:h";

void markline(char *msg)
{
	while ((*msg) != '\n' && (*msg) != 0) {
		msg++;
	}
	(*msg) = 0;
}

int openmhmsg(char *dir, int num)
{
	char 	filename[1000];

	sprintf(filename, "%s/%d", dir, num);

	return open(filename, O_RDONLY);
}

unsigned int getpopchar(int socket)
{
	int 	len;
	char 	buf;

	do {
		len = read(socket, &buf, 1);
		if (len < 0)
			return len;
	} while ((len == 0) || (buf == '\r'));

	return buf;
}

unsigned int getpopstring(int socket, char *buf)
{
	int 	c;	/* switch */

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
	int 	c = getpopstring(socket, buf);

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
	static char 	holding[4096];

	if (t && strlen(t) && t[strlen(t) - 1] == '\n')
		t[strlen(t) - 1] = 0;

	if (t && ((t[0] == ' ') || (t[0] == '\t'))) {
		if ((strlen(t) + strlen(holding)) > 4096)
			return;	/* Just discard approximate overflow */
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
			struct 	tm *d2;

			m->dated = 1;
			d2 	 = localtime(&d);
			m->date  = *d2;
		}
	}

	holding[0] = '\0';

	if (t)
		strcpy(holding, t);
}

char *getvars(char *name, char *xdefault)
{
	char 	*s;

	if ((s = getenv(name)) == NULL) {
		s = xdefault;
	}
	return s;
}

static void display_help(char *progname, char *port, char *pop_host, char *pop_user, 
		 char *pop_pass, char *from_address, char *pop_keep, 
		 char *pilot_dispose, char *topilot_mhdir, char *sendmail)
{
	printf("   Send and receive mail to and from your Palm device using POP3\n\n");
	printf("   Usage: %s -p <port> [options]\n\n", progname);
	printf("     -p <port>            Use device file <port> to communicate with Palm\n");
	printf("                          [$PILOTPORT is currently: '%s']\n\n", port);
	printf("     -h                   Display this information\n");
	printf("     -H host              POP3 host (if empty, mail won't be received)\n");
	printf("                          [$POPHOST is currently: '%s']\n\n", pop_host);
	printf("     -u user              POP3 user name\n");
	printf("                          [$POPUSER is currently: '%s']\n\n", pop_user);
	printf("     -P pass              POP3 password\n");
	printf("                          [$POPPASS is currently: '%s']\n\n", pop_pass);
	printf("     -f address           Outgoing 'From:' line\n");
	printf("                          [$PILOTFROM is currently: '%s']\n\n", from_address);
	printf("     -k keep|delete       Keep mail on POP server\n");
	printf("                          [$POPKEEP is currently: '%s']\n\n", pop_keep);
	printf("     -d keep|delete|file  Disposition of sent mail\n");
	printf("                          [$PILOTDISPOSE is currently: '%s']\n\n", pilot_dispose);
	printf("     -m mhdir             MH directory to download to Palm\n");
	printf("                          [$TOPILOT_MHDIR is currently: '%s']\n\n", topilot_mhdir);
	printf("     -s command           Sendmail command (if empty, mail won't be sent)\n");
	printf("                          [$SENDMAIL is currently: '%s']\n\n", sendmail);
	printf("   All options may be specified by setting the environment variable named in\n");
	printf("   brackets.\n\n");
	printf("      ********************************************* **\n");
	printf("      ** NOTE!! This is being deprecated soon. This **\n");
	printf("      ** means that this application is going away! **\n");
	printf("      ** Please use pilot-mailsync instead. Consult **\n");
	printf("      ** the manpages for additional information... **\n");
	printf("      ************************************************\n\n");

	exit(0);
}
int main(int argc, char *argv[])
{
	int 	c,		/* switch */
		db,
		sd		= -1,
		i, 
		l 		= 0,
		lost,
		dupe,
		rec,
		sent,
		popfd;

	struct	PilotUser User;
	struct	MailAppInfo tai;
	struct 	MailSyncPref p;
	struct 	MailSignaturePref sig;
	struct 	sockaddr_in pop_addr;
	struct 	hostent *hostent;

	char 	buffer[0xffff],
		*progname = argv[0],

		/* Thanks to Jon Armstrong for helping me with this */
		*port 		= getvars("PILOTPORT", ""),
		*from_address 	= getvars("PILOTFROM", ""),
		*pop_host 	= getvars("POPHOST", ""),
		*pop_user 	= getvars("POPUSER", ""),
		*pop_pass 	= getvars("POPPASS", ""),
		*sendmail 	= getvars("SENDMAIL", "/usr/sbin/sendmail -t -i"),
		*pop_keep 	= getvars("POPKEEP", "keep"),
		*pilot_dispose 	= getvars("PILOTDISPOSE", "keep"),
		*topilot_mhdir 	= getvars("TOPILOT_MHDIR", "");
	
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 's':
			sendmail = optarg;
			break;
		case 'h':
			display_help(progname, port, pop_host, pop_user, pop_pass, 
			     from_address, pop_keep, pilot_dispose, 
			     topilot_mhdir, sendmail);
			break;
		case 'p':
			port = optarg;
			break;
		case 'H':
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
			pilot_dispose = optarg;
			break;
		case 'k':
			pop_keep = optarg;
			break;
		case 'm':
			topilot_mhdir = optarg;
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (port == NULL && getenv("PILOTPORT")) {
		port = getenv("PILOTPORT");
	}
	
	if (!strlen(pop_host) && !strlen(sendmail)) {
		printf("%s requires that at least one of -h or -s must be set.\n\n",
			progname);
	}

	if (strlen(pop_host)) {
		if (!strlen(pop_user)) {
			printf("\nUndeclared option -u must be set to receive mail.\n\n");
		} else if (!strlen(pop_keep)
			   || (strcmp(pop_keep, "keep")
			       && strcmp(pop_keep, "delete"))) {
			printf("-k must have an argument of 'keep' or 'delete'.\n\n");
		}
	}

	if (!strlen(pop_pass) && strlen(pop_host)) {
		pop_pass = getpass("POP password: ");
	}

	if (strlen(sendmail)) {
		if (!strlen(pilot_dispose) || (strcmp(pilot_dispose, "keep") &&
					 strcmp(pilot_dispose, "delete") &&
					 strcmp(pilot_dispose, "file"))) {
			printf("-d must have an argument of 'keep', 'delete', or 'file'.\n\n");
		}
	}

	lost = dupe = rec = sent = 0;


	sd = pilot_connect(port);
	
	memset(&tai, '\0', sizeof(struct MailAppInfo));
	memset(&p, '\0', sizeof(struct MailSyncPref));

	/* Ask the pilot who it is. */
	dlp_ReadUserInfo(sd, &User);

	/* Tell user (via Palm) that we are starting things up */
	dlp_OpenConduit(sd);

	/* Open the Mail database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MailDB", &db) < 0) {
		printf("Unable to open MailDB\n");
		dlp_AddSyncLogEntry(sd, "Unable to open MailDB.\nFile not found.");
		exit(1);
	}

	dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_MailAppInfo(&tai, (unsigned char *)buffer, 0xffff);

	setbuf(stderr, 0);

	p.syncType 	= 0;
	p.getHigh 	= 0;
	p.getContaining = 0;
	p.truncate 	= 8 * 1024;
	p.filterTo 	= 0;
	p.filterFrom 	= 0;
	p.filterSubject = 0;

	if (pi_version(sd) > 0x0100) {
		if (dlp_ReadAppPreference
		    (sd, makelong("mail"), 1, 1, 0xffff, buffer, 0,
		     0) >= 0) {
			printf("Got local backup mail preferences\n");	/* 2 for remote prefs */
			unpack_MailSyncPref(&p, (unsigned char *)buffer, 0xffff);
		} else {
			printf
			    ("Unable to get mail preferences, trying current\n");
			if (dlp_ReadAppPreference
			    (sd, makelong("mail"), 1, 1, 0xffff, buffer, 0,
			     0) >= 0) {
				printf("Got local current mail preferences\n");	/* 2 for remote prefs */
				unpack_MailSyncPref(&p, (unsigned char *)buffer, 0xffff);
			} else
				printf
				    ("Couldn't get any mail preferences.\n");
		}

		if (dlp_ReadAppPreference
		    (sd, makelong("mail"), 3, 1, 0xffff, buffer, 0,
		     0) > 0) {
			unpack_MailSignaturePref(&sig, (unsigned char *)buffer, 0xffff);
		}

	}

	printf
	    ("Local Prefs: Sync=%d, High=%d, getc=%d, trunc=%d, to=|%s|, from=|%s|, subj=|%s|\n",
	     p.syncType, p.getHigh, p.getContaining, p.truncate,
	     p.filterTo ? p.filterTo : "<none>",
	     p.filterFrom ? p.filterFrom : "<none>",
	     p.filterSubject ? p.filterSubject : "<none>");

	printf("AppInfo Signature: |%s|\n\n",
	       sig.signature ? sig.signature : "<None>");

#if 0
	for (i = 0; 1; i++) {
		int 	attr, 
			category;
		
		struct Mail t;

		int len =
		    dlp_ReadRecordByIndex(sd, db, i, buffer, 0, 0, &attr,
					  &category);

		if (len < 0)
			break;

		/* Skip deleted records */
		if ((attr & dlpRecAttrDeleted)
		    || (attr & dlpRecAttrArchived))
			continue;

		unpack_Mail(&t, buffer, len);

		printf("Category: %s\n", tai.CategoryName[category]);
		printf
		    ("Read: %d, Signature: %d, confirmRead: %d, confirmDeliver: %d, priority: %d, address: %d\n",
		     t.read, t.signature, t.confirmRead, t.confirmDelivery,
		     t.priority, t.addressing);
		printf("Time: %s",
		       t.dated ? asctime(&t.date) : "<Undated>");
		printf("Subject: |%s|\n",
		       t.subject ? t.subject : "<None>");
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
			struct 	Mail t;
			int 	attr,
				size;
			recordid_t id;
			FILE *sendf;

			int len =
			    dlp_ReadNextRecInCategory(sd, db, 1, buffer,
						      &id, 0, &size,

						      &attr);

			if (len < 0)
				break;

			/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;

			unpack_Mail(&t, (unsigned char *)buffer, len);

			sendf = popen(sendmail, "w");

			if (!sendf) {
				printf("Error launching '%s' to transmit mail! (No mail lost, %d received, %d sent)\n",
					sendmail, rec, sent);
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
				fprintf(sendf, "Reply-To: %s\n",
					t.replyTo);
			}
			if (t.subject) {
				fprintf(sendf, "Subject: %s\n", t.subject);
			}
			fprintf(sendf, "X-mailer: pilot-mail-%d.%d.%d\n",
				PILOT_LINK_VERSION, PILOT_LINK_MAJOR,
				PILOT_LINK_MINOR);
			fprintf(sendf, "\n");	/* Separate header */

			if (t.body) {
				fputs(t.body, sendf);
				fprintf(sendf, "\n");
			}
			if (t.signature && sig.signature) {
				char *c = sig.signature;

				while ((*c == '\r') || (*c == '\n'))
					c++;
				if (strncmp(c, "--", 2)
				    && strncmp(c, "__", 2)
				    ) {
					fprintf(sendf, "\n-- \n");
				}
				fputs(sig.signature, sendf);
				fprintf(sendf, "\n");
			}

			if (pclose(sendf) == 0) {
				sent++;
				if (strcmp(pilot_dispose, "keep") == 0) {
					dupe++;
				} else if (strcmp(pilot_dispose, "delete") == 0) {
					dlp_DeleteRecord(sd, db, 0, id);
				} else {
					/* Rewrite into Filed category */
					dlp_WriteRecord(sd, db, attr, id,
							3, buffer, size,
							0);
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
				printf("Unable to resolve POP host '%s'. Check your network settings.\n",
					pop_host);
				goto end;
			}
			memcpy((char *) &pop_addr.sin_addr.s_addr,
			       hostent->h_addr, hostent->h_length);
		}
		pop_addr.sin_family = AF_INET;
		pop_addr.sin_port = htons(110);

		if ((popfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("Unable to obtain socket");
			goto end;
		}

		if (connect
		    (popfd, (struct sockaddr *) &pop_addr,
		     sizeof(pop_addr))
		    < 0) {
			printf("Unable to connect to POP server '%s'\n",
				pop_host);
			close(popfd);
			goto end;
		}

		read(popfd, buffer, 1024);
		if (buffer[0] != '+') {
			printf("POP server failed to announce itself\n");
			goto endpop;
		}

		sprintf(buffer, "USER %s\r\n", pop_user);
		write(popfd, buffer, strlen(buffer));
		read(popfd, buffer, 1024);
		if (buffer[0] != '+') {
			printf("USER command to POP server failed. Verify your username.\n");
			goto endpop;
		}

		sprintf(buffer, "PASS %s\r\n", pop_pass);
		write(popfd, buffer, strlen(buffer));
		read(popfd, buffer, 1024);
		if (buffer[0] != '+') {
			printf("PASS command to POP server failed\n. Verify your password.\n");
			close(popfd);
			goto endpop;
		}

		for (i = 1;; i++) {
			int 	len,
				h;
			char 	*msg;

			struct Mail t;

			t.to 		= 0;
			t.from 		= 0;
			t.cc 		= 0;
			t.bcc 		= 0;
			t.subject 	= 0;
			t.replyTo 	= 0;
			t.sentTo 	= 0;
			t.body 		= 0;
			t.dated 	= 0;

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

			msg = (char *)buffer;
			h = 1;
			for (;;) {
				if (getpopstring(popfd, msg) < 0) {
					printf("Error reading message\n");
					goto endpop;
				}

				if (h == 1) {	/* Header mode */
					if ((msg[0] == '.')
					    && (msg[1] == '\n')
					    && (msg[2] == 0)) {
						break;	/* End of message */
					}

					if (msg[0] == '\n') {
						h = 0;
						header(&t, 0);
					} else
						header(&t, msg);
					continue;
				}

				if ((msg[0] == '.') && (msg[1] == '\n')
				    && (msg[2] == 0)) {
					msg[0] = 0;
					break;	/* End of message */
				}

				if (msg[0] == '.') {
					/* Must be escape */
					memmove(msg, msg + 1, strlen(msg));
				}

				msg += strlen(msg);
			}

			/* Well, we've now got the message. I bet _you_ feel
			   happy with yourself. */

			if (h) {
				/* Oops, incomplete message, still reading headers */
				printf("Incomplete message %d\n",
					i);
				free_Mail(&t);
				continue;
			}

			if (strlen(msg) > p.truncate) {
				/* We could truncate it, but we won't for now */
				printf("Message %d too large (%ld bytes)\n",
					i, (long) strlen(msg));
				free_Mail(&t);
				continue;
			}

			t.body = strdup(buffer);

			len = pack_Mail(&t, (unsigned char *)buffer, 0xffff);

			if (dlp_WriteRecord
			    (sd, db, 0, 0, 0, buffer, len, 0) > 0) {
				rec++;
				if (strcmp(pop_keep, "delete") == 0) {
					sprintf(buffer, "DELE %d\r\n", i);
					write(popfd, buffer,
					      strlen(buffer));
					read(popfd, buffer, 1024);
					if (buffer[0] != '+') {
						printf("Error deleting message %d\n", i);
						dupe++;
					}
				} else
					dupe++;
			} else {
				printf("Error writing message to Palm\n");
			}

			free_Mail(&t);
		}

		sprintf(buffer, "QUIT\r\n");
		write(popfd, buffer, strlen(buffer));
		read(popfd, buffer, 1024);
		if (buffer[0] != '+') {
			printf("QUIT command to POP server failed\n");
		}

	      endpop:
		close(popfd);

	}

	if (strlen(topilot_mhdir)) {

		fprintf(stderr, "Reading directory %s... ", topilot_mhdir);
		fflush(stderr);

		/* MH directory reading section */

		for (i = 1;; i++) {
			int 	len,
				mhmsg,
				h;
			char 	*msg;
			struct 	Mail t;

			t.to 		= 0;
			t.from 		= 0;
			t.cc 		= 0;
			t.bcc 		= 0;
			t.subject 	= 0;
			t.replyTo 	= 0;
			t.sentTo 	= 0;
			t.body 		= 0;
			t.dated 	= 0;

			if ((mhmsg = openmhmsg(topilot_mhdir, i)) < 0) {
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
					break;	/* End of message */
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

			/* Well, we've now got the message. I bet _you_ feel
			   happy with yourself. */

			if (h) {
				/* Oops, incomplete message, still reading headers */
				printf("Incomplete message %d\n", i);
				free_Mail(&t);
				continue;
			}

			if (strlen(msg) > p.truncate) {
				/* We could truncate it, but we won't for now */
				printf("Message %d too large (%ld bytes)\n",
					i, (long) strlen(msg));
				free_Mail(&t);
				continue;
			}

			t.body = strdup(msg);

			len = pack_Mail(&t, (unsigned char *)buffer, 0xffff);

			if (dlp_WriteRecord
			    (sd, db, 0, 0, 0, buffer, len, 0) > 0) {
				rec++;
				if (strcmp(pop_keep, "delete") == 0) {
					char filename[1000];

					sprintf(filename, "%s/%d", topilot_mhdir,
						i);
					close(mhmsg);
					if (unlink(filename)) {
						printf("Error deleting message %d\n",
							i);
						dupe++;
					}
					continue;
				} else
					dupe++;
			} else {
				printf("Error writing message to Palm\n");
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

	sprintf(buffer,
		"Finished transferring mail. %d message%s sent, %d message%s received.\n",
		sent, (sent == 1) ? "" : "s", rec, (rec == 1) ? "" : "s");
	if (lost || dupe)
		sprintf(buffer + strlen(buffer),
			"(And %d lost, %d duplicated. Sorry.)\n", lost,
			dupe);
	fprintf(stderr, buffer);
	dlp_AddSyncLogEntry(sd, buffer);

	pi_close(sd);
	return 0;
}
