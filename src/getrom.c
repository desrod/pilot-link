/*
 * getrom:  Fetch ROM image for CoPilot or POSE
 *
 * Copyright (c) Kenneth Albanowski, based on code by Greg Hewgill
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
#include <termios.h>

#include "pi-source.h"
#include "pi-serial.h"
#include "pi-header.h"

/* Declare prototypes */
static void display_help(char *progname);
void print_splash(char *progname);
int pilot_connect(char *port);

void display_help(char *progname)
{
	fprintf(stderr, "   Usage: %s [-2] %s\n\n", progname, TTYPrompt);
	exit(2);
}

struct record *records = 0;

int main(int argc, char *argv[])
{
	int 	l,
		p,	
		rom,
		version = 1,
		max,
		i;
	
	char 	buf[0xffff],
		*progname = argv[0],
		*port = argv[1];

	unsigned char check;
	
	struct 	pi_sockaddr addr;
	struct 	pi_socket ps;

	extern 	char *optarg;
	extern 	int optind;
	
	if (argc < 2)
		display_help(progname);

	if (strcmp(argv[1], "-2") == 0) {
		version = 2;
		argv++;
		argc--;
	}

	addr.pi_family = PI_AF_PILOT;
	strcpy(addr.pi_device, port);

	ps.mac = calloc(1, sizeof(struct pi_mac));

	ps.rate = 38400;
	ps.sd = 0;
	if (pi_serial_open(&ps, &addr, sizeof(addr)) == -1) {
		fprintf(stderr, "   Unable to open port %s\n\n", port);
		exit(0);
	}

	rom =
	    open((version == 2) ? "pilot2.rom" : "pilot.rom",
		 O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (rom == -1) {
		perror("Unable to create pilot.rom");
		exit(0);
	}

	printf("   NOTICE: Use of this program may place you in violation\n"
	"           of your license agreement with Palm Computing.\n\n"
	"           Please read your Palm Computing handbook (\"Software\n"
	"           License Agreement\") before running this program.\n\n"
	"   Please launch %s on your Palm device.\n"
	"   Port: %s\n\n   Please press the HotSync button...\n", 
		(version == 2) ? "Getrom2.prc" : "Getrom.prc", argv[1]);

	max = (version == 2) ? 256 : 128;
	for (i = 0; i < max; i++) {
		do {
			l = read(ps.mac->fd, buf, 1);
			if (l < 1)
				continue;
		} while (buf[0] != '*');
		printf("\r%d/%d", i + 1, max);
		fflush(stdout);

		p = 0;
		do {
			l = read(ps.mac->fd, buf + p, 4096 - p);
			if (l < 0) {
				perror("Unable to read sync byte");
			}
			p += l;
		} while (p < 4096);

		check = 0xff;
		if (read(ps.mac->fd, buf + 4096, 1) < 0) {
			perror("Unable to read checksum byte");
			goto error;
		}

		for (p = 0; p < 4096; p++)
			check = ((check << 1) | check >> 7) ^ buf[p];

		if ((unsigned char) buf[4096] != check) {
			printf("\nChecksum error\n");
			goto error;
		}

		write(rom, buf, 4096);

		buf[4096] = '+';

		write(ps.mac->fd, buf + 4096, 1);
	}
	printf("\nSuccessful!\n");

      error:
	close(rom);

	ps.serial_close(&ps);

	return 0;
}
