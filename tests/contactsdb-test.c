/*
 * contactsdb-test.c:  Playing around with palmOne's ContactsDB-PAdd
 *
 * Written by  T. Joseph Carter <knghtbrd@bluecherry.net>
 *
 * This program is released to the public domain where allowed by law.  Some
 * countries (eg, the United States) lack legal provision for releasing
 * Copyright claims.  In such jurisdictions you are licensed to use, modify,
 * and distribute this program as you see fit.  This program is released
 * WITHOUT ANY WARRANTY, including the implied warranties of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * These terms do NOT apply to libpisock, which is covered by the GNU Library
 * General Public License.  Please see the GNU LGPL for details.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "popt.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-appinfo.h"
#include "pi-contact.h"

#undef PRINT_USELESS_CRAP
#undef SAVE_PICTURES

#define hi(x) (((x) >> 4) & 0x0f)
#define lo(x) ((x) & 0x0f)

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


int
print_appblock (int sd, int db, struct ContactAppInfo *cai)
{
	int i;
	size_t j;
	pi_buffer_t *appblock = pi_buffer_new(0xffff);

	if (dlp_ReadAppBlock(sd, db, 0, -1, appblock) <= 0)
		goto error;

	if (unpack_ContactAppInfo (cai, appblock) < 0)
		goto error;

	pi_buffer_free (appblock);

	printf ("Categories:\n");
	for (i = 0; i < 16; i++)
	{
		if (strlen(cai->category.name[i]) > 0)
			printf (" Category %i: %s\n",
					cai->category.ID[i], cai->category.name[i]);
	}
	printf (" Last Unique ID: %i\n", cai->category.lastUniqueID);

	printf ("\nCustom labels");
	for (i = 0; i < cai->numCustoms; i++)
	{
		if (i%4 == 0)
			printf ("\n ");
		printf ("%02i:%-16s ", i, cai->customLabels[i]);
	}
	if (i%4 != 0)
		printf ("\n");

	/* Fair enough for a test to print opaque data.. */
	if (cai->internal != NULL) {
		printf ("\nInternal data (opaque)\n");
		hexprint (cai->internal->data, cai->internal->used, 0, 0);
	}

	if (cai->labels != NULL && cai->labels->used % 16 == 0) {
		printf ("\nField labels (also \"opaque\")");
		i = 0;
		for (j = 0; j < cai->labels->used; j += 16)
		{
			if (i%4 == 0)
				printf ("\n ");
			printf ("%02i:%-16s ", i++, cai->labels->data + j);
		}
		if (i%4 != 0)
			printf ("\n");
	}
	
	printf ("\nCountry: %i\n", cai->country);

	printf ("Sorting: %s\n\n", cai->sortByCompany ? "By company" : "By name");

	return 0;

error:
	pi_buffer_free(appblock);
	return -1;
}


void
print_records (int sd, int db, struct ContactAppInfo *cai)
{
	char *l;
	struct Contact c;
	int i, idx, attr, category;
	recordid_t recid;
	pi_buffer_t *buf;

	buf = pi_buffer_new (0xffff);
	
	for (idx = 0; /**/; idx++) {
		if (dlp_ReadRecordByIndex (sd, db, idx, buf,
					&recid, &attr, &category) < 0)
			break;

		printf ("\nRecord 0x%04x\n", (unsigned int)recid);
		printf (" Category       : %i\n", category);
		printf (" Attributes     : %c%c%c%c%c\n",
				attr & dlpRecAttrDeleted	? 'X' : '-',
				attr & dlpRecAttrArchived	? 'A' : '-',
				attr & dlpRecAttrSecret		? 'S' : '-',
				attr & dlpRecAttrBusy		? 'B' : '-',
				attr & dlpRecAttrDirty		? 'd' : '-');
		printf (" Length         : %zu\n", buf->used);

		if (buf->used == 0)
			/* Empty records are legal */
			continue;

		if (unpack_Contact (&c, buf, cai->type) < 0) {
			printf (" [Broken record]\n");
			continue;
		}

#if PRINT_USELESS_CRAP	
		printf (" Phone labels: { %i, %i, %i, %i, %i, %i, %i }"
				" (showing [%i])\n",
				c.phoneLabel[0],
				c.phoneLabel[1],
				c.phoneLabel[2],
				c.phoneLabel[3],
				c.phoneLabel[4],
				c.phoneLabel[5],
				c.phoneLabel[6],
				c.showPhone);
		printf (" Address labels: { %i, %i, %i }\n",
				c.addressLabel[0],
				c.addressLabel[1],
				c.addressLabel[2]);
		printf (" IM labels: { %i, %i }\n",
				c.IMLabel[0],
				c.IMLabel[1]);
#endif /* PRINT_USELESS_CRAP */

		for (i = 0; i < NUM_CONTACT_ENTRIES; i++) {
			if (c.entry[i] != NULL) {
				/* Special cases are annoying.. */
				if (i > 3 && i < 11)
					l = contacts_phones[c.phoneLabel[i - 4]];
				else if (i == 11 || i == 12)
					l = contacts_ims[c.IMLabel[i - 11]];
				else if (i == 23)
					l = contacts_addrs[c.addressLabel[0]];
				else if (i == 28)
					l = contacts_addrs[c.addressLabel[1]];
				else if (i == 33)
					l = contacts_addrs[c.addressLabel[2]];
				else
					l = contacts_fields[i];

				printf (" %-15s: %s\n", l, c.entry[i]);
			}
		}
		
		if (c.birthdayFlag) {
			printf (" Birthday       : %i-%02i-%02i",
					c.birthday.tm_year + 1900,
					c.birthday.tm_mon + 1,
					c.birthday.tm_mday);

			if (c.reminder != -1)
				printf (" (%i day reminder)", c.reminder);
			puts ("");
		}

		if (c.picture != NULL) {
#ifdef SAVE_PICTURES
			char fname[25];
			FILE *f;

			snprintf (fname, 24, "rec-%lu.%s",
					(uint32_t)recid,
					c.pictype == cpic_jpeg ? "jpeg" : "img");
			printf (" Picture        : %s\n", fname);
			f = fopen (fname, "wb");
			if (f) {
				fwrite (c.picture->data, c.picture->used, 1, f);
				fclose (f);
			}
#else
			printf (" Picture        : %s (%zu bytes)\n",
					c.pictype == cpic_jpeg ? "JPEG" : "Unknown format",
					c.picture->used);
#endif /* SAVE_PICTURES */
		}

		free_Contact (&c);
	}
	pi_buffer_free (buf);

	return;
}


int
main (const int argc, const char **argv)
{
	int sd = -1;
	int db;
	struct ContactAppInfo cai;

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
		goto error;
	}

	if (print_appblock (sd, db, &cai) < 0) {
		fprintf (stderr, "Failed read/print appblock\n");
		goto error;
	} else {
		print_records (sd, db, &cai);
	}
	free_ContactsAppInfo (&cai);

	dlp_CloseDB(sd, db);
	dlp_EndOfSync (sd, 0);
	pi_close (sd);

error:
	if (sd != -1)
		pi_close (sd);
	return 1;
}
