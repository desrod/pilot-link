#include <stdio.h>
#include <stdarg.h>

#include <pi-debug.h>

static int debug_types = PI_DBG_NONE;
static int debug_level = PI_DBG_LVL_NONE;
static FILE *debug_file = NULL;

int pi_debug_get_types (void)
{
	return debug_types;
}

void pi_debug_set_types (int types)
{
	debug_types = types;
}

int pi_debug_get_level (void)
{
	return debug_level;
}

void pi_debug_set_level (int level)
{
	debug_level = level;
}

void pi_debug_set_file (const char *path) 
{
	if (debug_file != NULL && debug_file != stderr)
		fclose (debug_file);

	debug_file = fopen (path, "w");
	if (debug_file == NULL)
		debug_file = stderr;
}

void pi_log (int type, int level, char *format, ...)
{
	va_list ap;

	if (!(debug_types & type) && !(debug_types & PI_DBG_ALL))
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
