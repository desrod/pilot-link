/*
 * sync.c:  Implement generic synchronization algorithm
 *
 * Copyright (c) 2000, Helix Code Inc.
 *
 * Author: JP Rosevear <jpr@helixcode.com> 
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
 */

#include <stdlib.h>
#include <string.h>
#include "pi-config.h"
#include "pi-util.h"

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#define PILOT_CHARSET "CP1252"

/***********************************************************************
 *
 * Function:    convert_ToPilotChar
 *
 * Summary:     Convert any supported desktop text encoding to the Palm
 *		supported encoding
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_ToPilotChar(const char *charset, const char *text,
		    int bytes, char **ptext)
{
#ifdef HAVE_ICONV
	iconv_t cd;
	char *ib, *ob;
	size_t ibl, obl;

	cd = iconv_open(PILOT_CHARSET, charset);
	if (!cd)
		return -1;

	ibl = bytes;
	obl = bytes * 4 + 1;
	ib = strdup(text);
	*ptext = ob = malloc(obl);
	if (iconv(cd, &ib, &ibl, &ob, &obl) == -1)
		return -1;
	*ob = '\0';

	iconv_close(cd);

	return 0;
#else
	return -1;
#endif
}

/***********************************************************************
 *
 * Function:    convert_FromPilotChar
 *
 * Summary:     Convert from Palm supported encoding to a supported 
 *		desktop text encoding
 *
 * Parmeters:   None
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_FromPilotChar(const char *charset, const char *ptext,
		      int bytes, char **text)
{
#ifdef HAVE_ICONV
	iconv_t cd;
	char *ib, *ob;
	size_t ibl, obl;

	cd = iconv_open(charset, PILOT_CHARSET);
	if (!cd)
		return -1;

	ibl = bytes;
	obl = bytes * 4 + 1;
	ib = strdup(ptext);
	*text = ob = malloc(obl);
	if (iconv(cd, &ib, &ibl, &ob, &obl) == -1)
		return -1;
	*ob = '\0';

	iconv_close(cd);

	return 0;
#else
	return -1;
#endif
}
