/*
 * utils.c:  misc. stuff for dealing with packets.
 *
 * Portions Copyright (c) 1996, D. Jeff Dionne.
 * Portions Copyright (c) 1996, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#ifdef NeXT
# include <stdlib.h>
# include <string.h>
# include <assert.h>
#endif

#ifdef WIN32
#include <winsock.h>
#include <assert.h>
#else
#ifndef HAVE_INET_ATON
# include <sys/param.h>
#ifdef __EMX__
# include <sys/types.h>
# define INCL_DOSFILEMGR	/* File System values */
# define INCL_MISC
# include <os2.h>
#endif
# include <netinet/in.h>
# include <arpa/inet.h>
# include <ctype.h>
#endif
#endif

#include "pi-source.h"
#include "pi-socket.h"

/* this routine ruthlessly stolen verbatim from Brian J. Swetland */

/***********************************************************************
 *
 * Function:    crc16
 *
 * Summary:     Implementation of the CRC16 Cyclic Redundancy Check
 *
 * Parmeters:   None
 *
 * Returns:     CRC + NULL
 *
 ***********************************************************************/
int crc16(unsigned char *ptr, int count)
{
	int crc, i;

	crc = 0;
	while (--count >= 0) {
		crc = crc ^ (int) *ptr++ << 8;
		for (i = 0; i < 8; ++i)
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc = crc << 1;
	}
	return (crc & 0xFFFF);
}

#ifndef HAVE_STRDUP
/***********************************************************************
 *
 * Function:    strdup
 *
 * Summary:     Duplicate a string
 *
 * Parmeters:   None
 *
 * Returns:     String or NULL
 *
 ***********************************************************************/
char *strdup(const char *string)
{
	size_t length;
	char *result;

	assert(string != NULL);

	length = strlen(string) + 1;
	result = malloc(length);

	if (result == NULL)
		return NULL;

	memcpy(result, string, length);

	return result;
}
#endif

#ifndef HAVE_PUTENV

/* Borrowed from GNU sh-utils, and then probably from GNU libc */

#if HAVE_GNU_LD
# define environ __environ
#else
extern char **environ;
#endif

/***********************************************************************
 *
 * Function:    putenv
 *
 * Summary:     Put STRING, which is of the form "NAME=VALUE", in the
 *		environment
 *
 * Parmeters:   None
 *
 * Returns:     0 for success, nonzero if any errors occur
 *
 ***********************************************************************/
int putenv(const char *string)
{
	const char *const name_end = strchr(string, '=');
	register size_t size;
	register char **ep;

	if (name_end == NULL) {
		/* Remove the variable from the environment.  */
		size = strlen(string);
		for (ep = environ; *ep != NULL; ++ep)
			if (!strncmp(*ep, string, size)
			    && (*ep)[size] == '=') {
				while (ep[1] != NULL) {
					ep[0] = ep[1];
					++ep;
				}
				*ep = NULL;
				return 0;
			}
	}

	size = 0;
	for (ep = environ; *ep != NULL; ++ep)
		if (!strncmp(*ep, string, name_end - string)
		    && (*ep)[name_end - string] == '=')
			break;
		else
			++size;

	if (*ep == NULL) {
		static char **last_environ = NULL;
		char **new_environ =
		    (char **) malloc((size + 2) * sizeof(char *));

		if (new_environ == NULL)
			return -1;
		(void) memcpy((void *) new_environ, (void *) environ,
			      size * sizeof(char *));

		new_environ[size] = (char *) string;
		new_environ[size + 1] = NULL;
		if (last_environ != NULL)
			free((void *) last_environ);
		last_environ = new_environ;
		environ = new_environ;
	} else
		*ep = (char *) string;

	return 0;
}
#endif

#ifndef HAVE_INET_ATON
/***********************************************************************
 *
 * Function:    inet_aton
 *
 * Summary:     Manipulate our network address information
 *
 * Parmeters:   None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int inet_aton(const char *cp, struct in_addr *addr)
{
	register u_long val;
	register int base, n;
	register char c;
	u_int parts[4];
	register u_int *pp = parts;

	for (;;) {
		/* Collect number up to ``.''. Values are specified as for
		   C: 0x=hex, 0=octal, other=decimal. */
		val = 0;
		base = 10;
		if (*cp == '0') {
			if (*++cp == 'x' || *cp == 'X')
				base = 16, cp++;
			else
				base = 8;
		}
		while ((c = *cp) != '\0') {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			if (base == 16 && isascii(c) && isxdigit(c)) {
				val =
				    (val << 4) + (c + 10 -
						  (islower(c) ? 'a' :
						   'A'));
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.') {
			/* Internet format:
			     a.b.c.d
			     a.b.c   (with c treated as 16-bits)
			     a.b     (with b treated as 24 bits) */

			if (pp >= parts + 3 || val > 0xff)
				return (0);
			*pp++ = val, cp++;
		} else
			break;
	}
	/* Check for trailing characters. */
	if (*cp && (!isascii(*cp) || !isspace(*cp)))
		return (0);

	/* Concoct the address according to the number of parts specified. */
	n = pp - parts + 1;
	switch (n) {

	case 1:		/* a -- 32 bits */
		break;

	case 2:		/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:		/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:		/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |=
		    (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}
#endif

char *printlong(unsigned long val)
{
	static char buf[5];

	set_long(buf, val);
	buf[4] = 0;
	return buf;
}

unsigned long makelong(char *c)
{
	char c2[4];
	int l = strlen(c);

	if (l >= 4)
		return get_long(c);
	memset(c2, ' ', 4);
	memcpy(c2, c, l);
	return get_long(c2);
}

void dumpline(const unsigned char *buf, int len, int addr)
{
	int i;

	fprintf(stderr, "%.4x  ", addr);

	for (i = 0; i < 16; i++) {

		if (i < len)
			fprintf(stderr, "%.2x ",
				0xff & (unsigned int) buf[i]);
		else
			fprintf(stderr, "   ");
	}

	fprintf(stderr, "  ");

	for (i = 0; i < len; i++) {
		if (isprint(buf[i]) && (buf[i] >= 32) && (buf[i] <= 126))
			fprintf(stderr, "%c", buf[i]);
		else
			fprintf(stderr, ".");
	}
	fprintf(stderr, "\n");
}

void dumpdata(const unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i += 16) {
		dumpline(buf + i, ((len - i) > 16) ? 16 : len - i, i);
	}
}

double get_float(void *buffer)
{
	unsigned char *buf = buffer;

	/* Load values */
	unsigned long frac = get_long(buf);
	int exp = get_sshort(buf + 4);
	int sign = get_byte(buf + 6);

	return ldexp(sign ? (double) frac : -(double) frac, exp);
}

void set_float(void *buffer, double value)
{
	unsigned char *buf = buffer;

	unsigned long frac;
	int exp, sign;

	/* Take absolute */
	if (value < 0) {
		sign = 0;
		value = -value;
	} else
		sign = 0xFF;

	/* Convert mantissa to 32-bit integer, and take exponent */
	frac = (unsigned long) ldexp(frexp(value, &exp), 32);
	exp -= 32;

	/* Store values in buffer */
	set_long(buf, frac);
	set_sshort(buf + 4, exp);
	set_byte(buf + 6, sign);
	set_byte(buf + 7, 0);
}

int compareTm(struct tm *a, struct tm *b)
{
	int d;

	d = a->tm_year - b->tm_year;
	if (d)
		return d;
	d = a->tm_mon - b->tm_mon;
	if (d)
		return d;
	d = a->tm_mday - b->tm_mday;
	if (d)
		return d;
	d = a->tm_hour - b->tm_hour;
	if (d)
		return d;
	d = a->tm_min - b->tm_min;
	if (d)
		return d;
	d = a->tm_sec - b->tm_sec;
	return d;
}

#ifdef OS2

/* Replacement version of getenv(), because the one in the EMX 0.9c, fix03
   dist appears to be busted when called from inside a DLL. (MJJ) */
char *getenv(const char *envar)
{
	APIRET rc;
	unsigned char *envstring;

	/* just call the OS/2 function directly */
	rc = DosScanEnv(envar, &envstring);
	if (rc)
		return NULL;
	else
		return envstring;
}

#endif
