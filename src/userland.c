/*
 * userland.c: General definitions for userland conduits.
 *
 * Copyright (C) 2004 by Adriaan de Groot <groot@kde.org>
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

#include "pi-userland.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pi-header.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-source.h"

static const char *env_pilotport = "PILOTPORT";

int plu_connect()
{
	int sd = -1;
	int result;

	struct  SysInfo sys_info;

	/* Determine here for better user feedback on unset port */
	if (plu_port == NULL)
		plu_port = getenv(env_pilotport);
	if (plu_port == NULL) {
		fprintf(stderr, "\n   Unable to determine port to bind\n"
				"   Please use --help for more information\n\n");
		return -1;
	}

	if ((sd = pi_socket(PI_AF_PILOT,
			PI_SOCK_STREAM, PI_PF_DLP)) < 0) {
		fprintf(stderr, "\n   Unable to create socket '%s'\n", plu_port);
		return -1;
	}

	result = pi_bind(sd, plu_port);

	if (result < 0) {
		fprintf(stderr, "   Unable to bind to port: %s\n"
				"   Please use --help for more information\n\n",
				plu_port);
		return result;
	}

	if (!plu_quiet && isatty(fileno(stdout))) {
		printf("\n   Listening to port: %s\n\n"
			"   Please press the HotSync button now... ",
			plu_port);
		fflush(stdout);
	}

	if (pi_listen(sd, 1) < 0) {
		fprintf(stderr, "\n   Error listening on %s\n", plu_port);
		pi_close(sd);
		return -1;
	}

	sd = pi_accept(sd, 0, 0);
	if (sd < 0) {
		fprintf(stderr, "\n   Error accepting data on %s\n", plu_port);
		pi_close(sd);
		return -1;
	}

	if (!plu_quiet && isatty(fileno(stdout))) {
		printf("connected!\n\n");
	}

	if (dlp_ReadSysInfo(sd, &sys_info) < 0) {
		fprintf(stderr, "\n   Error read system info on %s\n", plu_port);
		pi_close(sd);
		return -1;
	}

	dlp_OpenConduit(sd);
	return sd;
}


int plu_findcategory(const struct CategoryAppInfo *info, const char *name, int flags)
{
	int index, match_category;

	match_category = -1;
	for (index = 0; index < 16; index += 1) {
		if (info->name[index][0]) {
			if (flags & PLU_CAT_CASE_INSENSITIVE) {
				if (strncasecmp(info->name[index], name, 15) == 0) {
					match_category = index;
					break;
				}
			} else {
				if (strncmp(info->name[index],name,15) == 0) {
					match_category = index;
					break;
				}
			}
		}
	}

	if ((match_category == -1)  && (flags & PLU_CAT_MATCH_NUMBERS)) {
		while (isspace(*name)) {
			name++;
		}
		if (isdigit(*name)) {
			match_category = atoi(name);
		}

		if ((match_category < 0) || (match_category > 15)) {
			match_category = -1;
		}
	}

	if (flags & PLU_CAT_WARN_UNKNOWN) {
		if (match_category == -1) {
			fprintf(stderr,"   WARNING: Unknown category '%s'%s.\n",
				name,
				(flags & PLU_CAT_DEFAULT_UNFILED) ? ", using 'Unfiled'" : "");
		}
	}

	if (flags & PLU_CAT_DEFAULT_UNFILED) {
		if (match_category == -1) {
			match_category = 0;
		}
	}

	return match_category;
}


int plu_protect_files(char *name, const char *extension, const size_t namelength)
{
	char *save_name;
	char c=1;

	save_name = strdup( name );
	if (NULL == save_name) {
		fprintf(stderr,"   ERROR: No memory for filename %s%s\n",name,extension);
		return -1;
	}

	/* 4 byes = _%02d and terminating NUL */
	if (strlen(save_name) + strlen(extension) + 4 > namelength) {
		fprintf(stderr,"   ERROR: Buffer for filename too small.\n");
		free(save_name);
		return -1;
	}

	snprintf(name,namelength,"%s%s",save_name,extension);

	while ( access( name, F_OK ) == 0) {
		snprintf( name, namelength, "%s_%02d%s", save_name, c, extension);
		c++;

		if (c >= 100) {
			fprintf(stderr,"   ERROR: Could not generate filename (tried 100 times).\n");
			free(save_name);
			return 0;
		}
	}

	free(save_name);
	return 1;
}


int plu_getromversion(int sd, plu_romversion_t *d)
{
	unsigned long ROMversion;

	if ((sd < 0)  || !d) {
		return -1;
	}

	if (dlp_ReadFeature(sd, makelong("psys"), 1, &ROMversion) < 0) {
		return -1;
	}

	d->major =
		(((ROMversion >> 28) & 0xf) * 10) + ((ROMversion >> 24) & 0xf);
	d->minor = ((ROMversion >> 20) & 0xf);
	d->bugfix = ((ROMversion >> 16) & 0xf);
	d->state = ((ROMversion >> 12) & 0xf);
	d->build =
		(((ROMversion >> 8) & 0xf) * 100) +
		(((ROMversion >> 4) & 0xf) * 10) + (ROMversion & 0xf);

	memset(d->name,0,sizeof(d->name));
	snprintf(d->name, sizeof(d->name),"%d.%d.%d", d->major, d->minor, d->bugfix);

	if (d->state != 3) {
		int len = strlen(d->name);
		snprintf(d->name + len, sizeof(d->name) - len,"-%s%d", (
			(d->state == 0) ? "d" :
			(d->state == 1) ? "a" :
			(d->state == 2) ? "b" : "u"), d->build);
	}

	return 0;
}

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
