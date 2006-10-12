/*
 * debug.c:  Pilot-Link debug configuration and debug logging
 *
 * Copyright (c) 1996, Anonymous
 *
 * $Id$
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "pi-debug.h"
#include "pi-threadsafe.h"

static int debug_types = PI_DBG_NONE;
static int debug_level = PI_DBG_LVL_NONE;
static FILE *debug_file = NULL;
static PI_MUTEX_DEFINE(logfile_mutex);

/***********************************************************************
 *
 * Function:    pi_debug_get_types
 *
 * Summary:     fetches the current debug types configuration
 *
 * Parameters:  void
 *
 * Returns:     debug_types
 *
 ***********************************************************************/
int
pi_debug_get_types (void)
{
	return debug_types;
}


/***********************************************************************
 *
 * Function:    pi_debug_set_types
 *
 * Summary:     sets the debug_types configuration
 *
 * Parameters:  types
 *
 * Returns:     void
 *
 ***********************************************************************/
void
pi_debug_set_types (int types)
{
	debug_types = types;
}


/***********************************************************************
 *
 * Function:    pi_debug_get_types
 *
 * Summary:     fetches the current debug level configuration
 *
 * Parameters:  void
 *
 * Returns:     debug_level
 *
 ***********************************************************************/
int
pi_debug_get_level (void)
{
	return debug_level;
}


/***********************************************************************
 *
 * Function:    pi_debug_set_level
 *
 * Summary:     sets the debug_level configuration
 *
 * Parameters:  level
 *
 * Returns:     void
 *
 ***********************************************************************/
void
pi_debug_set_level (int level)
{
	pi_mutex_lock(&logfile_mutex);
	debug_level = level;
	pi_mutex_unlock(&logfile_mutex);
}

/***********************************************************************
 *
 * Function:    pi_debug_set_file
 *
 * Summary:     sets the debug log file configuration
 *
 * Parameters:  char* to logfile name
 *
 * Returns:     void
 *
 ***********************************************************************/
void
pi_debug_set_file (const char *path) 
{
	pi_mutex_lock(&logfile_mutex);

	if (debug_file != NULL && debug_file != stderr)
		fclose (debug_file);

	debug_file = fopen (path, "a");
	if (debug_file == NULL)
		debug_file = stderr;

	pi_mutex_unlock(&logfile_mutex);
}


/***********************************************************************
 *
 * Function:    pi_log
 *
 * Summary:     logs a debug message
 *
 * Parameters:  type, level, format, ...
 *
 * Returns:     void
 *
 ***********************************************************************/
void
pi_log (int type, int level, const char *format, ...)
{
	va_list ap;

	if (!(debug_types & type) && type != PI_DBG_ALL)
		return;
	
	if (debug_level < level)
		return;

	pi_mutex_lock(&logfile_mutex);

	if (debug_file == NULL)
		debug_file = stderr;

#if HAVE_PTHREAD
	fprintf(debug_file, "[thread 0x%08lx] ", pi_thread_id());
#endif
	va_start(ap, format);
	vfprintf(debug_file, format, ap);
	va_end(ap);

	fflush(debug_file);

	pi_mutex_unlock(&logfile_mutex);
}

void
pi_dumpline(const char *buf, size_t len, unsigned int addr)
{
	unsigned int i;
	int offset;
	char line[256];

	offset = sprintf(line, "  %.4x  ", addr);

	for (i = 0; i < 16; i++) {
		if (i < len)
			offset += sprintf(line+offset, "%.2x ",
			       0xff & (unsigned int) buf[i]);
		else {
			strcpy(line+offset, "   ");
			offset += 3;
		}
	}

	strcpy(line+offset, "  ");
	offset += 2;

	for (i = 0; i < len; i++) {
		if (buf[i] == '%') {
			/* since we're going through pi_log, we need to
			 * properly escape % characters
			 */
			line[offset++] = '%';
			line[offset++] = '%';
		} else if (isprint(buf[i]) && buf[i] >= 32 && buf[i] <= 126)
			line[offset++] = buf[i];
		else
			line[offset++] = '.';
	}

	strcpy(line+offset,"\n");
	LOG((PI_DBG_ALL, PI_DBG_LVL_NONE, line));
}

void
dumpline(const char *buf, size_t len, unsigned int addr)
{
	/* this function will be removed in 0.13. Use pi_dumpline() instead. */
	pi_dumpline(buf, len, addr);
}

void
pi_dumpdata(const char *buf, size_t len)
{
	unsigned int i;

	for (i = 0; i < len; i += 16)
		pi_dumpline(buf + i, ((len - i) > 16) ? 16 : len - i, i);
}

void
dumpdata(const char *buf, size_t len)
{
	/* this function will be removed in 0.13. Use pi_dumpdata() instead */
	pi_dumpdata(buf, len);
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
