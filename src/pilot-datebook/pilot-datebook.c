/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

          
#include "pilot-datebook.h"
#include "pilot-datebook-data.h"

#include "pilot-datebook-job.h"


/* Name used to call this program */
char * progname;
const char PILOT_DATEBOOK_VERSION[] = "0.98";

/* Public functions */
int
main (int argc, char **argv)
{
  char * progname;
  struct job_list joblist;


  /* Init */
  progname = argv[0];
  joblist_init(&joblist);

  /* Show usage if no parameters were passed on command line */
  if (argc == 1)
    usage (progname, "");

  /* Build up joblist with jobs from command line parameters */
  if (!joblist_set_param (&joblist, argc, argv))
    usage (progname, "");

  /* Processing using job structure
   * (errors will terminate application via joblist_abort_all() )
   */
  joblist_process(&joblist);

  /* Erase any remaining processing stuff printed on the screen */
  info_message("                                                  \r"); 

  /* Show statistics */
  joblist_statistics(&joblist);

  /* Cleanup */
  joblist_exit(&joblist);

  /* Exit */
  return (0);
}


/* Print usage */
void
usage (char * progname, char * opt_arg)
{

  fprintf (stderr, "pilot-datebook %s  (c) 2000 by Matthias Hessler\n",
	   PILOT_DATEBOOK_VERSION);

  if (opt_arg == NULL
      || opt_arg[0] == '\0') {
    fprintf (stderr, "\n");
    fprintf (stderr, "usage: %s [options] [[command] [cmd_options] [@optfile]...]\n", progname);
    fprintf (stderr, "Mostly: %s  -r <in_fmt> -f <in_file>   -w <out_fmt> -f <out_file>\n", progname);
    fprintf (stderr, "    or: %s  -r <in_fmt>  -w <out_fmt>   < <in_file>  > <out_file>\n", progname);
    fprintf (stderr, "[use %s -hconvert  to see command line examples]\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "[options]:\n");
    fprintf (stderr, "  -q          quiet mode\n");
    fprintf (stderr, "  -v          verbose/debug mode\n");
    fprintf (stderr, "  -h          show this help\n");
    fprintf (stderr, "  -hhelp      show overview on all available help screens\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "[command]:\n");
    fprintf (stderr, "  -i <cond>   only do next command if <cond> is true\n");
    fprintf (stderr, "  -r <fmt>    read file with format <fmt>  (ignoring -i)\n");
    fprintf (stderr, "  -w <fmt>    write file with format <fmt>\n");
    fprintf (stderr, "  -s <order>  sort rows by <order>  (ignoring -i)\n");
    fprintf (stderr, "  -u <assign> update rows according to <assign>\n");
    fprintf (stderr, "  -d          delete rows  (only useful with -i)\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "[cmd_options] (for read/write):\n");
    fprintf (stderr, "  -f <file>   read from <file>/write to <file> (default: stdin/stdout)\n");
  }
  else if (!strcmp(opt_arg, "help")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Available help screens:\n");
    fprintf (stderr, "  -h          show usage\n");
    fprintf (stderr, "  -hhelp      show overview on all help screens\n");
    fprintf (stderr, "  -hfield     show supported data field names\n");
    fprintf (stderr, "  -hftype     show supported data field types\n");
    fprintf (stderr, "  -hformat    show supported data formats for <fmt>\n");
    fprintf (stderr, "  -hif        show information on how to specify <cond> for conditions\n");
    fprintf (stderr, "  -hsort      show information on <order> for sorting\n");
    fprintf (stderr, "  -hupdate    show information on <assign> for updating\n");
    fprintf (stderr, "  -hconvert   show command line examples for data conversion (read/write)\n");
    fprintf (stderr, "  -hcsv       show information on csv import/export\n");
  }
  else if (!strcmp(opt_arg, "convert")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Examples for data conversion (read/write):\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* Drop records starting before 1971:\n");
    fprintf (stderr, "    %s -r pdb -f in.pdb -i \"%s<1Jan1971\" -d -w pdb -f out.pdb\n",
	     progname, DATEBOOK_FIELD_BEGIN_DATE);
    fprintf (stderr, "* Export in.pdb into clear text file out.txt:\n");
    fprintf (stderr, "    %s  -r pdb -f in.pdb  -w longtxt -f out.txt\n", progname);
    fprintf (stderr, "* Import clear text file in.txt and write desktop file datebook.dat:\n");
    fprintf (stderr, "    %s  -r longtxt -f in.txt  -w windat -f datebook.dat\n", progname);
    fprintf (stderr, "* Behave like pilot-xfer -f DatebookDB (read hotsync, write to pdb):\n");
    fprintf (stderr, "    %s  -r hotsync -f /dev/pilot  -w pdb -f DatebookDB.pdb\n", progname);
    fprintf (stderr, "* Behave like install-datebook (read shorttxt, write hotsync):\n");
    fprintf (stderr, "    %s  -r shorttxt <in_file>  -w hotsync -f /dev/pilot\n", progname);
    fprintf (stderr, "  or (for multiple files):\n");
    fprintf (stderr, "    cat <files> | %s  -r shorttxt  -w hotsync -f /dev/pilot\n", progname);
    fprintf (stderr, "* Behave like reminders (read hotsync, write to stdout):\n");
    fprintf (stderr, "    %s  -r hotsync -f /dev/pilot  -w remind\n", progname);
    fprintf (stderr, "* Behave like read-ical -d (read hotsync, pipe to ical):\n");
    fprintf (stderr, "    %s  -r hotsync -f /dev/pilot  -w ical | ical -f - -calendar <filename>\n", progname);
  }
  else if (!strcmp(opt_arg, "csv")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "CSV (comma-separated values) import/export (read/write):\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* The CSV format is well understood by many programs, especially\n");
    fprintf (stderr, "  by spreadsheet programs\n");
    fprintf (stderr, "* Two additional [cmd_options] are supported: -o and -t\n");
    fprintf (stderr, "* -o <output_value_list> specifies a comma separated list of output values\n");
    fprintf (stderr, "* The default for <output_value_list> includes all printable data fields\n");
    fprintf (stderr, "* All data fields can be used in <output_value_list>, even transient\n");
    fprintf (stderr, "  internal data fields like the x/y/z variables\n");
    fprintf (stderr, "* -t <header_value_list> specifies which header to write/expect in the file\n");
    fprintf (stderr, "* <header_value_list> can be empty to indicate that no header line should\n");
    fprintf (stderr, "  be written/expected in the csv file\n");
    fprintf (stderr, "* If <header_value_list> has been provided, then it has to match\n");
    fprintf (stderr, "* If <header_value_list> is just \"*\", then it will always match\n");
    fprintf (stderr, "* Field names may be quoted; unknown field names can be matched with '*'\n");
    fprintf (stderr, "* If only <output_value_list> was provided, then it will also be used\n");
    fprintf (stderr, "  as <header_value_list>\n");
    fprintf (stderr, "* If neither <output_value_list> nor <header_value_list> have been provided,\n");
    fprintf (stderr, "  then default will be used for writing, and encountered header for reading\n");
    fprintf (stderr, "* You can use 'Outlook' as <header_value_list> to get compatible format which\n");
    fprintf (stderr, "  can be read by Outlook. Outlook output can be difficult to read, since\n");
    fprintf (stderr, "  it uses the local date format for writing, which may be difficult to parse.\n");
    fprintf (stderr, "* Outlook does a different quoting for newlines, therefore avoid those for now.\n");
  }
  else if (!strcmp(opt_arg, "field")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Available data fields (for use in <fmt>, <order>, <assign>):\n");
    fprintf (stderr, "* %s (long)\n", DATEBOOK_FIELD_UID);
    fprintf (stderr, "* %s (int)\n", DATEBOOK_FIELD_ATTRIBUTES);
    fprintf (stderr, "* %s (int: 0=Unfiled)\n", DATEBOOK_FIELD_CATEGORY);
    fprintf (stderr, "* %s (int: 0=Appointment, 1=Untimed)\n", DATEBOOK_FIELD_UNTIMED);
    fprintf (stderr, "* %s (time) = %s (time) + %s (seconds)\n",
	     DATEBOOK_FIELD_BEGIN,
	     DATEBOOK_FIELD_BEGIN_DATE,
	     DATEBOOK_FIELD_BEGIN_TIME);
    fprintf (stderr, "* %s (time) = %s (time) + %s (seconds)\n",
	     DATEBOOK_FIELD_END,
	     DATEBOOK_FIELD_END_DATE,
	     DATEBOOK_FIELD_END_TIME);
    fprintf (stderr, "* %s (int: 0=no alarm, 1=alarm)\n", DATEBOOK_FIELD_ALARM);
    fprintf (stderr, "* %s (int), %s (int: 0=minutes, 1=hours, 2=days)\n",
	     DATEBOOK_FIELD_ADVANCE,
	     DATEBOOK_FIELD_ADVANCE_UNIT);
    fprintf (stderr, "* %s (int: 0=none, 1=daily, 2=weekly, 3=monthly, 4=monthly/weekday,\n", DATEBOOK_FIELD_REPEAT_TYPE);
    fprintf (stderr, "* %s (int: 0=not forever, 1=forever)\t\t\t      5=yearly)\n", DATEBOOK_FIELD_REPEAT_FOREVER);
    fprintf (stderr, "* %s (time)\n", DATEBOOK_FIELD_REPEAT_END);
    fprintf (stderr, "* %s (int)\n", DATEBOOK_FIELD_REPEAT_FREQUENCY);
    fprintf (stderr, "* %s (int: day# or 0..6=Sun..Sat 1st, 7..13 2nd, 14..20 3rd, 21..27 4th,\n", DATEBOOK_FIELD_REPEAT_DAY);
    fprintf (stderr, "* %s (int)\t\t\t\t\t\t28-34 last week)\n", DATEBOOK_FIELD_REPEAT_WEEKSTART);
    fprintf (stderr, "* %s (int: add - 1=Sun,2=Mon,4=Tue,8=Wed,16=Thu,32=Fri,64=Sat)\n", DATEBOOK_FIELD_REPEAT_WEEKDAYS);
    /* Hide from the casual user (danger of updates):
      fprintf (stderr, "* %s (int: DO NOT UPDATE DIRECTLY!)\n", DATEBOOK_FIELD_REPEAT_EXCEPTION_NUM);
    */
    fprintf (stderr, "* %s (str)\n", DATEBOOK_FIELD_DESCRIPTION);
    fprintf (stderr, "* %s (str)\n", DATEBOOK_FIELD_NOTE);
    fprintf (stderr, "* %s,%s,%s (long), %s,%s,%s (int), %s,%s,%s (time),\n* %s,%s,%s (seconds), %s,%s,%s (str)\n",
	     DATEBOOK_FIELD_XLONG,
	     DATEBOOK_FIELD_YLONG,
	     DATEBOOK_FIELD_ZLONG,
	     DATEBOOK_FIELD_XINT,
	     DATEBOOK_FIELD_YINT,
	     DATEBOOK_FIELD_ZINT,
	     DATEBOOK_FIELD_XTIME,
	     DATEBOOK_FIELD_YTIME,
	     DATEBOOK_FIELD_ZTIME,
	     DATEBOOK_FIELD_XSECONDS,
	     DATEBOOK_FIELD_YSECONDS,
	     DATEBOOK_FIELD_ZSECONDS,
	     DATEBOOK_FIELD_XSTR,
	     DATEBOOK_FIELD_YSTR,
	     DATEBOOK_FIELD_ZSTR);
  }
  else if (!strcmp(opt_arg, "ftype")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Available data field types:\n");
    fprintf (stderr, "* long (is 'unsigned long' in C)\n");
    fprintf (stderr, "    Examples: '123456789','987654321'\n");
    fprintf (stderr, "* int (is 'int' in C)\n");
    fprintf (stderr, "    Examples: '123456','654321'\n");
    fprintf (stderr, "* time (is 'struct tm' in C, but will be converted to time_t for calculation)\n");
    fprintf (stderr, "    Examples: '27 oct 1968 19:15','1968-10-27 19:15','27 oct 1968','1968-10-27'\n");
    fprintf (stderr, "* seconds (is 'long' in C)\n");
    fprintf (stderr, "  Modifiers 'd','m','h' are possible (days, minutes, hours);\n");
    fprintf (stderr, "  alternatively specify in form 'x:y:z' (hours:minutes:seconds).\n");
    fprintf (stderr, "  Time field assignments will use 'seconds modulo 86400'.\n");
    fprintf (stderr, "    Examples: '86400','1d','24:' (= 1 day)\n");
    fprintf (stderr, "    Examples: '3600','1h','1:' (= 1 hour)\n");
    fprintf (stderr, "    Examples: '60','1m','0:1' (= 1 minute)\n");
    fprintf (stderr, "    Examples: '1','0:0:1' (= 1 second)\n");
    fprintf (stderr, "* str (is 'char *' in C, requires strdup)\n");
    fprintf (stderr, "  String literals have to be surrounded by single or double quotes.\n");
    fprintf (stderr, "  String literals may quote characters with backslash (like printf).\n");
    fprintf (stderr, "    Examples: \"String1\", \"String2,line1\\n\\t\\\"String2\\\",line2\"\n");
    fprintf (stderr, "\n");
  }
  else if (!strcmp(opt_arg, "format")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Available data formats (for <fmt>):\n");
    fprintf (stderr, "* hotsync     direct connection to pilot (read & write)\n");
    fprintf (stderr, "* pdb         pilot database file format (read & write)\n");
    fprintf (stderr, "* windat      file format for Windows desktop (read & write)\n");
    fprintf (stderr, "* csv         comma-separated values format (read & write)\n");
    fprintf (stderr, "* longtxt     human-readable text file format (read & write)\n");
    fprintf (stderr, "* shorttxt    install-datebook import text file format (read & write)\n");
    fprintf (stderr, "* remind      reminders import text format (write only)\n");
    fprintf (stderr, "* ical        ical import text format (write only)\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Please note:\n");
    fprintf (stderr, "* File formats hotsync, pdb, windat, and longtxt are almost lossless\n");
    fprintf (stderr, "* csv can be lossless with the exception of repeat exceptions\n");
    fprintf (stderr, "* Use command line option -l to set the desktop file location for windat\n");
    fprintf (stderr, "* See separate help screen for csv format (-hcsv)\n");
  }
  else if (!strcmp(opt_arg, "if")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Condition:  -i \"<cond>\"\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* <cond> specifies a list of conditions\n");
    fprintf (stderr, "* Multiple conditions can be listed with ';' in between (= AND).\n");
    fprintf (stderr, "* Each condition consists of: <field> <operator> <value>\n");
    fprintf (stderr, "* <field>: has to be a field name\n");
    fprintf (stderr, "* Use %s -hfields  to see available data fields\n", progname);
    fprintf (stderr, "* <operator>: use '==' for equal, '!=' for not equal, '<' for less,\n");
    fprintf (stderr, "* '<=' for less or equal, '>' for greater, '>=' for greater or equal\n");
    fprintf (stderr, "* <value>: currently has to be a literal\n");
    fprintf (stderr, "* All listed conditions within <cond> are connected with AND\n");
    fprintf (stderr, "* Multiple definitions of -i will be connected with OR\n");
    fprintf (stderr, "\n");

    fprintf (stderr, "Example: -i \"%s=0\" -u \"%s=1;%s=5;%s=0\"\n",
	     DATEBOOK_FIELD_ALARM,
	     DATEBOOK_FIELD_ALARM,
	     DATEBOOK_FIELD_ADVANCE,
	     DATEBOOK_FIELD_ADVANCE_UNIT);
    fprintf (stderr, "                    [if no alarm is set, then set alarm to 5 minutes before]\n");
    fprintf (stderr, "     or: -i \"begin<27 oct 1988 10:13\" -d\n");
    fprintf (stderr, "                      [delete all rows which begin before 27 Oct 1988 10:13]\n");
  }
  else if (!strcmp(opt_arg, "sort")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Sort:  -s \"<order>\"\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* <order> specifies a list of sort fields\n");
    fprintf (stderr, "* Multiple sort fields can be listed with ';' in between\n");
    fprintf (stderr, "* Use %s -hfields  to see available data fields\n", progname);
    fprintf (stderr, "* Rows are compared first on their first sort field. If those are equal,\n");
    fprintf (stderr, "then the second (then third,...) sort fields are compared\n");
    fprintf (stderr, "* Add '+' for ascending, '-' for descending order before sort field name\n");
    fprintf (stderr, "* Use '-s \"\"' to buffer all rows unsorted (avoids straight-through processing)\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "Example: -s \"%s;%s;%s\"  [default = ascending sort order]\n",
	     DATEBOOK_FIELD_ALARM,
	     DATEBOOK_FIELD_ADVANCE_UNIT,
	     DATEBOOK_FIELD_ADVANCE);
    fprintf (stderr, "     or: -s \"-%s;-%s;-%s\"  [descending sort order]\n",
	     DATEBOOK_FIELD_ALARM,
	     DATEBOOK_FIELD_ADVANCE_UNIT,
	     DATEBOOK_FIELD_ADVANCE);
  }
  else if (!strcmp(opt_arg, "update")) {
    fprintf (stderr, "(use %s -h  to see usage)\n", progname);
    fprintf (stderr, "\n");
    fprintf (stderr, "Update:  -u \"<assign>\"\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* <assign> specifies a list of assignments\n");
    fprintf (stderr, "* Multiple assignments can be listed with ';' in between.\n");
    fprintf (stderr, "* Each assignment consists of: <field> <operator> <value>\n");
    fprintf (stderr, "* <field>: has to be a field name\n");
    fprintf (stderr, "* Use %s -hfields  to see available data fields\n", progname);
    fprintf (stderr, "* <operator>: use '=' to assign, '+=' to increase, '-=' to decrease\n");
    fprintf (stderr, "* <value>: currently has to be a literal\n");
    fprintf (stderr, "* String literals have to be surrounded by single or double quotes\n");
    fprintf (stderr, "* String literals may quote characters with backslash (like printf)\n");
    fprintf (stderr, "* All listed updates are carried out\n");
    fprintf (stderr, "\n");

    fprintf (stderr, "Example: -u \"%s=1;%s=5;%s=0\"  [set alarm: 5 minutes before]\n",
	     DATEBOOK_FIELD_ALARM,
	     DATEBOOK_FIELD_ADVANCE,
	     DATEBOOK_FIELD_ADVANCE_UNIT);
    fprintf (stderr, "     or: -u \"%s+='\\nadditional\\tcomment\\;line1\\nline2'\"   [add text to note]\n",
	     DATEBOOK_FIELD_NOTE);
  }
  else {
    usage (progname, "");
  }

  /* Terminate program */
  exit (1);
}
