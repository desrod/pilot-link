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

#include "userland.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "pi-header.h"
#include "pi-socket.h"
#include "pi-dlp.h"

static char *port = NULL;
int userland_quiet = 0;

static void callback(poptContext pc,
	int reason,
	const struct poptOption *opt,
	const char *arg,
	void *data)
{
	switch(opt->val) {
	case 'v' :
		print_splash(poptGetInvocationName(pc));
		exit(0);
		break;
	}
}

struct poptOption userland_common_options[] = {
	{ NULL,0,POPT_ARG_CALLBACK,callback,0,NULL,NULL},
	{ "port",    'p', POPT_ARG_STRING, &port,  0 , "Use device <port> to communicate with Palm", "<port>"},
	{ "version",  0 , POPT_ARG_NONE,    NULL, 'v', "Display version information", NULL},
	{ "quiet",   'q', POPT_ARG_NONE,  &userland_quiet,  0 , "Suppress 'Hit HotSync button' message", NULL},
	POPT_TABLEEND
} ;

int userland_connect()
{
	int sd = -1;
	int result;

	struct  SysInfo sys_info;

	if ((sd = pi_socket(PI_AF_PILOT,
			PI_SOCK_STREAM, PI_PF_DLP)) < 0) {
		fprintf(stderr, "\n   Unable to create socket '%s'\n", port);
		return -1;
	}

	result = pi_bind(sd, port);

	if (result < 0) {
		fprintf(stderr, "\n   Unable to bind to port: %s\n",
				port);

		fprintf(stderr, "   Please use --help for more "
				"information\n\n");
		return result;
	}

	if (!userland_quiet && isatty(fileno(stdout))) {
		printf("\n   Listening to port: %s\n\n"
			"   Please press the HotSync button now... ",
			port);
	}

	if (pi_listen(sd, 1) < 0) {
		fprintf(stderr, "\n   Error listening on %s\n", port);
		pi_close(sd);
		return -1;
	}

	sd = pi_accept(sd, 0, 0);
	if (sd < 0) {
		fprintf(stderr, "\n   Error accepting data on %s\n", port);
		pi_close(sd);
		return -1;
	}

	if (!userland_quiet && isatty(fileno(stdout))) {
		printf("connected!\n\n");
	}

	if (dlp_ReadSysInfo(sd, &sys_info) < 0) {
		fprintf(stderr, "\n   Error read system info on %s\n", port);
		pi_close(sd);
		return -1;
	}

	dlp_OpenConduit(sd);
	return sd;
}


void userland_badoption(poptContext pc, int optc)
{
	fprintf(stderr, "%s: %s\n",
		poptBadOption(pc, POPT_BADOPTION_NOALIAS),
		poptStrerror(optc));
	exit(1);
}

int userland_findcategory(const struct CategoryAppInfo *info, const char *name)
{
	int index, match_category;

	match_category = -1;
	for (index = 0; index < 16; index += 1) {
		if ((info->name[index][0]) &&
			(strncasecmp(info->name[index], name, 15) == 0)) {
			match_category = index;
			break;
		}
	}

	return match_category;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
