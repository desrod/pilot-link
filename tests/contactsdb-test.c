/*
 * contactsdv-test.c:  Playing around with ContactsDB
 *
 * Written by  T. Joseph Carter <knghtbrd@bluecherry.net>
 *
 * This program is released to the public domain, except in countries such as
 * the United States where one cannot legally do so.  In such places, consider
 * this code to be licensed such that you may use it as if it were.
 *
 * This software is released without any warranty whatsoever in the hopes that
 * someone will figure out the damned ContactsDB format, or that someone from
 * PalmOne will write me an email and make sense of the output for me.  You
 * won't find enlightenment reading this code, only my frustration.  If it
 * breaks, it's not my problem.  If it breaks your PDA, I'm impressed since it
 * doesn't write anything--but it's still not my problem.  =)  You've been
 * duly warned.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "popt.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"


typedef enum
{
	db_unknown = 0,
	db_contacts10,
	db_contacts11
}
pbooktype_t;

const char *pbooknames[] =
{
	"Unknown version",
	"Contacts 1.0",
	"Contacts 1.1/1.2"
};


/* This is ugly, but it works */
void
hexprint (unsigned char *data, size_t len, size_t ofs, int ascii)
{
	int i, j;
	int line;

	line = ofs;
	i = 0;

	while (i < len)
	{
		printf (" %08X:", line);
		for (j = 0; j < 16; j++, i++)
		{
			if (i < len)
				printf (" %02X", data[i]);
			else
				printf ("   ");
			if (j == 7)
				printf (" ");
		}
		if (ascii)
		{
			printf (" |");
			i -= 16;
			for (j = 0; j < 16; j++, i++)
			{
				if (i < len)
				{
					if (isprint(data[i]))
						printf ("%c", data[i]);
					else
						printf (".");
				}
				else
					printf (" ");
			}
			printf ("|\n");
		}
		else
			printf ("\n");
		line += 16;
	}
}


int
main (const int argc, const char **argv)
{
	int i;
	int ofs;
	int sd = -1;
	int db;
	int clabels;
	unsigned char buf[0xffff];
	unsigned char *data;
	size_t len;
	pbooktype_t dbtype;

	if (argc != 2)
	{
		fprintf (stderr, "Usage: contactsdb-test <port>\n");
		return 1;
	}

	sd = pilot_connect (argv[1]);

	if (sd < 0)
		goto error;

	if (dlp_OpenConduit (sd) < 0)
		goto error;
	
	if (dlp_OpenDB(sd, 0, dlpOpenRead, "ContactsDB-PAdd", &db) < 0)
	{
		fprintf (stderr, "Unable to open Contacts database\n");
		dlp_AddSyncLogEntry (sd, "Unable to open ContactsDB.\n");
		goto error;
	}
	
	i = dlp_ReadAppBlock(sd, db, 0, buf, sizeof(buf));
	data = (unsigned char *)&buf;

	if (i <= 0)
		goto error;
	else
		len = i;

	if (len == 1092)
	{
		dbtype = db_contacts10;
		clabels = 49;
	}
	else if (len == 1156)
	{
		dbtype = db_contacts11;
		clabels = 53;
	}
	else
		dbtype = db_unknown;

	printf ("Data Length: %zu (%s)\n", len, pbooknames[dbtype]);

	if (dbtype == db_unknown || len < 278+26)
		/* something's wrong */
		goto error;

	printf ("Skipping Categories (278 bytes)\n");
	len -= 278;
	ofs += 278;
	data += 278;

	printf ("Unknown data (internal flags?)\n");
	hexprint (data, 26, ofs, 0);
	len -= 26;
	data += 26;
	ofs += 26;

	printf ("Field labels");
	for (i = 0; i < clabels; i++)
	{
		if (i%4 == 0)
			printf ("\n ");
		printf ("%02i:%-16s ", i, (const char *)data);
		ofs += 16;
		len -= 16;
		data += 16;
	}

	printf ("\n\nCountry: %hhu\n", data[0]);

	/* Skip a 0 byte */
	data += 2;
	ofs += 2;
	len -= 2;

	printf ("\nSorting: %s\n", data[0] ? "By company" : "By name");

	/* Skip a 0 byte */
	data += 2;
	ofs += 2;
	len -= 2;

	if (len > 0)
	{
		/* Should never be true! */
		printf ("\n\nWhatever is left:\n");
		hexprint (data, len, ofs, 1);
	}

	dlp_CloseDB(sd, db);
	dlp_EndOfSync (sd, 0);
	pi_close (sd);

error:
	if (sd != -1)
		pi_close (sd);
	return 1;
}
