/*
 * debug.c:  Pilot-Link debug configuration and debug logging
 *
 * Copyright (c) 1996, Anonymous
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
#include <stdarg.h>

#include <pi-debug.h>

static int debug_types = PI_DBG_NONE;
static int debug_level = PI_DBG_LVL_NONE;
static FILE *debug_file = NULL;


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
	debug_level = level;
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
	if (debug_file != NULL && debug_file != stderr)
		fclose (debug_file);

	debug_file = fopen (path, "w");
	if (debug_file == NULL)
		debug_file = stderr;
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
pi_log (int type, int level, char *format, ...)
{
	va_list ap;

	if (!(debug_types & type) && !(type == PI_DBG_ALL))
		return;
	
	if (debug_level < level)
		return;

	if (debug_file == NULL)
		debug_file = stderr;
	
	va_start(ap, format);
	vfprintf(debug_file, format, ap);
	va_end(ap);

	fflush(debug_file);
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
