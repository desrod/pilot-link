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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>

#include "pi-debug.h"
#include "pi-source.h"

/* this routine ruthlessly stolen verbatim from Brian J. Swetland */

/***********************************************************************
 *
 * Function:    crc16
 *
 * Summary:     Implementation of the CRC16 Cyclic Redundancy Check
 *
 * Parameters:  None
 *
 * Returns:     CRC + NULL
 *
 ***********************************************************************/
int crc16(unsigned char *ptr, int count)
{
	int	crc,
		i;

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
 * Function:	strdup
 *
 * Summary:	Duplicate a string
 *
 * Parameters:	None   
 *
 * Returns:	String or NULL
 *
 ***********************************************************************/
char *strdup(const char *s)
{
        char *result;
        size_t size = strlen(s) + 1;
   
        if (!(result = malloc(size))) {
                return NULL;
        }

        memcpy(result, s, size);

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
 * Parameters:  None
 *
 * Returns:     0 for success, nonzero if any errors occur
 *
 ***********************************************************************/
int putenv(const char *string)
{
	const char 	*const name_end = strchr(string, '=');
	register 	size_t size;
	register 	char **ep;

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
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int inet_aton(const char *cp, struct in_addr *addr)
{
	register u_long 	val;
	register int 		base;
	register int		n;
	register char 		c;
	u_int 			parts[4];
	register u_int 		*pp = parts;

	for (;;) {
		/* Collect number up to ``.''. Values are specified as for
		   C: 0x=hex, 0=octal, other=decimal. */
		val 	= 0;
		base 	= 10;
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
	int 	l 	= strlen(c);
	char 	c2[4];

	if (l >= 4)
		return get_long(c);
	memset(c2, ' ', 4);
	memcpy(c2, c, (size_t)l);
	return get_long(c2);
}

void dumpline(const char *buf, size_t len, unsigned int addr)
{
	unsigned int i;

	pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "  %.4x  ", addr);

	for (i = 0; i < 16; i++) {

		if (i < len)
			pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "%.2x ",
			       0xff & (unsigned int) buf[i]);
		else
			pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "   ");
	}

	pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "  ");

	for (i = 0; i < len; i++) {
		if (isprint(buf[i]) && (buf[i] >= 32) && (buf[i] <= 126))
			pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "%c", buf[i]);
		else
			pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, ".");
	}
	pi_log(PI_DBG_ALL, PI_DBG_LVL_NONE, "\n");
}

void dumpdata(const char *buf, size_t len)
{
	unsigned int i;

	for (i = 0; i < len; i += 16)
		dumpline(buf + i, ((len - i) > 16) ? 16 : len - i, i);
}

double get_float(void *buffer)
{
	unsigned char *buf = buffer;
	int 	exp 	= get_sshort(buf + 4),
		sign 	= get_byte(buf + 6);
	unsigned long frac = get_long(buf);

	return ldexp(sign ? (double) frac : -(double) frac, exp);
}

void set_float(void *buffer, double value)
{
	int 	exp, 
		sign;
	unsigned char *buf = buffer;
	unsigned long frac;
	double r;

	/* Take absolute */
	if (value < 0) {
		sign 	= 0;
		value 	= -value;
	} else
		sign 	= 0xFF;

	/* Convert mantissa to 32-bit integer, and take exponent */
	r = ldexp(frexp(value, &exp), 32);
	frac = (unsigned long)r;
	exp -= 32;

	/* Store values in buffer */
	set_long(buf, frac);
	set_sshort(buf + 4, exp);
	set_byte(buf + 6, sign);
	set_byte(buf + 7, 0);
}

int compareTm(struct tm *a, struct tm *b)
{
	int 	date;

	date = a->tm_year - b->tm_year;
	if (date)
		return date;
	date = a->tm_mon - b->tm_mon;
	if (date)
		return date;
	date = a->tm_mday - b->tm_mday;
	if (date)
		return date;
	date = a->tm_hour - b->tm_hour;
	if (date)
		return date;
	date = a->tm_min - b->tm_min;
	if (date)
		return date;
	date = a->tm_sec - b->tm_sec;
	return date;
}


/* Fix some issues with some locales reporting 2 or 4 digit years */
size_t palm_strftime(char *s, size_t max, const char *fmt,
        const struct tm *tm) {

        return strftime(s, max, fmt, tm);
}


#ifdef OS2
/* Replacement version of getenv(), because the one in the EMX 0.9c, fix03
   dist appears to be busted when called from inside a DLL. (MJJ) */
char *getenv(const char *envar)
{
	APIRET 	rc;
	unsigned char *envstring;

	/* just call the OS/2 function directly */
	rc = DosScanEnv(envar, &envstring);
	if (rc)
		return NULL;
	else
		return envstring;
}
#endif

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
