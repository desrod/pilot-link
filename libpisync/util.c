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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
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
 * Summary:     Convert from Palm supported encoding to a supported 
 *		desktop text encoding.  The Palm charset is assumed to
 *		be CP1252.  This default can be overridden by the
 *		'PILOT_CHARSET' environment variable.
 *		It is more efficient to use 'convert_ToPilotCharWithCharset'
 *		if the pilot charset is known.
 *
 * Parameters:
 *		charset		iconv-recognised destination charset
 *		text		multibyte sequence in desktop charset
 *		bytes		maximum number of bytes from 'text' to convert
 *		ptext (output)	on success, 'ptext' will point to newly
 *				allocated null-terminated array of converted
 *				string.
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_ToPilotChar(const char *charset, const char *text,
		    int bytes, char **ptext)
{
#ifdef HAVE_ICONV
	char*   pcharset;

	if ((pcharset = getenv("PILOT_CHARSET")) == NULL)
		pcharset = PILOT_CHARSET;
	return convert_ToPilotChar_WithCharset(charset, text, bytes,
	    ptext, pcharset);
#else
	return -1;
#endif
}


/***********************************************************************
 *
 * Function:    convert_ToPilotChar_WithCharset
 *
 * Summary:     Convert any supported desktop text encoding to the
 *		specified Palm supported encoding
 *
 * Summary:     Convert from Palm supported encoding to a supported 
 *		desktop text encoding.  The Palm charset is explicity
 *		specified.
 *
 * Parameters:
 *		charset		iconv-recognised destination charset
 *		text		multibyte sequence in desktop charset
 *		bytes		maximum number of bytes from 'text' to convert
 *		ptext (output)	on success, 'ptext' will point to newly
 *				allocated null-terminated array of converted
 *				string.
 *		pi_charset	iconv-recognised pilot-charset identifier
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_ToPilotChar_WithCharset(const char *charset, const char *text,
		    int bytes, char **ptext, const char * pi_charset)
{
#ifdef HAVE_ICONV
	char*	ob;
	iconv_t cd;
	size_t 	ibl, obl;

	if(NULL==pi_charset){
		pi_charset = PILOT_CHARSET;
	}

	cd = iconv_open(pi_charset, charset);
	if (cd==(iconv_t)-1)
		return -1;

	ibl 	= bytes;
	obl 	= bytes * 4 + 1;
	*ptext 	= ob = malloc(obl);
	
	if (iconv(cd, &text, &ibl, &ob, &obl) == (size_t)-1)
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
 *		desktop text encoding.  The Palm charset is assumed to
 *		be CP1252.  This default can be overridden by the
 *		'PILOT_CHARSET' environment variable.
 *		It is more efficient to use 'convert_FromPilotCharWithCharset'
 *		if the pilot charset is known.
 *
 * Parameters:
 *		charset		iconv-recognised destination charset
 *		ptext		multibyte sequence in pilot's charset
 *		bytes		maximum number of bytes from ptext to convert
 *		text (output)	on success, 'text' will point to newly
 *				allocated null-terminated array of converted
 *				string.
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_FromPilotChar(const char *charset, const char *ptext,
		      int bytes, char **text)
{
#ifdef HAVE_ICONV
	char*   pcharset;

	if ((pcharset = getenv("PILOT_CHARSET")) == NULL)
		pcharset = PILOT_CHARSET;
	return convert_FromPilotChar_WithCharset(charset, ptext, bytes,
	    text, pcharset);
#else
	return -1;
#endif
}


/***********************************************************************
 *
 * Function:    convert_FromPilotChar_WithCharset
 *
 * Summary:     Convert from specified Palm supported encoding to a supported 
 *		desktop text encoding
 *
 * Parameters:
 *		charset		iconv-recognised destination charset
 *		ptext		multibyte sequence in pilot's charset
 *		bytes		maximum number of bytes from ptext to convert
 *		text (output)	on success, 'text' will point to newly
 *				allocated null-terminated array of converted
 *				string.
 *		pi_charset	iconv-recognised pilot-charset identifier
 *
 * Returns:     0 on success, -1 on failure
 *
 ***********************************************************************/
int
convert_FromPilotChar_WithCharset(const char *charset, const char *ptext,
		      int bytes, char **text, const char * pi_charset)
{
#ifdef HAVE_ICONV
	char*	ob;
	iconv_t cd;
	size_t 	ibl, obl;

	if(NULL==pi_charset){
		pi_charset = PILOT_CHARSET;
	}

	cd = iconv_open(charset, pi_charset);
	if (cd==(iconv_t)-1)
		return -1;

	ibl 	= bytes;
	obl 	= bytes * 4 + 1;
	*text 	= ob = malloc(obl);
	
	if (iconv(cd, &ptext, &ibl, &ob, &obl) == (size_t)-1)
		return -1;
	*ob = '\0';

	iconv_close(cd);

	return 0;
#else
	return -1;
#endif
}
/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
