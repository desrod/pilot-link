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
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "popt.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-appinfo.h"

#undef PRINT_USELESS_CRAP

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)


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

char *contacts_fields[] =
{
	"Last name",
	"First name",
	"Company",
	"Title",
	"Phone 1",
	"Phone 2",
	"Phone 3",
	"Phone 4",
	"Phone 5",
	"Phone 6",
	"Phone 7",
	"Messaging 1",
	"Messaging 2",
	"Website",
	"Custom 1",
	"Custom 2",
	"Custom 3",
	"Custom 4",
	"Custom 5",
	"Custom 6",
	"Custom 7",
	"Custom 8",
	"Custom 9",
	"Address 1",
	"City",
	"State",
	"Zip",
	"Country",
	"Address 2",
	"City",
	"State",
	"Zip",
	"Country",
	"Address 3",
	"City",
	"State",
	"Zip",
	"Country",
	"Note"
};

char *contacts_phones[] =
{
	"Phone (Work)",
	"Phone (Home)",
	"Phone (Fax)",
	"Phone (Other)",
	"Email",
	"Phone (Main)",
	"Phone (Pager)",
	"Phone (Mobile)"
};

char *contacts_ims[] =
{
	"IM",
	"AIM",
	"MSN IM",
	"Yahoo IM",
	"ICQ"
};

char *contacts_addrs[] =
{
	"Addr (Work)",
	"Addr (Home)",
	"Addr (Other)"
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
		printf ("  %08X:", line);
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


void
print_categories (unsigned char *buf, int len)
{
	CategoryAppInfo_t c;
	int i;

	unpack_CategoryAppInfo (&c, buf, len);
	for (i = 0; i < 16; i++)
	{
		if (strlen(c.name[i]) > 0)
			printf (" Category %i: %s\n",
					c.ID[i], c.name[i]);
	}
	printf (" Last Unique ID: %i\n", c.lastUniqueID);

	return;
}


pbooktype_t
print_appblock (int sd, int db)
{
	int i;
	int ofs;
	int clabels;
	unsigned char buf[0xffff];
	unsigned char *data;
	size_t len;
	pbooktype_t dbtype;

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

	printf ("Categories:\n");
	print_categories (data + ofs, 278);
	ofs += 278;

	printf ("Internal data:\n");
	hexprint (data + ofs, 26, ofs, 0);
	ofs += 26;

	/*
	 * Basically, ContactsDB 1.1 adds four field labels.  "Picture" is
	 * inserted into the list at i == 46, and the contents of the picture menu
	 * ("Camera", "Photos", "Remove") are appended to the end of the list,
	 * beginning at i == 50.
	 */
	printf ("Field labels");
	for (i = 0; i < clabels; i++)
	{
		if (i%4 == 0)
			printf ("\n ");
		printf ("%02i:%-16s ", i, (const char *)(data + ofs));
		ofs += 16;
	}

	printf ("\n\nCountry: %hhu\n", *(data + ofs));

	/* Skip a 0 byte */
	ofs += 2;

	printf ("\nSorting: %s\n", data + ofs ? "By company" : "By name");

	/* Skip a 0 byte */
	ofs += 2;

	if (ofs < len)
	{
		/* Should never be true! */
		printf ("\n\nWhatever is left:\n");
		hexprint (data + ofs, len - ofs, ofs, 1);
	}

	puts("");

	return dbtype;

error:

	return db_unknown;
}


void
print_record (int recid, int attr, int category, pi_buffer_t *buf)
{
	size_t ofs = 0;
	int phonelabels[8];
	int addrlabels[3];
	int imlabels[2];
	char *l;
	uint32_t contents1, contents2;
	int i;
	char *s;

	printf ("Category %i, ID 0x%04x", category, (unsigned int)recid);
	if (attr & dlpRecAttrDeleted)
		printf (", deleted");
	if (attr & dlpRecAttrDirty)
		printf (", dirty");
	if (attr & dlpRecAttrBusy)
		printf (", busy");
	if (attr & dlpRecAttrSecret)
		printf (", secret");
	if (attr & dlpRecAttrArchived)
		printf (", archived");
	if (attr == 0)
		printf (", no attributes");
	printf (" (%zu bytes)\n", buf->used);

	if (buf->used == 0)
		return;

	if (buf->used < 17)
		goto broken;

	/*
	 * The first 17 bytes are a header 
	 */
	phonelabels[0] = lo(get_byte(buf->data + 3));
	phonelabels[1] = hi(get_byte(buf->data + 3));
	phonelabels[2] = lo(get_byte(buf->data + 2));
	phonelabels[3] = hi(get_byte(buf->data + 2));
	phonelabels[4] = lo(get_byte(buf->data + 1));
	phonelabels[5] = hi(get_byte(buf->data + 1));
	phonelabels[6] = lo(get_byte(buf->data));
	phonelabels[7] = hi(get_byte(buf->data));

	addrlabels[0] = lo(get_byte(buf->data + 5));
	addrlabels[1] = hi(get_byte(buf->data + 5));
	addrlabels[2] = lo(get_byte(buf->data + 4));
	/* high nybble of data[4] unused */

	imlabels[0] = lo(get_byte(buf->data + 7));
	imlabels[1] = hi(get_byte(buf->data + 7));
	/* data[6] unused */

	contents1 = get_long(buf->data + 8);
	contents2 = get_long(buf->data + 12);

#if PRINT_USELESS_CRAP	
	printf (" Phone labels: { %i, %i, %i, %i, %i, %i, %i } (showing [%i])\n",
			phonelabels[0],
			phonelabels[1],
			phonelabels[2],
			phonelabels[3],
			phonelabels[4],
			phonelabels[5],
			phonelabels[6],
			phonelabels[7]);

	printf (" Address labels: { %i, %i, %i }\n",
			addrlabels[0],
			addrlabels[1],
			addrlabels[2]);

	printf (" IM labels: { %i, %i }\n",
			imlabels[0],
			imlabels[1]);

	printf (" Record contents: 0x%08x 0x%08x\n", contents1, contents2);

	/* + 17 to make it absolute */
	printf (" Offset to Company: 0x%04x\n", get_byte(buf->data + 16) + 17);

#endif /* PRINT_USELESS_CRAP */

	ofs += 17;

	for (i = 0; i < 28; i++)
	{
		if ((contents1 & (1 << i)) != 0)
		{
			/* This isn't safe, should probably be checking record length */
			s = buf->data + ofs;
			ofs += strlen(s) + 1;

			/* Special cases are annoying.. */
			if (i > 3 && i < 11)
				l = contacts_phones[phonelabels[i - 4]];
			else if (i == 11 || i == 12)
				l = contacts_ims[imlabels[i - 11]];
			else if (i == 23)
				l = contacts_addrs[addrlabels[i - 23]];
			else
				l = contacts_fields[i];

			printf (" %-15s: %s\n", l, s);

			contents1 ^= (1 << i);
		}
	}

	for (i = 0; i < 11; i++)
	{
		if ((contents2 & (1 << i)) != 0)
		{
			/* This isn't safe, should probably be checking record length */
			s = buf->data + ofs;
			ofs += strlen(s) + 1;
			if (i == 0 || i == 5)
				l = contacts_addrs[addrlabels[i==0? 1: 2]];
			else
				l = contacts_fields[i + 28];

			printf (" %-15s: %s\n", l, s);

			contents2 ^= (1 << i);
		}
	}

	if (contents2 & 0x1800)
	{
		uint16_t bday = get_short(buf->data + ofs);

		ofs += 4;

		printf (" Birthday       : %i-%02i-%02i",
				((bday & 0xfe00) >> 9) + 1904,
				((bday & 0x01e0) >> 4) - 1,
				bday & 0x001f);

		/* if this is set without a date, it should be caught below */
		if (contents2 & 0x2000)
		{
			int reminder = (int)get_byte(buf->data + ofs);

			ofs++;
			printf (" (%i day reminder)", reminder);
			contents2 ^= 0x2000;
		}
		puts ("");

		contents2 ^= 0x1800;
	}

	/* we cleared all of the bits we recognize */
	if (contents1 != 0 || contents2 != 0)
		goto broken;

	if (ofs < buf->used)
	{
		/* Under Contacts 1.0, this is probably actually an error */
		printf (" Picture        : %zu bytes\n", buf->used - ofs);
	}

	puts ("");

	return;

broken:

	/* Something's wrong, print the whole record */
	puts ("Broken/unrecognized record:");
	hexprint (buf->data, buf->used, 0, 1);
	puts("");
	return;
}


int
main (const int argc, const char **argv)
{
	int sd = -1;
	int db;
	pbooktype_t dbtype;
	int i;
	int l;
	recordid_t recid;
	int attr;
	int category;
	pi_buffer_t *buf;

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

	dbtype = print_appblock(sd, db);


	buf = pi_buffer_new (0xffff);
	for (i = 0; /**/; i++)
	{
		l = dlp_ReadRecordByIndex (sd, db, i, buf, &recid, &attr, &category);
		if (l < 0)
			break;

		print_record(recid, attr, category, buf);
	}
	pi_buffer_free (buf);

	dlp_CloseDB(sd, db);
	dlp_EndOfSync (sd, 0);
	pi_close (sd);

error:
	if (sd != -1)
		pi_close (sd);
	return 1;
}
