/*
 * Pilot Datebook processing utility
 *
 * (c) 2000, Matthias Hessler <pilot-datebook@mhessler.de>
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
 
#include <string.h>
#include "pilot-datebook-data.h"

/* pilot-datebook-joblist.h */
extern void joblist_abort_all(void);


/* Constants */
const int DATEBOOK_MAX_CATEGORIES = 16;
const char *Weekday[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const int WEEKDAY_LEN = sizeof(Weekday[0]);
const char WEEKDAY_UNKNOWN[] = "unknown_weekday";

/*
  const char *Month[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
*/
/* Month name looks nicer if capitalized; case is irrelevant for parsedate
 */
static const char *Month[12] = {"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};
static const int MONTH_LEN = sizeof(Month[0]);
static const char MONTH_UNKNOWN[] = "unknown_month";
static const char DATE_UNKNOWN[] = "unknown_date";
static const char TIME_UNKNOWN[] = "unknown_time";
static const char BOOLEAN_YES[] = "yes";
static const char BOOLEAN_NO[] = "no";
static const char BOOLEAN_TRUE[] = "true";
static const char BOOLEAN_FALSE[] = "false";



static const char * DATA_FORMAT_TXT[] = {
  "invalid",
  "hotsync",
  "pdb",
  "csv",
  "windat",
  "longtxt",
  "shorttxt",
  "remind",
  "ical"
};

/* Field names */
const char DATEBOOK_FIELD_UID[] = "uid";
const char DATEBOOK_FIELD_ATTRIBUTES[] = "attributes";
const char DATEBOOK_FIELD_CATEGORY[] = "category";
const char DATEBOOK_FIELD_UNTIMED[] = "untimed";
const char DATEBOOK_FIELD_BEGIN[] = "begin";
const char DATEBOOK_FIELD_BEGIN_DATE[] = "beginDate";
const char DATEBOOK_FIELD_BEGIN_TIME[] = "beginTime";
const char DATEBOOK_FIELD_END[] = "end";
const char DATEBOOK_FIELD_END_DATE[] = "endDate";
const char DATEBOOK_FIELD_END_TIME[] = "endTime";
const char DATEBOOK_FIELD_ALARM[] = "alarm";
const char DATEBOOK_FIELD_ADVANCE[] = "advance";
const char DATEBOOK_FIELD_ADVANCE_UNIT[] = "advanceUnit";
const char DATEBOOK_FIELD_REPEAT_TYPE[] = "repeatType";
const char DATEBOOK_FIELD_REPEAT_FOREVER[] = "repeatForever";
const char DATEBOOK_FIELD_REPEAT_END[] = "repeatEnd";
const char DATEBOOK_FIELD_REPEAT_END_DATE[] = "repeatEndDate";
const char DATEBOOK_FIELD_REPEAT_END_TIME[] = "repeatEndTime";
const char DATEBOOK_FIELD_REPEAT_FREQUENCY[] = "repeatFrequency";
const char DATEBOOK_FIELD_REPEAT_DAY[] = "repeatDay";
const char DATEBOOK_FIELD_REPEAT_WEEKSTART[] = "repeatWeekstart";
const char DATEBOOK_FIELD_REPEAT_WEEKDAYS[] = "repeatWeekdays";
const char DATEBOOK_FIELD_REPEAT_EXCEPTION_NUM[] = "repeatExceptionNum";
/* Leave out repeatException for now
 * (complicated array compare)
 */
const char DATEBOOK_FIELD_DESCRIPTION[] = "description";
const char DATEBOOK_FIELD_NOTE[] = "note";

/* Transient calculation/scratch fields */
const char DATEBOOK_FIELD_XLONG[] = "xLong";
const char DATEBOOK_FIELD_YLONG[] = "yLong";
const char DATEBOOK_FIELD_ZLONG[] = "zLong";
const char DATEBOOK_FIELD_XINT[] = "xInt";
const char DATEBOOK_FIELD_YINT[] = "yInt";
const char DATEBOOK_FIELD_ZINT[] = "zInt";
const char DATEBOOK_FIELD_XTIME[] = "xTime";
const char DATEBOOK_FIELD_YTIME[] = "yTime";
const char DATEBOOK_FIELD_ZTIME[] = "zTime";
const char DATEBOOK_FIELD_XSECONDS[] = "xSeconds";
const char DATEBOOK_FIELD_YSECONDS[] = "ySeconds";
const char DATEBOOK_FIELD_ZSECONDS[] = "zSeconds";
const char DATEBOOK_FIELD_XSTR[] = "xStr";
const char DATEBOOK_FIELD_YSTR[] = "yStr";
const char DATEBOOK_FIELD_ZSTR[] = "zStr";



/* 'Global' Variables
 *
 * (message routines need access to verbose levels)
 */
int pilot_datebook_verbose_level;




enum DATA_FORMAT
txt2dataformat (char * txt)
{
  /* Check for supported formats */
  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_HOTSYNC]) == 0)
    return DATA_FORMAT_HOTSYNC;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_PDB]) == 0)
    return DATA_FORMAT_PDB;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_CSV]) == 0)
     return DATA_FORMAT_CSV;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_WINDAT]) == 0)
     return DATA_FORMAT_WINDAT;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_LONGTXT]) == 0)
    return DATA_FORMAT_LONGTXT;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_SHORTTXT]) == 0)
    return DATA_FORMAT_SHORTTXT;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_REMIND]) == 0)
    return DATA_FORMAT_REMIND;

  if (strcmp(txt, DATA_FORMAT_TXT[DATA_FORMAT_ICAL]) == 0)
    return DATA_FORMAT_ICAL;


  /* Data format is unknown */
  return DATA_FORMAT_INVALID;
}


/* Return data format string corresponding to data format number */
const char *
dataformat2txt (enum DATA_FORMAT format)
{
  switch (format)
    {

    case DATA_FORMAT_HOTSYNC:
      return DATA_FORMAT_TXT[DATA_FORMAT_HOTSYNC];
      break;

    case DATA_FORMAT_PDB:
      return DATA_FORMAT_TXT[DATA_FORMAT_PDB];
      break;

    case DATA_FORMAT_CSV:
      return DATA_FORMAT_TXT[DATA_FORMAT_CSV];
      break;

    case DATA_FORMAT_WINDAT:
      return DATA_FORMAT_TXT[DATA_FORMAT_WINDAT];
      break;

    case DATA_FORMAT_LONGTXT:
      return DATA_FORMAT_TXT[DATA_FORMAT_LONGTXT];
      break;

    case DATA_FORMAT_SHORTTXT:
      return DATA_FORMAT_TXT[DATA_FORMAT_SHORTTXT];
      break;

    case DATA_FORMAT_REMIND:
      return DATA_FORMAT_TXT[DATA_FORMAT_REMIND];
      break;

    case DATA_FORMAT_ICAL:
      return DATA_FORMAT_TXT[DATA_FORMAT_ICAL];
      break;

    default:
      return DATA_FORMAT_TXT[DATA_FORMAT_INVALID];
    }
}


/* Return weekday string corresponding to number of week day (0 = Sunday) */
const char *
int2weekday (int weekday_num)
{
  if (weekday_num < 0
      || weekday_num > 7)
    return WEEKDAY_UNKNOWN;
  else
    return Weekday[weekday_num];
}


/* Parse weekday string, and return number of week day (0 = Sunday) */
int
weekday2int (char * weekday_str)
{
  int k;
  int found;

  found  = -1;

  for(k=0;k<7;k++) {
    if(!data_strincmp(weekday_str, Weekday[k], WEEKDAY_LEN -1)) {
      found = k;
      break;
    }
  } /* for */

  /* Return result */
  return found;
}


/* Return month string corresponding to number of month (0 = January) */
const char *
int2month (int month_num)
{
  if (month_num < 0
      || month_num > 11)
    return MONTH_UNKNOWN;
  else
    return Month[month_num];
}


/* Parse month string, and return number of month (0 = January) */
int
month2int (char * month_str)
{
  int k;
  int found;

  found  = -1;

  for(k=0;k<11;k++) {
    if(!data_strincmp(month_str, Month[k], MONTH_LEN -1)) {
      found = k;
      break;
    }
  } /* for */

  /* Return result */
  return found;
}


/* General helper functions */

/* Write routine for unstructured data */
void
write_dump (FILE * out_file, void *buf, int n)
{
  int i, j, c;

  for (i = 0; i < n; i += 16) {
    fprintf (out_file, "%04x: ", i);
    for (j = 0; j < 16; j++) {
      if (i+j < n)
	fprintf (out_file, "%02x ", ((unsigned char *)buf)[i+j]);
      else
	fprintf (out_file, "   ");
    }
    fprintf (out_file, "  ");
    for (j = 0; j < 16 && i+j < n; j++) {
      c = ((unsigned char *)buf)[i+j] & 0x7f;
      if (c < ' ' || c >= 0x7f)
	fputc ('.', out_file);
      else
	fputc (c, out_file);
    }
    fprintf (out_file, "\n");
  }
}


/* Format time <time_t> into human-readable date string
 *
 * The format can also be parsed by parsedate, however when
 * reading in a human-readable string, care has to be taken to
 * read all space-separated parts.
 * The space separator is actually only there for easier reading,
 * parsedate will also process it if spaces are removed.
 */
void
write_human_date_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, DATE_UNKNOWN);
  }
  else {
    tm = *localtime (&t);
    snprintf (buffer, buffer_size, "%d %s %d",
	      tm.tm_mday,
	      Month[tm.tm_mon],
	      tm.tm_year+1900);
  }
}


/* Format time <time_t> into human-readable time string
 * (assume time is in local time)
 *
 * The format can also be parsed by parsedate, and parsedate
 * will rightly assume that the seconds are 0 when not present.
 */
void
write_human_time_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    tm = *localtime (&t);
    /* leave out seconds if not present */
    if (tm.tm_sec == 0) {
      snprintf (buffer, buffer_size, "%02d:%02d",
		tm.tm_hour, tm.tm_min);
    }
    else {
      snprintf (buffer, buffer_size, "%02d:%02d:%02d",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
  }
}


/* Format time <time_t> into human-readable time string
 * (assume time is in gmt time)
 *
 * The format can also be parsed by parsedate, and parsedate
 * will rightly assume that the seconds are 0 when not present.
 */
void
write_human_gmtime_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    tm = *gmtime (&t);
    /* leave out seconds if not present */
    if (tm.tm_sec == 0) {
      snprintf (buffer, buffer_size, "%02d:%02d",
		tm.tm_hour, tm.tm_min);
    }
    else {
      snprintf (buffer, buffer_size, "%02d:%02d:%02d",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
  }
}


/* Format time <time_t> into human-readable date and time string
 *
 * The format can also be parsed by parsedate, and parsedate
 * will rightly assume that the seconds are 0 when not present.
 */
void
write_human_full_time_str (time_t t, char * buffer, int buffer_size)
{
  int buffer_len;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    /* Get date string */
    write_human_date_str (t, buffer, buffer_size);

    /* Add space separator between date and time string */
    buffer_len = strlen(buffer);
    if (buffer_len +2 < buffer_size) {
      buffer[buffer_len] = ' ';
      buffer[buffer_len+1] = '\0';
      buffer_len++;

      /* Get time string */
      write_human_time_str (t, buffer + buffer_len, buffer_size - buffer_len);
    }
  }
}


/* Format time <time_t> into standardized readable date
 *
 * The format can also be parsed by parsedate, but only after
 * the dashes used as separator are replaced by forward slashes
 * (which is what read_iso_time_str will do).
 */
void
write_iso_date_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, DATE_UNKNOWN);
  }
  else {
    tm = *localtime (&t);
    snprintf (buffer, buffer_size, "%04d-%02d-%02d",
	      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  }
}


/* Format time <time_t> into standardized readable time
 * (assume time is in local time)
 *
 * The format can also be parsed by parsedate.
 */
void
write_iso_time_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    tm = *localtime (&t);
    snprintf (buffer, buffer_size, "%02d:%02d:%02d",
	      tm.tm_hour, tm.tm_min, tm.tm_sec);
  }
}


/* Format time <time_t> into standardized readable time
 * (assume time is in gmt time)
 *
 * The format can also be parsed by parsedate.
 */
void
write_iso_gmtime_str (time_t t, char * buffer, int buffer_size)
{
  struct tm tm;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    tm = *gmtime (&t);
    snprintf (buffer, buffer_size, "%02d:%02d:%02d",
	      tm.tm_hour, tm.tm_min, tm.tm_sec);
  }
}


/* Format time <time_t> into standardized readable date and time string
 *
 * The format can also be parsed by parsedate.
 */
void
write_iso_full_time_str (time_t t, char * buffer, int buffer_size)
{
  int buffer_len;


  if (t < 0) {
    snprintf(buffer, buffer_size, TIME_UNKNOWN);
  }
  else {
    /* Get date string */
    write_iso_date_str (t, buffer, buffer_size);

    /* Add space separator between date and time string */
    buffer_len = strlen(buffer);
    if (buffer_len +2 < buffer_size) {
      buffer[buffer_len] = ' ';
      buffer[buffer_len+1] = '\0';
      buffer_len++;

      /* Get time string */
      write_iso_time_str (t, buffer + buffer_len, buffer_size - buffer_len);
    }
  }
}



/* Reading dates is a bit tricky.
 * Unfortunately, parsedate always assumes time is formatted in GMT if no
 * timezone is provided.
 * Also, when providing a timezone for which it is not clear whether it
 * is on daylight saving, then parsedate will shift all summer times
 * according to daylight savings, while keeping all winter times unchanged
 * (if you are in winter). Or shift all winter times if you are in summer...
 *
 * Much more desirable is to have parsedate interpret all times as
 * local times, and switch off the shifting of summer/winter times.
 * Therefore I've implemented a work around for this, if
 * no time zone is given (a time zone is detected by a non-digit after
 * the last space - ignoring trailing space):
 *
 * I now tell parsedate explicitly to assume GMT. The resulting time_t is
 * then passed to gmtime to convert into a struct tm without any conversion.
 * Now mktime will convert the struct tm to a time_t, taking the local
 * timezone properly into a account - such that a later printout will
 * work as expected.
 *
 * Please let me know if you know a better way or there are any issues
 * with this workaround. I had quite some fun before arriving at this
 * solution...
 */


/* Read human-formatted or iso-formatted date-only string */
time_t
read_iso_date_str1 (char * date_buffer)
{
  char parsedate_buffer[50];


  /* Join date/time parts */
  snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s",
	    date_buffer, "00:00");

  /* Parse */
  return (read_iso_time_str1(parsedate_buffer));
}


/* Read human-formatted or iso-formatted date & time string */
time_t
read_iso_time_str4n (int num_read, char * buffer1, char * buffer2, char * buffer3, char * buffer4)
{
  char parsedate_buffer[50];


  /* Join date/time parts */
  if (num_read == 4) {
    snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s %s %s",
	      buffer1, buffer2, buffer3, buffer4);
  }
  else if (num_read == 3) {
    snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s %s",
	      buffer1, buffer2, buffer3);
  }
  else if (num_read == 2) {
    snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s",
	      buffer1, buffer2);
  }
  else if (num_read == 1) {
    snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s",
	      buffer1);
    }
  else
    return (-1);

  /* Parse */
  return (read_iso_time_str1(parsedate_buffer));
}


/* Read human-formatted or iso-formatted date & time string */
time_t
read_iso_time_str2 (char * date_buffer, char * time_buffer)
{
  char parsedate_buffer[50];


  /* Join date/time parts */
  snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s",
	    date_buffer, time_buffer);

  /* Parse */
  return (read_iso_time_str1(parsedate_buffer));
}


/* Read human-formatted or iso-formatted date & time string */
time_t
read_iso_time_str1 (char * date_buffer)
{
  char parsedate_buffer[50];
  time_t t;
  char * pos = NULL;
  int timezone_present;


  if (strstr(date_buffer, DATE_UNKNOWN) != NULL
      || strstr(date_buffer, TIME_UNKNOWN) != NULL) {
    t = -1;
  }
  else {
    /* Check whether time zone was given
     *
     * A time zone is assumed to be present if the first character
     * after the last space is not a digit.
     */
    pos = data_rindex(date_buffer, ' ');
    timezone_present = FALSE;
    if (pos != NULL) {
      /* Ignore trailing space */
      if (*(pos+1) == '\0') {
	/* Skip over trailing space */
	while (isspace(*pos)
	       && pos > date_buffer)
	  pos--;
	/* Found first real character, now search backwards for space */
	while (!isspace(*pos)
	       && pos > date_buffer)
	  pos--;
	/* Now found space before last real text or number */
      }
      /* Now check first letter for timezone presence,
       * anything which is not a digit is assumed to be a timezone.
       */
      if (pos != NULL)
	timezone_present = !isdigit(*(pos+1));
    }

    if (timezone_present) {
      /* Use timezone */
      strncpy (parsedate_buffer, date_buffer, sizeof(parsedate_buffer));
    }
    else {
      /* Get time in GMT (use "" for local time!?) */
      snprintf (parsedate_buffer, sizeof(parsedate_buffer), "%s %s",
		date_buffer, "");
    }

    /* parsedate wants to have '/' as a year/month/date separator,
     * so convert any '-' separator we might have to '/'
     * (to allow reading of a string written with write_iso_date_str)
     */
    pos = data_index(parsedate_buffer, '-');
    while (pos != NULL) {
      *pos = '/';
      pos = data_index(parsedate_buffer, '-');
    }

    /* parse date */
    t = parsedate(parsedate_buffer);

    /* Convert GMT to local time
     * (parsedate assumes GMT, except when time zone is given)
     */
    if (!timezone_present) {
      if (t >= 0)
	t = mktime(gmtime(&t));
    }
  }

  /* Now return time according to local time zone */
  return t;
}



/* String handling */

/* Find position of a character search_char within string_string */
char *
data_index(const char * search_string, char search_char)
{

  if (search_string == NULL)
    return NULL;

  do {
    if (*search_string == search_char)
      return (char *) search_string;
    else if (*search_string == '\0')
      return (char *) NULL;
    else
      search_string++;
  } while (TRUE);
}


char *
data_rindex(const char * search_string, char search_char)
{
  const char * pos1;


  if (search_string == NULL)
    return NULL;

  pos1 = search_string + strlen(search_string);

  do {
    if (*pos1 == search_char)
      return (char *) pos1;
    else if (pos1 == search_string)
      return (char *) NULL;
    else
      pos1--;
  } while (TRUE);
}


/* Compare two strings, ignoring case */
int
data_stricmp(const char * pos1, const char * pos2)
{
  if (pos1 == NULL)
    return -1;
  if (pos2 == NULL)
    return 1;

  do {
    if (tolower(*pos1) != tolower(*pos2))
      return (tolower(*pos1) - tolower(*pos2));
    else if (*pos1 == '\0'
	     || *pos2 == '\0')
      return 0;
    else {
      pos1++;
      pos2++;
    }
  } while (TRUE);
}


/* Compare two strings, ignoring case */
int
data_strincmp(const char * pos1, const char * pos2, int max_len)
{
  if (pos1 == NULL)
    return -1;
  if (pos2 == NULL)
    return 1;

  do {
    if (tolower(*pos1) != tolower(*pos2))
      return (tolower(*pos1) - tolower(*pos2));
    else if (*pos1 == '\0'
	     || *pos2 == '\0')
      return 0;
    else {
      pos1++;
      pos2++;
      max_len--;
      if (max_len <= 0)
	return 0;
    }
  } while (TRUE);
}


/* Quote dangerous characters */
void
text_quote(const char * in_start, char * out_start, int max_len)
{
  const char * in;
  char * out;
  int len;


  in = in_start;
  out = out_start;
  /* Start len with 1 to leave room for trailing 0 at the end */
  len = 1;
  /* Check for max_len -1 to allow for quoted character at the end */
  while(*in != '\0'
	&& len < max_len-1) {
    if(*in == '\\') {
      /* Quote backslash */
      *out++ = '\\';
      len++;
      in++;
    }
    else if(*in == '\t') {
      /* Quote tabulator */
      *out++ = '\\';
      *out++ = 't';
      len+=2;
      in++;
    }
    else if(*in == '\n') {
      /* Quote newline */
      *out++ = '\\';
      *out++ = 'n';
      len+=2;
      in++;
    }
    else if(*in == ';') {
      /* Quote delimiter */
      *out++ = '\\';
      *out++ = ';';
      len+=2;
      in++;
    }
    else if(*in == '\"') {
      /* Quote double quote */
      *out++ = '\\';
      *out++ = '\"';
      len+=2;
      in++;
    }
    else if(*in == '\'') {
      /* Quote single */
      *out++ = '\\';
      *out++ = '\'';
      len+=2;
      in++;
    }
    else {
      *out++=*in++;
      len++;
    }
  }

  /* End string */
  *out = '\0';
}


/* Un-quote dangerous characters */
void
text_unquote(const char * in_start, char * out_start, int max_len)
{
  const char * in;
  const char * in_end;
  char * out;
  int len;


  /* Only process input between in_start and in_end; workaround for the
   * case where we tell length of input string in max_len.
   * max_len has to be minimum of (length input, size output buffer).
   */
  in = in_start;
  in_end = in_start + max_len;
  out = out_start;
  len = 0;
  while(*in != '\0'
	&& len < max_len
	&& in < in_end) {
    if(*in != '\\') {
      /* Normal character */
      *out++ = *in++;
      len++;
    }
    else {
      /* Quoted character */
      if(*(in+1) == '\\') {
	/* Un-quote backslash */
	*out++ = '\\';
	len++;
	in+=2;
      }
      else if(*(in+1) == 't') {
	/* Un-quote tabulator */
	*out++ = '\t';
	len++;
	in+=2;
      }
      else if(*(in+1) == 'n') {
	/* Un-quote newline */
	*out++ = '\n';
	len++;
	in+=2;
      }
      else {
	/* Store quoted character without backslash */
	*out++=*(in+1);
	len++;
	in+=2;
      }
    } /* if found backslash as quote */
  } /* while */
  /* End string if still enough room */
  if (len <= max_len)
    *out = '\0';
}


/* Error handling */

/* Handle error message
 *
 * An error is something where further processing makes no sense:
 * - a file can not be opened
 * - out of memory
 * ...
 * The program can assume that the code after the call to error_message
 * will never be reached.
 *
 * An error message will always be shown.
 * Always, program execution will be stopped.
 */
void
error_message(char * format, ...)
{
  va_list printf_args;


  /* Always show error messages */

  /* Get variable arguments */
  va_start(printf_args, format);

  /* Output message */
  vfprintf (stderr, format, printf_args);

  /* No more processing of variable arguments */
  va_end(printf_args);

  /* Errors are fatal */
  joblist_abort_all();
  exit (1);
}


/* Handle warning message
 *
 * An warning is something where further processing could make sense:
 * - one input record could not be understood (maybe skipping will help)
 * - some inconsistency between read data is detected
 * ...
 * If the call to warn_message returns, the program will try to continue
 * on a 'best effort' basis.
 *
 * A warning message will be shown (unless switched off by command line).
 * Program execution may stop (depending on command line switch).
 */
void
warn_message(char * format, ...)
{
  va_list printf_args;


  /* Check message verbose level */
  if (get_message_verbose() >= MESSAGE_VERBOSE_WARN) {

    /* Get variable arguments */
    va_start(printf_args, format);

    /* Output message */
    vfprintf (stderr, format, printf_args);

    /* No more processing of variable arguments */
    va_end(printf_args);
  }

  /* Treat warnings as fatal for now */
  joblist_abort_all();
  exit (1);
}


/* Handle information message
 *
 * An information is something which has no influence on the processing:
 * - statistics
 * - some exceptional data is detected
 * ...
 * If the call to warn_message returns, the program will try to continue
 * on a 'best effort' basis.
 *
 * An information message may be shown (depending on command line switch).
 * Program execution will not stop.
 */
void
info_message(char * format, ...)
{
  va_list printf_args;


  /* Check message verbose level */
  if (get_message_verbose() >= MESSAGE_VERBOSE_INFO) {

    /* Get variable arguments */
    va_start(printf_args, format);

    /* Output message */
    vfprintf (stderr, format, printf_args);

    /* No more processing of variable arguments */
    va_end(printf_args);
  }

  /* Information message will not cause any exit */
}


/* Handle debug message
 *
 * A debug message is a message used for debugging.
 * If the call to debug_message returns, the program will continue without
 * further side effects.
 *
 * A debug message will normally not be shown (unless switched on by
 * command line switch).
 * Program execution will not stop.
 */
void
debug_message(char * format, ...)
{
  va_list printf_args;


  /* Check message verbose level */
  if (get_message_verbose() >= MESSAGE_VERBOSE_DEBUG) {

    /* Get variable arguments */
    va_start(printf_args, format);

    /* Output message */
    vfprintf (stderr, format, printf_args);

    /* No more processing of variable arguments */
    va_end(printf_args);
  }

  /* Debug message will not cause any exit */
}


/* Message verbose level
 *
 * level 0 => only show errors
 * level 1 => show errors + warnings
 * level 2 => show errors + warnings + information
 * level 3 => show errors + warnings + information + debug
 *
 */

/* Get message verbose level */
enum MESSAGE_VERBOSE_LEVEL
get_message_verbose (void)
{
  return pilot_datebook_verbose_level;
}

/* Set message verbose level */
void
set_message_verbose (enum MESSAGE_VERBOSE_LEVEL verbose_level)
{
  pilot_datebook_verbose_level = verbose_level;
}




/* Find string position of next delimiter */
char *
data_find_delimiter (const char * pos1, char delimiter)
{
  char * pos2;
  char * pos3;
  char * pos4;

  /* Check for quoted text */
  do {
    pos2 = data_index(pos1, delimiter);
    /* Skip over quoted delimiter */
    while (pos2 != NULL
	   && (pos2 > pos1)
	   && *(pos2-1) == '\\')
      pos2 = data_index(pos2+1, delimiter);

    pos3 = data_index(pos1, '"');
    if (pos3 != NULL
	&& pos3 < pos2) {
      /* Find end of quoting */
      pos4 = data_index(pos3 +1, '"');
      /* Skip over quoted quote */
      while (pos4 != NULL
	     && *(pos4-1) == '\\')
	pos4 = data_index(pos4+1, '"');
      /* Had previously found delimiter been inside quotes? */
      if (pos4 > pos2) {
	/* Then search for delimiter after end of quote */
	pos1 = pos4 +1;
	continue;
      } /* if */
    } /* if found begin of quoting */

    pos3 = data_index(pos1, '\'');
    if (pos3 != NULL
	&& pos3 < pos2) {
      /* Find end of quoting */
      pos4 = data_index(pos3 +1, '\'');
      /* Skip over quoted quote */
      while (pos4 != NULL
	     && *(pos4-1) == '\\')
	pos4 = data_index(pos4+1, '\'');
      /* Had previously found delimiter been inside quotes? */
      if (pos4 > pos2) {
	/* Then search for delimiter after end of quote */
	pos1 = pos4 +1;
	continue;
      } /* if */
    } /* if */

    /* If we reach here, we have found a delimiter */
    break;
  } while (TRUE);

  return pos2;
}



/* Value handling */
/* Identify values */
void
value_init (struct value_data * value, const char * value_start, const char * value_end)
{
  char date_buffer[100];
  time_t t;
  int len = 0;
  char * pos1;
  long l;


  debug_message("Entering value_init\n");

  /* Remove surrounding quotes, if present */
  if (value_start < (value_end -1)
      && *value_start == '"'
      && *(value_end -1) == '"') {
    value_start++;
    value_end--;
  }
  else if (value_start < (value_end -1)
	   && *value_start == '\''
	   && *(value_end -1) == '\'') {
    value_start++;
    value_end--;
  }

  /* Identify expected data type, then read data */
  switch (value->type) {
  case DATEBOOK_FIELD_LONG:
    value->type = DATEBOOK_FIELD_LONG;
    value->literal.lit_long = strtol(value_start, &pos1, 0);
    if (pos1 != value_end)
      error_message("Did not understand long value <%.*s>\n",
		    value_end - value_start,
		    value_start);
    break;

  case DATEBOOK_FIELD_INT:
    value->type = DATEBOOK_FIELD_INT;
    if (data_strincmp(value_start, BOOLEAN_YES, strlen(BOOLEAN_YES)) == 0)
      value->literal.lit_int = 1;
    else if (data_strincmp(value_start, BOOLEAN_NO, strlen(BOOLEAN_NO)) == 0)
      value->literal.lit_int = 0;
    else if (data_strincmp(value_start, BOOLEAN_TRUE, strlen(BOOLEAN_TRUE)) == 0)
      value->literal.lit_int = 1;
    else if (data_strincmp(value_start, BOOLEAN_FALSE, strlen(BOOLEAN_FALSE)) == 0)
      value->literal.lit_int = 0;
    else
      value->literal.lit_int = atoi(value_start);
    break;

  case DATEBOOK_FIELD_TIME:
    value->type = DATEBOOK_FIELD_TIME;
    len = value_end - value_start;
    if (len >= sizeof(date_buffer))
      error_message("Date_buffer size: Can not parse date/time <%.*s>\n\n",
		    len,
		    value_start);
    strncpy(date_buffer, value_start, len);
    date_buffer[len] = '\0';
    t = read_iso_time_str1 (date_buffer);
    if (t == -1) {
      warn_message("Can not parse date/time <%s>\n\n",
		   date_buffer);
      break;
    }
    value->literal.lit_time = *localtime(&t);
    break;

  case DATEBOOK_FIELD_SECONDS:
    value->type = DATEBOOK_FIELD_SECONDS;
    value->literal.lit_seconds = strtol(value_start, &pos1, 10);
    /* Assume seconds, if no time unit is given */
    if (*pos1 == ':') {
      /* These were hours, now we get minutes */
      value->literal.lit_seconds *= (60*60);
      pos1++;
      if (pos1 < value_end) {
	/* Get minutes */
	l = strtol(pos1, &pos1, 10);
	value->literal.lit_seconds += l*60;
	if (*pos1 == ':') {
	  /* Get seconds if present */
	  pos1++;
	  if (pos1 < value_end) {
	    /* Get seconds */
	    l = strtol(pos1, &pos1, 10);
	    value->literal.lit_seconds += l;
	  } /* if seconds present */
	} /* found ':' */
      } /* if minutes present */
    } /* found ':' */
    else {
      /* Check for date/time qualifier */
      if (*pos1 == 'm') {
	/* Minutes */
	value->literal.lit_seconds *= 60;
	pos1++;
      }
      else if (*pos1 == 'h') {
	/* Hours */
	value->literal.lit_seconds *= (60*60);
	pos1++;
      }
      else if (*pos1 == 'd') {
	/* Days */
	value->literal.lit_seconds *= (60*60*24);
	pos1++;
      }
    } /* else check for qualifier */
    if (pos1 != value_end)
      error_message("Did not understand seconds value <%.*s>\n",
		    value_end - value_start,
		    value_start);
    break;

  case DATEBOOK_FIELD_STR:
    {
      char buffer[0xffff];


      /* Unquote text */
      if (value_end - value_start < sizeof(buffer)) {
	text_unquote(value_start, buffer, value_end - value_start);
	buffer[value_end - value_start] = '\0';
      }
      else {
	text_unquote(value_start, buffer, sizeof(buffer));
	buffer[sizeof(buffer)] = '\0';
      }
      value->type = DATEBOOK_FIELD_STR;
      value->literal.lit_str = strdup(buffer);
    }
    break;

  default:
    error_message("Do not know how to handle data type <%d>\n",
		  value->type);
  }

  debug_message("Leaving value_init\n");
}


/* Destroy data structure */
void
value_exit(struct value_data * value)
{

  /* Debug */
  debug_message("Entering value_exit\n");

  /* Only have to free string, if a string had been allocated */
  if (value->type == DATEBOOK_FIELD_STR
      && value->literal.lit_str != NULL)
    free ((void *) value->literal.lit_str);
  value->literal.lit_str = NULL;

  debug_message("Leaving value_exit\n");
}



/* Field handling */

/* Identify fields */
void
field_init (struct field_data * field, const char * field_start, const char * field_end)
{
  char buffer[100];


  debug_message("Entering field_init\n");

  /* Extract field name */
  if (field_end == NULL) {
    /* Last field */
    strncpy(buffer,
	    field_start,
	    sizeof(buffer) -1);
  }
  else {
    /* More sort fields to come */
    if ((field_end -field_start) > (sizeof(buffer) -1)) {
      strncpy(buffer,
	      field_start,
	      sizeof(buffer) -1);
    }
    else {
      strncpy(buffer,
	      field_start,
	      field_end -field_start);
    }
    buffer[field_end -field_start] = '\0';
  }


  debug_message("Identified field <%s>...\n", buffer);

  /* Would be nice to have a better parser, but identifying fields
   * is probably not performance critical, since it is NOT used during
   * actual data processing - only during setup.
   */
  if (!strncmp(buffer, DATEBOOK_FIELD_UID, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_UID;
    field->type = DATEBOOK_FIELD_LONG;
    field->get_func.get_long = getRowUid;
    field->set_func.set_long = setRowUid;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ATTRIBUTES, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ATTRIBUTES;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowAttributes;
    field->set_func.set_int = setRowAttributes;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_CATEGORY, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_CATEGORY;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowCategory;
    field->set_func.set_int = setRowCategory;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_UNTIMED, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_UNTIMED;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowUntimed;
    field->set_func.set_int = setRowUntimed;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_BEGIN, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_BEGIN;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowBegin;
    field->set_func.set_time = setRowBegin;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_BEGIN_DATE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_BEGIN_DATE;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowBeginDate;
    field->set_func.set_time = setRowBeginDate;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_BEGIN_TIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_BEGIN_TIME;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowBeginTime;
    field->set_func.set_seconds = setRowBeginTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_END, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_END;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowEnd;
    field->set_func.set_time = setRowEnd;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_END_DATE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_END_DATE;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowEndDate;
    field->set_func.set_time = setRowEndDate;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_END_TIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_END_TIME;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowEndTime;
    field->set_func.set_seconds = setRowEndTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ALARM, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ALARM;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowAlarm;
    field->set_func.set_int = setRowAlarm;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ADVANCE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ADVANCE;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowAdvance;
    field->set_func.set_int = setRowAdvance;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ADVANCE_UNIT, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ADVANCE_UNIT;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowAdvanceUnit;
    field->set_func.set_int = setRowAdvanceUnit;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_TYPE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_TYPE;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatType;
    field->set_func.set_int = setRowRepeatType;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_FOREVER, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_FOREVER;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatForever;
    field->set_func.set_int = setRowRepeatForever;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_END, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_END;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowRepeatEnd;
    field->set_func.set_time = setRowRepeatEnd;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_END_DATE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_END_DATE;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowRepeatEndDate;
    field->set_func.set_time = setRowRepeatEndDate;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_END_TIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_END_TIME;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowRepeatEndTime;
    field->set_func.set_seconds = setRowRepeatEndTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_FREQUENCY, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_FREQUENCY;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatFrequency;
    field->set_func.set_int = setRowRepeatFrequency;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_DAY, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_DAY;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatDay;
    field->set_func.set_int = setRowRepeatDay;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_WEEKDAYS, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_WEEKDAYS;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatWeekdays;
    field->set_func.set_int = setRowRepeatWeekdays;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_WEEKSTART, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_WEEKSTART;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatWeekstart;
    field->set_func.set_int = setRowRepeatWeekstart;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_REPEAT_EXCEPTION_NUM, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_REPEAT_EXCEPTION_NUM;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowRepeatExceptionNum;
    field->set_func.set_int = setRowRepeatExceptionNum;
  }
  /* Leave out repeatException for now
   * (complicated array compare)
   */
  else if (!strncmp(buffer, DATEBOOK_FIELD_DESCRIPTION, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_DESCRIPTION;
    field->type = DATEBOOK_FIELD_STR;
    field->get_func.get_str = getRowDescription;
    field->set_func.set_str = setRowDescription;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_NOTE, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_NOTE;
    field->type = DATEBOOK_FIELD_STR;
    field->get_func.get_str = getRowNote;
    field->set_func.set_str = setRowNote;
  }
  /* Transient */
  else if (!strncmp(buffer, DATEBOOK_FIELD_XLONG, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_XLONG;
    field->type = DATEBOOK_FIELD_LONG;
    field->get_func.get_long = getRowXLong;
    field->set_func.set_long = setRowXLong;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_YLONG, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_YLONG;
    field->type = DATEBOOK_FIELD_LONG;
    field->get_func.get_long = getRowYLong;
    field->set_func.set_long = setRowYLong;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ZLONG, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ZLONG;
    field->type = DATEBOOK_FIELD_LONG;
    field->get_func.get_long = getRowZLong;
    field->set_func.set_long = setRowZLong;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_XINT, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_XINT;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowXInt;
    field->set_func.set_int = setRowXInt;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_YINT, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_YINT;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowYInt;
    field->set_func.set_int = setRowYInt;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ZINT, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ZINT;
    field->type = DATEBOOK_FIELD_INT;
    field->get_func.get_int = getRowZInt;
    field->set_func.set_int = setRowZInt;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_XTIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_XTIME;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowXTime;
    field->set_func.set_time = setRowXTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_YTIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_YTIME;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowYTime;
    field->set_func.set_time = setRowYTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ZTIME, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ZTIME;
    field->type = DATEBOOK_FIELD_TIME;
    field->get_func.get_time = getRowZTime;
    field->set_func.set_time = setRowZTime;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_XSECONDS, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_XSECONDS;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowXSeconds;
    field->set_func.set_seconds = setRowXSeconds;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_YSECONDS, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_YSECONDS;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowYSeconds;
    field->set_func.set_seconds = setRowYSeconds;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ZSECONDS, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ZSECONDS;
    field->type = DATEBOOK_FIELD_SECONDS;
    field->get_func.get_seconds = getRowZSeconds;
    field->set_func.set_seconds = setRowZSeconds;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_XSTR, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_XSTR;
    field->type = DATEBOOK_FIELD_STR;
    field->get_func.get_str = getRowXStr;
    field->set_func.set_str = setRowXStr;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_YSTR, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_YSTR;
    field->type = DATEBOOK_FIELD_STR;
    field->get_func.get_str = getRowYStr;
    field->set_func.set_str = setRowYStr;
  }
  else if (!strncmp(buffer, DATEBOOK_FIELD_ZSTR, sizeof(buffer))) {
    field->name = DATEBOOK_FIELD_ZSTR;
    field->type = DATEBOOK_FIELD_STR;
    field->get_func.get_str = getRowZStr;
    field->set_func.set_str = setRowZStr;
  }
  else {
    error_message("Do not understand field <%s>\n", buffer);
  }

  debug_message("Leaving field_init\n");
}


/* Destroy data structure */
void
field_exit(struct field_data * field)
{

  /* Debug */
  debug_message("Entering field_exit\n");

  /* Nothing to do for the moment, since no memory alloc in field_init() */

  debug_message("Leaving field_exit\n");
}


/* Get value from row for one field */
struct value_data
row_get_field (struct row_data * row, struct field_data * field)
{
  struct value_data result;


  debug_message("Entering row_get_field\n");

  switch (field->type) {
  case DATEBOOK_FIELD_LONG:
    result.type = DATEBOOK_FIELD_LONG;
    result.literal.lit_long = (field->get_func.get_long) (row);
    break;

  case DATEBOOK_FIELD_INT:
    result.type = DATEBOOK_FIELD_INT;
    result.literal.lit_int = (field->get_func.get_int) (row);
    break;

  case DATEBOOK_FIELD_TIME:
    result.type = DATEBOOK_FIELD_TIME;
    result.literal.lit_time = (field->get_func.get_time) (row);
    break;

  case DATEBOOK_FIELD_SECONDS:
    result.type = DATEBOOK_FIELD_SECONDS;
    result.literal.lit_seconds = (field->get_func.get_seconds) (row);
    break;

  case DATEBOOK_FIELD_STR:
    result.type = DATEBOOK_FIELD_STR;
    result.literal.lit_str = (field->get_func.get_str) (row);
    break;

  default:
    error_message("Can not get value for unknown field type\n");
  }

  debug_message("Leaving row_get_field\n");

  return result;
}


/* Get value from row for one field */
void
row_set_field (struct row_data * row, struct field_data * field, struct value_data * value)
{

  debug_message("Entering row_set_field\n");

  switch (field->type) {
  case DATEBOOK_FIELD_LONG:
    (field->set_func.set_long) (row, value->literal.lit_long);
    break;

  case DATEBOOK_FIELD_INT:
    (field->set_func.set_int) (row, value->literal.lit_int);
    break;

  case DATEBOOK_FIELD_TIME:
    (field->set_func.set_time) (row, value->literal.lit_time);
    break;

  case DATEBOOK_FIELD_SECONDS:
    (field->set_func.set_seconds) (row, value->literal.lit_seconds);
    break;

  case DATEBOOK_FIELD_STR:
    (field->set_func.set_str) (row, value->literal.lit_str);
    break;

  default:
    error_message("Can not set value for unknown field type\n");
  }

  debug_message("Leaving row_set_field\n");
}


/* Compare one field in row1 with the same field in row2 */
int
field_cmp_rows (struct field_data * field, struct row_data * row1, struct row_data * row2)
{
  int result = 0;


  switch (field->type) {
  case DATEBOOK_FIELD_LONG:
    result = ((field->get_func.get_long) (row1)
	      - (field->get_func.get_long) (row2));
    break;
  case DATEBOOK_FIELD_INT:
    result = ((field->get_func.get_int) (row1)
	      - (field->get_func.get_int) (row2));
    break;
  case DATEBOOK_FIELD_TIME:
    {
      struct tm tm1;
      struct tm tm2;

      tm1 = (field->get_func.get_time) (row1);
      tm2 = (field->get_func.get_time) (row2);
      result = (mktime(&tm1) - mktime(&tm2));
    }
    break;
  case DATEBOOK_FIELD_STR:
    {
      char * str1;
      char * str2;

      str1 = (field->get_func.get_str) (row1);
      str2 = (field->get_func.get_str) (row2);
      if (str1 == NULL && str2 == NULL)
	return 0;
      else if (str1 == NULL)
	return -1;
      else if (str2 == NULL)
	return +1;
      else
	return strcmp(str1, str2);
    }
    break;
  default:
    break;
  }

  return result;
}


/* Init expression data */
void
expr_init (struct expr_data * expr, enum DATEBOOK_FIELD_TYPE expect_type, const char * expr_start, const char * expr_end)
{
  char date_buffer[100];


  /* Debug */
  debug_message("Entering expr_init\n");

  /* Safety checks */
  if (expr == NULL)
    error_message("Assert failed: expr == NULL\n");
  if (expr_start == NULL)
    error_message("Assert failed: expr_start == NULL\n");
  if (expr_end == NULL)
    error_message("Assert failed: expr_end == NULL\n");

  if (expr_start >= expr_end)
    error_message("Can not find any value in expression <%.*s>\n",
		  expr_end - expr_start,
		  expr_start);

  /* Store expression text */
  expr->name = (char *) malloc(expr_end - expr_start +1);
  strncpy (expr->name, expr_start, expr_end - expr_start);
  (expr->name)[expr_end - expr_start] = '\0';

  /* Parse expression */
  if (isalpha(*expr_start)) {
    /* Expect field variable */
    expr->isField = TRUE;
    field_init (&(expr->content.field), expr_start, expr_end);

    /* Error if data type does not match */
    if (expr->content.field.type != expect_type)
      error_message("Field data type for <%.*s> differs from expected data type\n",
		    expr_end - expr_start,
		    expr_start);
  }
  else {
    /* Expect literal */
    expr->isField = FALSE;
    expr->content.constant.type = expect_type;
    value_init (&(expr->content.constant), expr_start, expr_end);

    /* Error if data type does not match */
    if (expr->content.constant.type != expect_type)
      error_message("Literal data type for <%.*s> differs from expected data type\n",
		    expr_end - expr_start,
		    expr_start);
  }


  /* Debug output */
  if (expr->isField) {
    /* Field */
    switch(expr->content.field.type) {
    case DATEBOOK_FIELD_LONG:
      debug_message("right_field(long) = <%s>\n",
		    expr->content.field.name);
      break;

    case DATEBOOK_FIELD_INT:
      debug_message("right_field(int) = <%s>\n",
		    expr->content.field.name);
      break;

    case DATEBOOK_FIELD_TIME:
      debug_message("right_field(time) = <%s>\n",
		    expr->content.field.name);
      break;

    case DATEBOOK_FIELD_SECONDS:
      debug_message("right_field(seconds) = <%s>\n",
		    expr->content.field.name);
      break;

    case DATEBOOK_FIELD_STR:
      debug_message("right_field(str) = <%s>\n",
		    expr->content.field.name);
      break;

    default:
      error_message("Unknown right_field data type...\n");
    }
  } /* if field */
  else {
    /* Constant */
    switch(expr->content.constant.type) {
    case DATEBOOK_FIELD_LONG:
      debug_message("right(long) = <%lu>\n",
		    expr->content.constant.literal.lit_long);
      break;

    case DATEBOOK_FIELD_INT:
      debug_message("right(int) = <%d>\n",
		    expr->content.constant.literal.lit_int);
      break;

    case DATEBOOK_FIELD_TIME:
      write_human_full_time_str(mktime(&(expr->content.constant.literal.lit_time)),
				date_buffer,
				sizeof(date_buffer));
      debug_message("right(time) = <%s>\n",
		    date_buffer);
      break;

    case DATEBOOK_FIELD_SECONDS:
      debug_message("right(seconds) = <%ld>\n",
		    expr->content.constant.literal.lit_seconds);
      break;

    case DATEBOOK_FIELD_STR:
      debug_message("right(str) = <%s>\n",
		    expr->content.constant.literal.lit_str);
      break;

    default:
      error_message("Unknown right_field data type...\n");
    }
  } /* if constant */

  debug_message("Leaving expr_init\n");
}


/* Free expr data */
void
expr_exit (struct expr_data * expr)
{

  debug_message("Entering expr_exit\n");

  /* Free expression string */
  free ((void *) expr->name);
  expr->name = NULL;

  if (expr->isField)
    field_exit(&(expr->content.field));
  else
    value_exit(&(expr->content.constant));

  debug_message("Leaving expr_exit\n");
}


/* Do actual update in row for one field */
struct value_data
expr_row (struct expr_data * expr, struct row_data * row)
{
  struct value_data result;


  debug_message("Entering expr_row\n");

  if (expr->isField) {
    /* Field */
    result = row_get_field(row, &(expr->content.field));
    if (result.type != expr->content.field.type)
      error_message("Result type of field <%s> does not match expected type for expr <%s>\n",
		    expr->content.field.name,
		    expr->name);
  }
  else {
    result = expr->content.constant;
#if 0
    /* Literal */
    switch (expr->content.constant.type) {
    case DATEBOOK_FIELD_LONG:
      result.type = DATEBOOK_FIELD_LONG;
      result.literal.lit_long = expr->content.constant.literal.lit_long;
      break;

    case DATEBOOK_FIELD_INT:
      result.type = DATEBOOK_FIELD_INT;
      result.literal.lit_int = expr->content.constant.literal.lit_int;
      break;

    case DATEBOOK_FIELD_TIME:
      result.type = DATEBOOK_FIELD_TIME;
      result.literal.lit_time = expr->content.constant.literal.lit_time;
      break;

    case DATEBOOK_FIELD_SECONDS:
      result.type = DATEBOOK_FIELD_SECONDS;
      result.literal.lit_seconds = expr->content.constant.literal.lit_seconds;
      break;

    case DATEBOOK_FIELD_STR:
      result.type = DATEBOOK_FIELD_STR;
      result.literal.lit_str = expr->content.constant.literal.lit_str;
      /* Please note: string result is not allowed to be freed!!! */
      break;

    default:
      break;
    }
#endif
  } /* else (literal) */

  debug_message("Leaving expr_row\n");

  return result;
}



/* Assignment handling */

/* Init assign data */
void
assign_init (struct assign_data * assign, const char * assign_start, const char * assign_end)
{
  char * op_start = NULL;
  char * right_start = NULL;


  debug_message("Entering assign_init\n");

  /* Safety checks */
  if (assign == NULL)
    error_message("Assert failed: assign == NULL\n");
  if (assign_start == NULL)
    error_message("Assert failed: assign_start == NULL\n");
  if (assign_end == NULL)
    error_message("Assert failed: assign_end == NULL\n");

  /* Find assignment character within extracted update assign */
  op_start = data_index(assign_start, '=');
  if (op_start == NULL
      || op_start - assign_end >= 0)
    error_message("Invalid assignment <%.*s>\n",
                  assign_end - assign_start,
                  assign_start);

  /* Right part of assignment starts after assignment operator */
  right_start = op_start +1;

  /* Which assignment operator? */
  if (*(op_start -1) == '+') {
    /* Increment update */
    op_start--;
    assign->op = DATEBOOK_ASSIGN_ADD;
  }
  else if (*(op_start -1) == '-') {
    /* Decrement update */
    op_start--;
    assign->op = DATEBOOK_ASSIGN_SUBTRACT;
  }
  else {
    /* Assign update */
    assign->op = DATEBOOK_ASSIGN;
  }

  /* Sanity check of all pointers */
  if (assign_start >= op_start)
    error_message("Can not identify left (=target field) in update <%.*s>\n",
		  assign_end - assign_start,
		  assign_start);
  if (op_start >= right_start)
    error_message("Can not identify assign operator in update <%.*s>\n",
		  assign_end - assign_start,
		  assign_start);
  if (right_start >= assign_end)
    error_message("Can not find any right (=assign value) in update <%.*s>\n",
		  assign_end - assign_start,
		  assign_start);

  /* Store assignment text */
  assign->name = (char *) malloc(assign_end - assign_start +1);
  strncpy (assign->name, assign_start, assign_end - assign_start);
  (assign->name)[assign_end - assign_start] = '\0';

  /* Store assignment data field */
  field_init (&(assign->left), assign_start, op_start);

  /* Debug output */
  debug_message("left=<%s> type=<%d>, assignment=<%d>, ",
	       assign->left.name,
	       assign->left.type,
	       assign->op);

  /* Store expression data */
  if (assign->left.type == DATEBOOK_FIELD_TIME
      && (assign->op == DATEBOOK_ASSIGN_ADD
	  || assign->op == DATEBOOK_ASSIGN_SUBTRACT))
    assign->expr_type = DATEBOOK_FIELD_SECONDS;
  else
    assign->expr_type = assign->left.type;

  expr_init (&(assign->right), assign->expr_type, right_start, assign_end);

  debug_message("Leaving assign_init\n");
}


/* Free assign data */
void
assign_exit (struct assign_data * assign)
{

  debug_message("Entering assign_exit\n");

  /* Free field data */
  field_exit (&(assign->left));

  /* Free expression data */
  expr_exit (&(assign->right));

  /* Free assignment string */
  free ((void *) assign->name);
  assign->name = NULL;

  debug_message("Leaving assign_exit\n");
}


/* Do actual update in row for one field */
void
assign_row (struct assign_data * assign, struct row_data * row)
{
  struct value_data result;


  debug_message("Entering assign_row\n");

  /* Get value from right expression */
  result = expr_row(&(assign->right), row);

  /* Check whether returned expression type matches expected expression type */
  if (result.type != assign->expr_type)
    error_message("Assignment <%s> expected expression type <%d>, but got <%d>\n",
		  assign->name,
		  assign->expr_type,
		  result.type);

  switch (result.type) {
  case DATEBOOK_FIELD_LONG:
    {
      if (assign->op == DATEBOOK_ASSIGN
	  && result.type == DATEBOOK_FIELD_LONG)
	(assign->left.set_func.set_long) (row,
					  result.literal.lit_long);
      else if (assign->op == DATEBOOK_ASSIGN_ADD
	       && result.type == DATEBOOK_FIELD_LONG)
	(assign->left.set_func.set_long) (row,
					  (assign->left.get_func.get_long) (row)
					  + result.literal.lit_long);
      else if (assign->op == DATEBOOK_ASSIGN_SUBTRACT
	       && result.type == DATEBOOK_FIELD_LONG)
	(assign->left.set_func.set_long) (row,
					  (assign->left.get_func.get_long) (row)
					  - result.literal.lit_long);
      else
	error_message("Unknown long assign operation <%d> to type <%d>\n",
		      assign->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_INT:
    {
      if (assign->op == DATEBOOK_ASSIGN
	  && result.type == DATEBOOK_FIELD_INT)
	(assign->left.set_func.set_int) (row,
					 result.literal.lit_int);
      else if (assign->op == DATEBOOK_ASSIGN_ADD
	       && result.type == DATEBOOK_FIELD_INT)
	(assign->left.set_func.set_int) (row,
					 (assign->left.get_func.get_int) (row)
					 + result.literal.lit_int);
      else if (assign->op == DATEBOOK_ASSIGN_SUBTRACT
	       && result.type == DATEBOOK_FIELD_INT)
	(assign->left.set_func.set_int) (row,
					 (assign->left.get_func.get_int) (row)
					 - result.literal.lit_int);
      else
	error_message("Unknown int assign operation <%d> to type <%d>\n",
		      assign->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_TIME:
  case DATEBOOK_FIELD_SECONDS:
    {
      time_t t;
      struct tm tm;

      if (assign->op == DATEBOOK_ASSIGN
	  && result.type == DATEBOOK_FIELD_TIME) {
	(assign->left.set_func.set_time) (row, result.literal.lit_time);
      }
      else if (assign->op == DATEBOOK_ASSIGN
	  && result.type == DATEBOOK_FIELD_SECONDS) {
	(assign->left.set_func.set_seconds) (row, result.literal.lit_seconds);
      }
      else if (assign->op == DATEBOOK_ASSIGN_ADD
	       && result.type == DATEBOOK_FIELD_SECONDS) {
	tm = (assign->left.get_func.get_time) (row);
	t = mktime(&tm);
	t += result.literal.lit_seconds;
	(assign->left.set_func.set_time) (row, *localtime(&t));
      }
      else if (assign->op == DATEBOOK_ASSIGN_SUBTRACT
	       && result.type == DATEBOOK_FIELD_SECONDS) {
	tm = (assign->left.get_func.get_time) (row);
	t = mktime(&tm);
	t -= result.literal.lit_seconds;
	(assign->left.set_func.set_time) (row, *localtime(&t));
      }
      else
	error_message("Unknown time assign operation <%d> to type <%d>\n",
		      assign->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_STR:
    {
      if (assign->op == DATEBOOK_ASSIGN
	  && result.type == DATEBOOK_FIELD_STR) {
	(assign->left.set_func.set_str) (row, result.literal.lit_str);
      }
      else if (assign->op == DATEBOOK_ASSIGN_ADD
	       && result.type == DATEBOOK_FIELD_STR) {
	char buffer[0xffff];

	if ((assign->left.get_func.get_str) (row) == NULL) {
	  (assign->left.set_func.set_str) (row, result.literal.lit_str);
	}
	else {
	  snprintf(buffer, sizeof(buffer) -1, "%s%s",
		   (assign->left.get_func.get_str) (row),
		   result.literal.lit_str);
	  buffer[sizeof(buffer) -1] = '\0';
	  (assign->left.set_func.set_str) (row, buffer);
	}
      }
      else if (assign->op == DATEBOOK_ASSIGN_SUBTRACT
	       && result.type == DATEBOOK_FIELD_STR) {
	char * pos1;
	char * pos2;
	char buffer[0xffff];
	int len;

	/* Remove first occurance of substract string */
	buffer[0] = '\0';
	pos1 = (assign->left.get_func.get_str) (row);
	if (pos1 != NULL)
	  pos2 = strstr(pos1, result.literal.lit_str);
	while (pos1 != NULL
	       && *pos1 != '\0') {
	  /* Check for end of string */
	  if (pos2 == NULL) {
	    pos2 = pos1 + strlen(pos1);
	  }

	  /* Copy to buffer */
	  len = strlen(buffer);
	  if (pos2 - pos1 < sizeof(buffer) - len) {
	    strncat(buffer, pos1, pos2 - pos1);
	    buffer[len + (pos2 - pos1)] = '\0';
	  }
	  else if (sizeof(buffer) -len -1 > 0) {
	    strncat(buffer, pos1, sizeof(buffer) -len -1);
	    buffer[sizeof(buffer) -1] = '\0';
	  }

	  /* Find next occurance of search string */
	  if (*pos2 != '\0') {
	    pos1 = pos2 + strlen(result.literal.lit_str);
	    pos2 = strstr(pos1, result.literal.lit_str);
	  }
	  else {
	    break;
	  }
	} /* while */
	/* Assign resulting buffer to string */
	(assign->left.set_func.set_str) (row, buffer);
      }
      else
	error_message("Unknown string assign operation <%d> to type <%d>\n",
		      assign->op,
		      result.type);
    }
    break;

  default:
    break;
  }

  debug_message("Leaving assign_row\n");
}



/* Condition handling */

/* Init cond data */
void
cond_init (struct cond_data * cond, const char * cond_start, const char * cond_end)
{
  char * op_start = NULL;
  char * right_start = NULL;
  char * pos1;


  debug_message("Entering cond_init\n");

  /* Safety checks */
  if (cond == NULL)
    error_message("Assert failed: cond == NULL\n");
  if (cond_start == NULL)
    error_message("Assert failed: cond_start == NULL\n");
  if (cond_end == NULL)
    error_message("Assert failed: cond_end == NULL\n");

  /* Find condition character within extracted update condition */
  op_start = data_index(cond_start, '=');
  pos1 = data_index(cond_start, '<');
  if (op_start == NULL
      || (pos1 != NULL
	  && pos1 < op_start))
    op_start = pos1;
  pos1 = data_index(cond_start, '>');
  if (op_start == NULL
      || (pos1 != NULL
	  && pos1 < op_start))
    op_start = pos1;

  /* Did we find condition? */
  if (op_start == NULL)
    error_message("Invalid condition <%.*s>: could not find compare operator\n",
                  cond_end - cond_start,
                  cond_start);
  if (cond_end - op_start <= 0)
    error_message("Invalid condition <%.*s>: could not find right\n",
                  cond_end - cond_start,
                  cond_start);
  if (op_start - cond_start <= 0)
    error_message("Invalid condition <%.*s>: could not find left\n",
                  cond_end - cond_start,
                  cond_start);

  /* Right part of condition starts after condition operator */
  right_start = op_start +1;

  /* Which condition operator? */
  if (*op_start == '='
      && *(op_start +1) == '=') {
    /* equal comparison */
    right_start++;
    cond->op = DATEBOOK_COND_EQUAL;
  }
  else if (*op_start == '='
	   && *(op_start -1) == '!') {
    /* not equal comparison */
    op_start--;
    cond->op = DATEBOOK_COND_NOT_EQUAL;
  }
  else if (*op_start == '<'
      && *(op_start +1) == '=') {
    /* less equal comparison */
    right_start++;
    cond->op = DATEBOOK_COND_LESS_EQUAL;
  }
  else if (*op_start == '<'
      && *(op_start +1) != '=') {
    /* less comparison */
    /* keep right_start as is */
    cond->op = DATEBOOK_COND_LESS;
  }
  else if (*op_start == '>'
      && *(op_start +1) == '=') {
    /* greater equal comparison */
    right_start++;
    cond->op = DATEBOOK_COND_GREATER_EQUAL;
  }
  else if (*op_start == '>'
      && *(op_start +1) != '=') {
    /* greater comparison */
    /* keep right_start as is */
    cond->op = DATEBOOK_COND_GREATER;
  }
  else {
    error_message("Could not find valid condition operator in condition <%.*s>\n",
		  cond_end - cond_start,
		  cond_start);
  }


  /* Sanity check of all pointers */
  if (cond_start >= op_start)
    error_message("Can not identify left (=target field) in condition <%.*s>\n",
		  cond_end - cond_start,
		  cond_start);
  if (op_start >= right_start)
    error_message("Can not identify condition operator in condition <%.*s>\n",
		  cond_end - cond_start,
		  cond_start);
  if (right_start >= cond_end)
    error_message("Can not find any right (=cond value) in condition <%.*s>\n",
		  cond_end - cond_start,
		  cond_start);

  /* Store condition text */
  cond->name = (char *) malloc(cond_end - cond_start +1);
  strncpy (cond->name, cond_start, cond_end - cond_start);
  (cond->name)[cond_end - cond_start] = '\0';

  /* Store condition data field */
  field_init (&(cond->left), cond_start, op_start);

  /* Debug output */
  debug_message("left=<%s> type=<%d>, condition=<%d>, ",
	       cond->left.name,
	       cond->left.type,
	       cond->op);

  /* Store expression data */
  cond->expr_type = cond->left.type;

  expr_init (&(cond->right), cond->expr_type, right_start, cond_end);

  debug_message("Leaving cond_init\n");
}


/* Free cond data */
void
cond_exit (struct cond_data * cond)
{

  debug_message("Entering cond_exit\n");

  /* Free field data */
  field_exit (&(cond->left));

  /* Free expression data */
  expr_exit (&(cond->right));

  /* Free condition string */
  free ((void *) cond->name);
  cond->name = NULL;

  debug_message("Leaving cond_exit\n");
}


/* Do actual update in row for one field */
int
cond_row (struct cond_data * cond, struct row_data * row)
{
  struct value_data result;
  int compare = 0;


  debug_message("Entering cond_row\n");

  /* Get value from right expression */
  result = expr_row(&(cond->right), row);

  /* Check whether returned expression type matches expected expression type */
  if (result.type != cond->expr_type)
    error_message("Condition <%s> expected expression type <%d>, but got <%d>\n",
		  cond->name,
		  cond->expr_type,
		  result.type);

  switch (result.type) {
  case DATEBOOK_FIELD_LONG:
    {
      if (cond->op == DATEBOOK_COND_EQUAL)
	compare = ((cond->left.get_func.get_long) (row)
		   == result.literal.lit_long);
      else if (cond->op == DATEBOOK_COND_NOT_EQUAL)
	compare = ((cond->left.get_func.get_long) (row)
		   != result.literal.lit_long);
      else if (cond->op == DATEBOOK_COND_LESS)
	compare = ((cond->left.get_func.get_long) (row)
		   < result.literal.lit_long);
      else if (cond->op == DATEBOOK_COND_LESS_EQUAL)
	compare = ((cond->left.get_func.get_long) (row)
		   <= result.literal.lit_long);
      else if (cond->op == DATEBOOK_COND_GREATER)
	compare = ((cond->left.get_func.get_long) (row)
		   > result.literal.lit_long);
      else if (cond->op == DATEBOOK_COND_GREATER_EQUAL)
	compare = ((cond->left.get_func.get_long) (row)
		   >= result.literal.lit_long);
      else
	error_message("Unknown long condition <%d> to type <%d>\n",
		      cond->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_INT:
    {
      if (cond->op == DATEBOOK_COND_EQUAL)
	compare = ((cond->left.get_func.get_int) (row)
		   == result.literal.lit_int);
      else if (cond->op == DATEBOOK_COND_NOT_EQUAL)
	compare = ((cond->left.get_func.get_int) (row)
		   != result.literal.lit_int);
      else if (cond->op == DATEBOOK_COND_LESS)
	compare = ((cond->left.get_func.get_int) (row)
		   < result.literal.lit_int);
      else if (cond->op == DATEBOOK_COND_LESS_EQUAL)
	compare = ((cond->left.get_func.get_int) (row)
		   <= result.literal.lit_int);
      else if (cond->op == DATEBOOK_COND_GREATER)
	compare = ((cond->left.get_func.get_int) (row)
		   > result.literal.lit_int);
      else if (cond->op == DATEBOOK_COND_GREATER_EQUAL)
	compare = ((cond->left.get_func.get_int) (row)
		   >= result.literal.lit_int);
      else
	error_message("Unknown int condition <%d> to type <%d>\n",
		      cond->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_TIME:
    {
      time_t t1 = 0;
      time_t t2 = 0;
      struct tm tm;

      tm = (cond->left.get_func.get_time) (row);
      t1 = mktime(&tm);
      t2 = mktime(&(result.literal.lit_time));

      if (cond->op == DATEBOOK_COND_EQUAL)
	compare = (t1 == t2);
      else if (cond->op == DATEBOOK_COND_NOT_EQUAL)
	compare = (t1 != t2);
      else if (cond->op == DATEBOOK_COND_LESS)
	compare = (t1 < t2);
      else if (cond->op == DATEBOOK_COND_LESS_EQUAL)
	compare = (t1 <= t2);
      else if (cond->op == DATEBOOK_COND_GREATER)
	compare = (t1 > t2);
      else if (cond->op == DATEBOOK_COND_GREATER_EQUAL)
	compare = (t1 >= t2);
      else
	error_message("Unknown time condition <%d> to type <%d>\n",
		      cond->op,
		      result.type);
    }
    break;

  case DATEBOOK_FIELD_STR:
    {
      char * str1;
      char * str2;


      str1 = (cond->left.get_func.get_str) (row);
      str2 = result.literal.lit_str;

      if (cond->op == DATEBOOK_COND_EQUAL)
	compare = (strcmp(str1, str2) == 0);
      else if (cond->op == DATEBOOK_COND_NOT_EQUAL)
	compare = (strcmp(str1, str2) != 0);
      else if (cond->op == DATEBOOK_COND_LESS)
	compare = (strcmp(str1, str2) < 0);
      else if (cond->op == DATEBOOK_COND_LESS_EQUAL)
	compare = (strcmp(str1, str2) <= 0);
      else if (cond->op == DATEBOOK_COND_GREATER)
	compare = (strcmp(str1, str2) > 0);
      else if (cond->op == DATEBOOK_COND_GREATER_EQUAL)
	compare = (strcmp(str1, str2) >= 0);
      else
	error_message("Unknown string cond operation <%d> to type <%d>\n",
		      cond->op,
		      result.type);
    }
    break;

  default:
    break;
  }

  debug_message("Compare result=<%d>\n", compare);

  debug_message("Leaving cond_row\n");

  return compare;
}


/* Getters */


/* General data */

/* Is row valid? */
int
getRowIsValid(struct row_data * row)
{
  return row->isValid;
}

/* Is row selected? */
enum DATEBOOK_SELECT_TYPE
getRowIsSelected(struct row_data * row)
{
  return row->isSelected;
}

/* Row number (= temporary data) */
int
getRowRecordNum(struct row_data * row)
{
  return row->record_num;
}

/* Unique identifying number */
unsigned long
getRowUid(struct row_data * row)
{
  return row->uid;
}

/* Status Attribute: (backup, or deleted, or added, ...) */
int
getRowAttributes(struct row_data * row)
{
  return row->attributes;
}


/* Category: (0 = Unfiled) */
int
getRowCategory(struct row_data * row)
{
  return row->category;
}



/* Datebook specific data */

/* Complete pilot-link appointment data */
struct Appointment
getRowAppointment(struct row_data * row)
{
  struct Appointment appointment;
  int i;
  int weekdays;


  /* Appointment data returned should be treated as read-only,
   * since strings, and exception pointers are not newly allocated.
   * Advantage: calling program does not have to care about memory
   * handling, if appointment data is stored in local variable.
   */

  /* Safety net */
  memset(&appointment, 0, sizeof(appointment));

  /* Collect all data */

  /* Collect data through getters to allow for safety & consistency checks */

  appointment.event = getRowUntimed(row);
  appointment.begin = getRowBegin(row);
  appointment.end = getRowEnd(row);

  appointment.alarm = getRowAlarm(row);
  appointment.advance = getRowAdvance(row);
  appointment.advanceUnits = getRowAdvanceUnit(row);

  appointment.repeatType = getRowRepeatType(row);
  appointment.repeatForever = getRowRepeatForever(row);
  appointment.repeatEnd = getRowRepeatEnd(row);
  appointment.repeatFrequency = getRowRepeatFrequency(row);
  appointment.repeatDay = getRowRepeatDay(row);
  weekdays = getRowRepeatWeekdays(row);
  for (i=0; i<7; i++)
    if ((weekdays>>i) & 1)
      appointment.repeatDays[i] = 1;
    else
      appointment.repeatDays[i] = 0;
  appointment.repeatWeekstart = getRowRepeatWeekstart(row);
  appointment.exceptions = getRowRepeatExceptionNum(row);
  appointment.exception = getRowRepeatException(row);

  appointment.description = getRowDescription(row);
  appointment.note = getRowNote(row);


  return appointment;
}


/* UNTIMED_EVENT (=1) or APPOINTMENT? (=0) */
int
getRowUntimed(struct row_data * row)
{
  return row->appointment.event;
}


/* Begin of appointment */
struct tm
getRowBegin(struct row_data * row)
{
  return row->appointment.begin;
}


/* Begin date of appointment */
struct tm
getRowBeginDate(struct row_data * row)
{
  struct tm tm;


  /* Start with time */
  tm = getRowBegin(row);

  /* Erase seconds, minutes, hours */
  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;

  return tm;
}


/* Begin time of appointment */
long
getRowBeginTime(struct row_data * row)
{
  struct tm tm;


  tm = getRowBegin(row);
  return (tm.tm_hour*60*60 + tm.tm_min*60 + tm.tm_sec);
}


/* End of appointment */
struct tm
getRowEnd(struct row_data * row)
{
  return row->appointment.end;
}


/* Begin date of appointment */
struct tm
getRowEndDate(struct row_data * row)
{
  struct tm tm;


  /* Start with time */
  tm = getRowEnd(row);

  /* Erase seconds, minutes, hours */
  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;

  return tm;
}


/* Begin time of appointment */
long
getRowEndTime(struct row_data * row)
{
  struct tm tm;


  tm = getRowEnd(row);
  return (tm.tm_hour*60*60 + tm.tm_min*60 + tm.tm_sec);
}


/* Alarm. yes (=1) or no (=0) */
int
getRowAlarm(struct row_data * row)
{
  return row->appointment.alarm;
}


/* Advance: alarm should be how much in advance (number)? */
int
getRowAdvance(struct row_data * row)
{
  /* Commented out to write bit-exact copy of windat files
  if (!row->appointment.alarm)
    return 0;
  else
  */
    return row->appointment.advance;
}


/* AdvanceUnit: alarm should be how much in advance (days, months, mins)? */
int
getRowAdvanceUnit(struct row_data * row)
{
  /* Commented out to write bit-exact copy of windat files
  if (!row->appointment.alarm)
    return 0;
  else
  */
    return row->appointment.advanceUnits;
}


/* Repeat Type: none, daily, weekly, monthlyByDay, monthlyByDate, yearly */
/* enum repeatTypes */
int
getRowRepeatType(struct row_data * row)
{
  return row->appointment.repeatType;
}


/* Repeat Forever: yes (1) or no (0) */
int
getRowRepeatForever(struct row_data * row)
{
  if (row->appointment.repeatType == repeatNone)
    return 0;
  else
    return row->appointment.repeatForever;
}


/* Repeat End: last date of repeat event */
struct tm
getRowRepeatEnd(struct row_data * row)
{
  struct tm tm;


  if (row->appointment.repeatType == repeatNone)
    memset(&tm, 0, sizeof(tm));
  else
    tm = row->appointment.repeatEnd;

  return tm;
}


/* Repeat End date */
struct tm
getRowRepeatEndDate(struct row_data * row)
{
  struct tm tm;


  /* Start with time */
  tm = getRowRepeatEnd(row);

  /* Erase seconds, minutes, hours */
  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;

  return tm;
}


/* Repeat End time */
long
getRowRepeatEndTime(struct row_data * row)
{
  struct tm tm;


  tm = getRowRepeatEnd(row);
  return (tm.tm_hour*60*60 + tm.tm_min*60 + tm.tm_sec);
}


/* Repeat Frequency: event occurs every time (1) or every x-th time (x) */
int
getRowRepeatFrequency(struct row_data * row)
{
  if (row->appointment.repeatType == repeatNone)
    return 0;
  else
    return row->appointment.repeatFrequency;
}


/* Repeat Day: which day of the month should we repeat (for repeatMonthlyByDay)
 * 27 => on the 27th every month (for repeat monthly)
 * 0-6, 7-13, 14-20, 21-27, 28-34 => Sun-Sat 1st, 2nd, 3rd, 4th, last week
 * (for repeat monthly/weekday)
 */
int
getRowRepeatDay(struct row_data * row)
{
  if (row->appointment.repeatType == repeatNone)
    return 0;
  else
    return row->appointment.repeatDay;
}


/* Repeat Weekdays: Should we repeat on given weekday (for repeatWeekly):
 * yes (1) or no (0)?
 * Output parameter int will be converted from bit array:
 * Sun => weekdays = 1, Mon => weekdays = 2, Tue => weekdays = 4,
 * Sun + Tue => weekdays = 1 + 4 = 5
 */
int
getRowRepeatWeekdays(struct row_data * row)
{
  int i;
  int result;


  if (row->appointment.repeatType == repeatNone)
    result = 0;
  else {
    result = 0;
    for (i=0; i<7; i++)
      if (row->appointment.repeatDays[i])
	result += 1<<i;
  }

  return result;
}


/* Repeat Weekstart: Which day of the week starts week (for repeatWeekly):
 * Sun => weekday = 0, Mon => weekday = 1
 */
int
getRowRepeatWeekstart(struct row_data * row)
{
  /* Commented out to write bit-exact copy of windat files
  if (row->appointment.repeatType == repeatNone)
    return 0;
  else
  */
    return row->appointment.repeatWeekstart;
}


/* Repeat Exceptions: How many repetitions should not take place? */
int
getRowRepeatExceptionNum(struct row_data * row)
{
  if (row->appointment.repeatType == repeatNone)
    return 0;
  else
    return row->appointment.exceptions;
}


/* Repeat Exception: Give pointer to array of all repetition exceptions */
struct tm *
getRowRepeatException(struct row_data * row)
{
  if (row->appointment.repeatType == repeatNone)
    return NULL;
  else
    return row->appointment.exception;
}


/* Description: Description of apointment */
char *
getRowDescription(struct row_data * row)
{
  return row->appointment.description;
}


/* Note: Note of apointment */
char *
getRowNote(struct row_data * row)
{
  return row->appointment.note;
}


/* Transient calculation/scratch fields */

/* xLong */
unsigned long
getRowXLong(struct row_data * row)
{
  return row->xLong;
}


/* yLong */
unsigned long
getRowYLong(struct row_data * row)
{
  return row->yLong;
}


/* zLong */
unsigned long
getRowZLong(struct row_data * row)
{
  return row->zLong;
}


/* xInt */
int
getRowXInt(struct row_data * row)
{
  return row->xInt;
}


/* yInt */
int
getRowYInt(struct row_data * row)
{
  return row->yInt;
}


/* zInt */
int
getRowZInt(struct row_data * row)
{
  return row->zInt;
}

/* xTime */
struct tm
getRowXTime(struct row_data * row)
{
  return row->xTime;
}


/* yTime */
struct tm
getRowYTime(struct row_data * row)
{
  return row->yTime;
}


/* zTime */
struct tm
getRowZTime(struct row_data * row)
{
  return row->zTime;
}


/* xSeconds */
long
getRowXSeconds(struct row_data * row)
{
  return row->xSeconds;
}


/* ySeconds */
long
getRowYSeconds(struct row_data * row)
{
  return row->ySeconds;
}


/* zSeconds */
long
getRowZSeconds(struct row_data * row)
{
  return row->zSeconds;
}


/* xStr */
char *
getRowXStr(struct row_data * row)
{
  return row->xStr;
}


/* yStr */
char *
getRowYStr(struct row_data * row)
{
  return row->yStr;
}


/* zStr */
char *
getRowZStr(struct row_data * row)
{
  return row->zStr;
}





/* Setters */


/* General data */

/* Initialize row data structure */
void
rowInit(struct row_data * row)
{

  /* Safety net */
  memset(row, 0, sizeof(*row));

  /* Now set default values */
  row->isValid = FALSE;
  row->isSelected = DATEBOOK_SELECT_UNKNOWN;
  row->record_num = 0;
  row->uid = 0;
  row->attributes = 0;
  row->category = 0;
  memset(&(row->appointment), 0, sizeof(row->appointment));
}


/* Destroy row data structure */
void
rowExit(struct row_data * row)
{
  free_Appointment(&row->appointment);
}


/* Is row valid? */
void
setRowIsValid(struct row_data * row, int isValid)
{
  row->isValid = isValid;
}

/* Is row selected? */
void
setRowIsSelected(struct row_data * row, enum DATEBOOK_SELECT_TYPE isSelected)
{
  row->isSelected = isSelected;
}

/* Row number (= temporary data) */
void
setRowRecordNum(struct row_data * row, int record_num)
{
  row->record_num = record_num;
}


/* Unique identifying number */
void
setRowUid(struct row_data * row, unsigned long uid)
{
  row->uid = uid;
}


/* Status Attribute: (backup, or deleted, or added, ...) */
void
setRowAttributes(struct row_data * row, int attributes)
{
  row->attributes = attributes;
}


/* Category: (0 = Unfiled) */
void
setRowCategory(struct row_data * row, int category)
{
  row->category = category;
}



/* Datebook specific data */

/* Complete pilot-link appointment data */
void
setRowAppointment(struct row_data * row, struct Appointment appointment)
{
  int i;
  int weekdays;


  /* Write all data */

  /* Write data through setters to allow for safety & consistency checks,
   * and arbitrary underlying data structure.
   */
  setRowUntimed(row, appointment.event);
  setRowBegin(row, appointment.begin);
  setRowEnd(row, appointment.end);

  setRowAlarm(row, appointment.alarm);
  setRowAdvance(row, appointment.advance);
  setRowAdvanceUnit(row, appointment.advanceUnits);

  setRowRepeatType(row, appointment.repeatType);
  setRowRepeatForever(row, appointment.repeatForever);
  setRowRepeatEnd(row, appointment.repeatEnd);
  setRowRepeatFrequency(row, appointment.repeatFrequency);
  setRowRepeatDay(row, appointment.repeatDay);
  weekdays = 0;
  for (i=0; i<7; i++)
    if (appointment.repeatDays[i])
      weekdays += 1<<i;
  setRowRepeatWeekdays(row, weekdays);
  setRowRepeatWeekstart(row, appointment.repeatWeekstart);
  setRowRepeatExceptionNum(row, appointment.exceptions);
  setRowRepeatException(row, appointment.exception);

  setRowDescription(row, appointment.description);
  setRowNote(row, appointment.note);
}


/* UNTIMED_EVENT (=1) or APPOINTMENT? (=0) */
void
setRowUntimed(struct row_data * row, int untimed)
{
  row->appointment.event = untimed;
}


/* Begin of appointment */
void
setRowBegin(struct row_data * row, struct tm begin)
{
  row->appointment.begin = begin;
}


/* Begin date of appointment */
void
setRowBeginDate(struct row_data * row, struct tm begin)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowBegin(row);
  begin.tm_sec = tm.tm_sec;
  begin.tm_min = tm.tm_min;
  begin.tm_hour = tm.tm_hour;

  setRowBegin(row, begin);
}


/* Begin time of appointment */
void
setRowBeginTime(struct row_data * row, long begin)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowBegin(row);
  tm.tm_sec = begin % 60;
  tm.tm_min = (begin/60) % 60;
  tm.tm_hour = (begin/(60*60)) % 24;

  setRowBegin(row, tm);
}


/* End of appointment */
void
setRowEnd(struct row_data * row, struct tm end)
{
  row->appointment.end = end;
}


/* End date of appointment */
void
setRowEndDate(struct row_data * row, struct tm end)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowEnd(row);
  end.tm_sec = tm.tm_sec;
  end.tm_min = tm.tm_min;
  end.tm_hour = tm.tm_hour;

  setRowEnd(row, end);
}


/* End time of appointment */
void
setRowEndTime(struct row_data * row, long end)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowEnd(row);
  tm.tm_sec = end % 60;
  tm.tm_min = (end/60) % 60;
  tm.tm_hour = (end/(60*60)) % 24;

  setRowEnd(row, tm);
}


/* Alarm. yes (=1) or no (=0) */
void
setRowAlarm(struct row_data * row, int alarm)
{
  row->appointment.alarm = alarm;
}


/* Advance: alarm should be how much in advance (number)? */
void
setRowAdvance(struct row_data * row, int advance)
{
  row->appointment.advance = advance;
}


/* AdvanceUnit: alarm should be how much in advance (days, months, mins)? */
void
setRowAdvanceUnit(struct row_data * row, int advanceUnit)
{
  row->appointment.advanceUnits = advanceUnit;
}


/* Repeat Type: none, daily, weekly, monthlyByDay, monthlyByDate, yearly */
void
setRowRepeatType(struct row_data * row, int repeatType)
{
  row->appointment.repeatType =repeatType;
}


/* Repeat Forever: yes (1) or no (0) */
void
setRowRepeatForever(struct row_data * row, int repeatForever)
{
  row->appointment.repeatForever =repeatForever;
}


/* Repeat End: last date of repeat event */
void
setRowRepeatEnd(struct row_data * row, struct tm repeatEnd)
{
  row->appointment.repeatEnd = repeatEnd;
}


/* Repeat end date */
void
setRowRepeatEndDate(struct row_data * row, struct tm repeatEnd)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowRepeatEnd(row);
  repeatEnd.tm_sec = tm.tm_sec;
  repeatEnd.tm_min = tm.tm_min;
  repeatEnd.tm_hour = tm.tm_hour;

  setRowRepeatEnd(row, repeatEnd);
}


/* Repeat end time */
void
setRowRepeatEndTime(struct row_data * row, long repeatEnd)
{
  struct tm tm;


  /* Keep seconds, minutes, hours */
  tm = getRowRepeatEnd(row);
  tm.tm_sec = repeatEnd % 60;
  tm.tm_min = (repeatEnd/60) % 60;
  tm.tm_hour = (repeatEnd/(60*60)) % 24;

  setRowRepeatEnd(row, tm);
}


/* Repeat Frequency: event occurs every time (1) or every x-th time (x) */
void
setRowRepeatFrequency(struct row_data * row, int repeatFrequency)
{
  row->appointment.repeatFrequency = repeatFrequency;
}


/* Repeat Day: which day of the month should we repeat (for repeatMonthlyByDay)
 * 27 => on the 27th every month
 */
void
setRowRepeatDay(struct row_data * row, int repeatDay)
{
  row->appointment.repeatDay = (enum DayOfMonthType) repeatDay;
}


/* Repeat Weekdays: Should we repeat on given weekday (for repeatWeekly):
 * yes (1) or no (0)?
 * Input parameter int will be converted into bit array:
 * Sun => weekdays = 1, Mon => weekdays = 2, Tue => weekdays = 4,
 * Sun + Tue => weekdays = 1 + 4 = 5
 */
void
setRowRepeatWeekdays(struct row_data * row, int weekdays)
{
  int i;


  for (i=0; i<7; i++)
    if ((weekdays>>i) & 1)
      row->appointment.repeatDays[i] = 1;
    else
      row->appointment.repeatDays[i] = 0;
}


/* Repeat Weekstart: Which day of the week starts week (for repeatWeekly):
 * Sun => weekday = 0, Mon => weekday = 1
 */
void
setRowRepeatWeekstart(struct row_data * row, int repeatWeekstart)
{
  row->appointment.repeatWeekstart = repeatWeekstart;
}


/* Number of Repeat Exceptions: How many repetitions should not take place? */
void
setRowRepeatExceptionNum(struct row_data * row, int exceptionNum)
{
  row->appointment.exceptions = exceptionNum;
}


/* Repeat Exception: Give pointer to array of all repetition exceptions */
void
setRowRepeatException(struct row_data * row, struct tm * exception)
{
  row->appointment.exception = exception;
}


/* Description: Description of appointment */
void
setRowDescription(struct row_data * row, char * description)
{
  if (row->appointment.description != description) {
    /* Free old description string */
    if (row->appointment.description != NULL)
      free ((void *) row->appointment.description);

    if (description == NULL
	|| *description == '\0')
      row->appointment.description = NULL;
    else
      row->appointment.description = strdup(description);
  }
}


/* Note: Note of apointment */
void
setRowNote(struct row_data * row, char * note)
{
  if (row->appointment.note != note) {
    /* Free old note string */
    if (row->appointment.note != NULL)
      free ((void *) row->appointment.note);

    if (note == NULL
	|| *note == '\0')
      row->appointment.note = NULL;
    else
      row->appointment.note = strdup(note);
  }
}


/* Transient calculation/scratch fields */
/* xLong */
void
setRowXLong(struct row_data * row, unsigned long xLong)
{
  row->xLong = xLong;
}


/* yLong */
void
setRowYLong(struct row_data * row, unsigned long yLong)
{
  row->yLong = yLong;
}


/* zLong */
void
setRowZLong(struct row_data * row, unsigned long zLong)
{
  row->zLong = zLong;
}


/* xInt */
void
setRowXInt(struct row_data * row, int xInt)
{
  row->xInt = xInt;
}


/* yInt */
void
setRowYInt(struct row_data * row, int yInt)
{
  row->yInt = yInt;
}


/* zInt */
void
setRowZInt(struct row_data * row, int zInt)
{
  row->zInt = zInt;
}


/* xTime */
void
setRowXTime(struct row_data * row, struct tm xTime)
{
  row->xTime = xTime;
}


/* yTime */
void
setRowYTime(struct row_data * row, struct tm yTime)
{
  row->yTime = yTime;
}


/* zTime */
void
setRowZTime(struct row_data * row, struct tm zTime)
{
  row->zTime = zTime;
}


/* xSeconds */
void
setRowXSeconds(struct row_data * row, long xSeconds)
{
  row->xSeconds = xSeconds;
}


/* ySeconds */
void
setRowYSeconds(struct row_data * row, long ySeconds)
{
  row->ySeconds = ySeconds;
}


/* zSeconds */
void
setRowZSeconds(struct row_data * row, long zSeconds)
{
  row->zSeconds = zSeconds;
}


/* xStr */
void
setRowXStr(struct row_data * row, char * xStr)
{
  if (row->xStr != xStr) {
    /* Free old string */
    if (row->xStr != NULL)
      free ((void *) row->xStr);

    if (xStr == NULL
	|| *xStr == '\0')
      row->xStr = NULL;
    else
      row->xStr = strdup(xStr);
  }
}


/* yStr */
void
setRowYStr(struct row_data * row, char * yStr)
{
  if (row->yStr != yStr) {
    /* Free old string */
    if (row->yStr != NULL)
      free ((void *) row->yStr);

    if (yStr == NULL
	|| *yStr == '\0')
      row->yStr = NULL;
    else
      row->yStr = strdup(yStr);
  }
}


/* zStr */
void
setRowZStr(struct row_data * row, char * zStr)
{
  if (row->zStr != zStr) {
    /* Free old string */
    if (row->zStr != NULL)
      free ((void *) row->zStr);

    if (zStr == NULL
	|| *zStr == '\0')
      row->zStr = NULL;
    else
      row->zStr = strdup(zStr);
  }
}
