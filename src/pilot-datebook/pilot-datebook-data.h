/*
 * Pilot Datebook processing utility
 * Matthias Hessler <pilot-datebook@mhessler.de> December 2000
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */

#ifndef _PILOT_DATEBOOK_DATA
#define _PILOT_DATEBOOK_DATA
          
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> /* isspace */
#include <stdarg.h> /* va_arg, vprintf stuff */

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-file.h"
#include "pi-datebook.h"


#ifdef sun
extern char* optarg;
extern int optind;
#endif


/* Public constants */
extern const int DATEBOOK_MAX_CATEGORIES;
extern const int WEEKDAY_LEN;
extern const int MONTH_LEN;

/* Field names */
extern const char DATEBOOK_FIELD_UID[];
extern const char DATEBOOK_FIELD_ATTRIBUTES[];
extern const char DATEBOOK_FIELD_CATEGORY[];
extern const char DATEBOOK_FIELD_UNTIMED[];
extern const char DATEBOOK_FIELD_BEGIN[];
extern const char DATEBOOK_FIELD_BEGIN_DATE[];
extern const char DATEBOOK_FIELD_BEGIN_TIME[];
extern const char DATEBOOK_FIELD_END[];
extern const char DATEBOOK_FIELD_END_DATE[];
extern const char DATEBOOK_FIELD_END_TIME[];
extern const char DATEBOOK_FIELD_ALARM[];
extern const char DATEBOOK_FIELD_ADVANCE[];
extern const char DATEBOOK_FIELD_ADVANCE_UNIT[];
extern const char DATEBOOK_FIELD_REPEAT_TYPE[];
extern const char DATEBOOK_FIELD_REPEAT_FOREVER[];
extern const char DATEBOOK_FIELD_REPEAT_END[];
extern const char DATEBOOK_FIELD_REPEAT_END_DATE[];
extern const char DATEBOOK_FIELD_REPEAT_END_TIME[];
extern const char DATEBOOK_FIELD_REPEAT_FREQUENCY[];
extern const char DATEBOOK_FIELD_REPEAT_DAY[];
extern const char DATEBOOK_FIELD_REPEAT_WEEKSTART[];
extern const char DATEBOOK_FIELD_REPEAT_WEEKDAYS[];
extern const char DATEBOOK_FIELD_REPEAT_EXCEPTION_NUM[];
/* Leave out repeatException for now
 * (complicated array compare)
 */
extern const char DATEBOOK_FIELD_DESCRIPTION[];
extern const char DATEBOOK_FIELD_NOTE[];

/* Transient calculation/scratch fields */
extern const char DATEBOOK_FIELD_XLONG[];
extern const char DATEBOOK_FIELD_YLONG[];
extern const char DATEBOOK_FIELD_ZLONG[];
extern const char DATEBOOK_FIELD_XINT[];
extern const char DATEBOOK_FIELD_YINT[];
extern const char DATEBOOK_FIELD_ZINT[];
extern const char DATEBOOK_FIELD_XTIME[];
extern const char DATEBOOK_FIELD_YTIME[];
extern const char DATEBOOK_FIELD_ZTIME[];
extern const char DATEBOOK_FIELD_XSECONDS[];
extern const char DATEBOOK_FIELD_YSECONDS[];
extern const char DATEBOOK_FIELD_ZSECONDS[];
extern const char DATEBOOK_FIELD_XSTR[];
extern const char DATEBOOK_FIELD_YSTR[];
extern const char DATEBOOK_FIELD_ZSTR[];


/* FIXME: Fixes for AIX 4.3.3 Need testers */
#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif
/* End AIX fix */


/* enum */
typedef enum { FALSE=0, TRUE=1 } BOOLEAN;

/* File data format */
enum DATA_FORMAT {
  /* invalid data format */
  DATA_FORMAT_INVALID = 0,

  /* direct hotsync to PalmPilot via pilotlink */
  DATA_FORMAT_HOTSYNC = 1,

  /* PDB data format (= file format of pilotlink) */
  DATA_FORMAT_PDB = 2,

  /* CSV separated list */
  DATA_FORMAT_CSV = 3,

  /* Windows desktop file format (*.dat) */
  DATA_FORMAT_WINDAT = 4,

  /* human readable long text format */
  DATA_FORMAT_LONGTXT = 5,

  /* former install-datebook input */
  DATA_FORMAT_SHORTTXT = 6,

  /* former reminders output */
  DATA_FORMAT_REMIND = 7,

  /* former read-ical output */
  DATA_FORMAT_ICAL = 8
};


/* Message verbose level */
enum MESSAGE_VERBOSE_LEVEL {
  MESSAGE_VERBOSE_ERROR = 0,
  MESSAGE_VERBOSE_WARN = 1,
  MESSAGE_VERBOSE_INFO = 2,
  MESSAGE_VERBOSE_DEBUG = 3
};


/* Field types */
enum DATEBOOK_FIELD_TYPE {
  DATEBOOK_FIELD_LONG = 0,
  DATEBOOK_FIELD_INT = 1,
  DATEBOOK_FIELD_TIME = 2,
  DATEBOOK_FIELD_SECONDS = 3,
  DATEBOOK_FIELD_STR = 4
};


/* Assign types */
enum DATEBOOK_ASSIGN_TYPE {
  DATEBOOK_ASSIGN = 0,
  DATEBOOK_ASSIGN_ADD = 1,
  DATEBOOK_ASSIGN_SUBTRACT = 2
};


/* Condition types */
enum DATEBOOK_COND_TYPE {
  DATEBOOK_COND_EQUAL = 0,
  DATEBOOK_COND_NOT_EQUAL = 1,
  DATEBOOK_COND_LESS = 2,
  DATEBOOK_COND_LESS_EQUAL = 3,
  DATEBOOK_COND_GREATER = 4,
  DATEBOOK_COND_GREATER_EQUAL = 5
};


/* Selection types */
enum DATEBOOK_SELECT_TYPE {
  DATEBOOK_SELECT_UNKNOWN = 0,
  DATEBOOK_SELECT_YES = 1,
  DATEBOOK_SELECT_NO = 2
};




/* Data structures */

/* Datebook header data
 * (read data will be converted into this data form, and write data will
 * be formatted out of this record data)
 */
struct header_data {

  /* Valid data in header structure? */
  int isValid;

  /* Pilotlink header structure */
  struct DBInfo info;

  /* Datebook application header data */
  int app_info_size;
  struct AppointmentAppInfo aai;

  /* Datebook sort header data */
  void * sort_info;
  int sort_info_size;
};


/* Datebook record data
 * (read data will be converted into this data form, and write data will
 * be formatted out of this record data)
 */
struct row_data {

  /* Valid data in row structure? */
  int isValid;

  /* Has row been selected by compare function? */
  enum DATEBOOK_SELECT_TYPE isSelected;

  /* Row number within read file (=temporary data) */
  int record_num;


  /* Unique identifying number */
  unsigned long uid;

  /* Status Attributes (backed up, deleted, added, ...) */
  int attributes;

  /* Category (0 = Unfiled) */
  int category;

  /* Pilotlink data structure for datebook data */
  struct Appointment appointment;


  /* Transient calculation/scratch fields
   * (will not be stored, but can be used for intermediate calculations)
   */

  /* Long */
  unsigned long xLong;
  unsigned long yLong;
  unsigned long zLong;

  /* Int */
  int xInt;
  int yInt;
  int zInt;

  /* Time */
  struct tm xTime;
  struct tm yTime;
  struct tm zTime;

  /* Seconds */
  long xSeconds;
  long ySeconds;
  long zSeconds;

  /* Str */
  char * xStr;
  char * yStr;
  char * zStr;
}; 



/* Datebook field data
 * (contains generic details on how to access datebook data fields,
 * used for sort, modify,...)
 */
struct field_data {
  enum DATEBOOK_FIELD_TYPE type;
  const char * name;
  union {
    unsigned long (*get_long) (struct row_data * row);
    int (*get_int) (struct row_data * row);
    struct tm (*get_time) (struct row_data * row);
    long (*get_seconds) (struct row_data * row);
    char * (*get_str) (struct row_data * row);
  } get_func;
  union {
    void (*set_long) (struct row_data * row, unsigned long new_long);
    void (*set_int) (struct row_data * row, int new_int);
    void (*set_time) (struct row_data * row, struct tm new_time);
    void (*set_seconds) (struct row_data * row, long new_seconds);
    void (*set_str) (struct row_data * row, char * new_str);
  } set_func;
};


/* Datebook value data
 * (contains values to/of datebook data fields,
 * used in expressions)
 */
struct value_data {
  enum DATEBOOK_FIELD_TYPE type;
  union {
    /* Literal */
    unsigned long lit_long;
    int lit_int;
    struct tm lit_time;
    long lit_seconds;
    char * lit_str;
  } literal;
};


/* Datebook expression data
 * (contains values to/of datebook data fields,
 * used in assignments)
 */
struct expr_data {
  char * name;
  int isField;
  union {
    /* Constant value */
    struct value_data constant;

    /* Field */
    struct field_data field;
  } content;
};


/* Datebook assignment data
 * (contains generic details on how to assign values to datebook data fields,
 * used for update)
 */
struct assign_data {
  char * name;
  struct field_data left;
  enum DATEBOOK_ASSIGN_TYPE op;
  enum DATEBOOK_FIELD_TYPE expr_type;
  struct expr_data right;
};


/* Datebook condition data
 * (contains generic details on how to compare datebook data fields,
 * used for if)
 */
struct cond_data {
  char * name;
  struct field_data left;
  enum DATEBOOK_COND_TYPE op;
  enum DATEBOOK_FIELD_TYPE expr_type;
  struct expr_data right;
};



/* Functions */

/* From parsedate.c */
extern time_t parsedate(char * p);

/* public */
enum DATA_FORMAT txt2dataformat (char * txt);
const char * dataformat2txt (enum DATA_FORMAT format);
const char * int2weekday (int weekday_num);
int weekday2int (char * weekday_str);
const char * int2month (int month_num);
int month2int (char * month_str);

/* Helper functions */
void write_dump (FILE * out_file, void *buf, int n);
void write_human_date_str (time_t t, char * buffer, int buffer_size);
void write_human_time_str (time_t t, char * buffer, int buffer_size);
void write_human_gmtime_str (time_t t, char * buffer, int buffer_size);
void write_human_full_time_str (time_t t, char * buffer, int buffer_size);
void write_iso_date_str (time_t t, char * buffer, int buffer_size);
void write_iso_time_str (time_t t, char * buffer, int buffer_size);
void write_iso_gmtime_str (time_t t, char * buffer, int buffer_size);
void write_iso_full_time_str (time_t t, char * buffer, int buffer_size);

time_t read_iso_date_str1 (char * date_buffer);
time_t read_iso_time_str4n (int num_read, char * buffer1, char * buffer2, char * buffer3, char * buffer4);
time_t read_iso_time_str2 (char * date_buffer, char * time_buffer);
time_t read_iso_time_str1 (char * date_buffer);

/* String handling */
char * data_index(const char * search_string, char search_char);
char * data_rindex(const char * search_string, char search_char);
int data_stricmp(const char * pos1, const char * pos2);
int data_strincmp(const char * pos1, const char * pos2, int max_len);
void text_quote(const char * in_start, char * out_start, int max_len);
void text_unquote(const char * in_start, char * out_start, int max_len);

/* Error handling */
void error_message(char * format, ...);
void warn_message(char * format, ...);
void info_message(char * format, ...);
void debug_message(char * format, ...);
enum MESSAGE_VERBOSE_LEVEL get_message_verbose (void);
void set_message_verbose (enum MESSAGE_VERBOSE_LEVEL verbose_level);


/* Find delimiter location */
char * data_find_delimiter (const char * pos1, char delimiter);

/* Value handling */
void value_init (struct value_data * value, const char * value_start, const char * value_end);
void value_exit (struct value_data * value);

/* Field handling */
void field_init (struct field_data * field, const char * field_start, const char * field_end);
void field_exit (struct field_data * field);
int field_cmp_rows (struct field_data * field, struct row_data * row1, struct row_data * row2);
struct value_data row_get_field (struct row_data * row, struct field_data * field);
void row_set_field (struct row_data * row, struct field_data * field, struct value_data * value);

/* Expression handling */
void expr_init (struct expr_data * expr, enum DATEBOOK_FIELD_TYPE expect_type, const char * expr_start, const char * expr_end);
void expr_exit (struct expr_data * expr);
struct value_data expr_row (struct expr_data * expr, struct row_data * row);

/* Assignment handling */
void assign_init (struct assign_data * assign, const char * assign_start, const char * assign_end);
void assign_exit (struct assign_data * assign);
void assign_row (struct assign_data * assign, struct row_data * row);

/* Condition handling */
void cond_init (struct cond_data * cond, const char * cond_start, const char * cond_end);
void cond_exit (struct cond_data * cond);
int cond_row (struct cond_data * cond, struct row_data * row);


/* Getters */

/* General data */
int getRowIsValid(struct row_data * row);
enum DATEBOOK_SELECT_TYPE getRowIsSelected(struct row_data * row);
int getRowRecordNum(struct row_data * row);
unsigned long getRowUid(struct row_data * row);
int getRowAttributes(struct row_data * row);
int getRowCategory(struct row_data * row);

/* Datebook specific data */
struct Appointment getRowAppointment(struct row_data * row);
int getRowUntimed(struct row_data * row);
struct tm getRowBegin(struct row_data * row);
struct tm getRowBeginDate(struct row_data * row);
long getRowBeginTime(struct row_data * row);
struct tm getRowEnd(struct row_data * row);
struct tm getRowEndDate(struct row_data * row);
long getRowEndTime(struct row_data * row);
int getRowAlarm(struct row_data * row);
int getRowAdvance(struct row_data * row);
int getRowAdvanceUnit(struct row_data * row);
int getRowRepeatType(struct row_data * row);
int getRowRepeatForever(struct row_data * row);
struct tm getRowRepeatEnd(struct row_data * row);
struct tm getRowRepeatEndDate(struct row_data * row);
long getRowRepeatEndTime(struct row_data * row);
int getRowRepeatFrequency(struct row_data * row);
int getRowRepeatDay(struct row_data * row);
int getRowRepeatWeekdays(struct row_data * row);
int getRowRepeatWeekstart(struct row_data * row);
int getRowRepeatExceptionNum(struct row_data * row);
struct tm * getRowRepeatException(struct row_data * row);
char * getRowDescription(struct row_data * row);
char * getRowNote(struct row_data * row);

/* Transient calculation/scratch fields */
unsigned long getRowXLong(struct row_data * row);
unsigned long getRowYLong(struct row_data * row);
unsigned long getRowZLong(struct row_data * row);
int getRowXInt(struct row_data * row);
int getRowYInt(struct row_data * row);
int getRowZInt(struct row_data * row);
struct tm getRowXTime(struct row_data * row);
struct tm getRowYTime(struct row_data * row);
struct tm getRowZTime(struct row_data * row);
long getRowXSeconds(struct row_data * row);
long getRowYSeconds(struct row_data * row);
long getRowZSeconds(struct row_data * row);
char * getRowXStr(struct row_data * row);
char * getRowYStr(struct row_data * row);
char * getRowZStr(struct row_data * row);



/* Setters */

/* General data */
void rowInit(struct row_data * row);
void rowExit(struct row_data * row);

void setRowIsValid(struct row_data * row, int isValid);
void setRowIsSelected(struct row_data * row, enum DATEBOOK_SELECT_TYPE isSelected);
void setRowRecordNum(struct row_data * row, int record_num);
void setRowUid(struct row_data * row, unsigned long uid);
void setRowAttributes(struct row_data * row, int attributes);
void setRowCategory(struct row_data * row, int category);

/* Datebook specific data */
void setRowAppointment(struct row_data * row, struct Appointment appointment);
void setRowUntimed(struct row_data * row, int untimed);
void setRowBegin(struct row_data * row, struct tm begin);
void setRowBeginDate(struct row_data * row, struct tm begin);
void setRowBeginTime(struct row_data * row, long begin);
void setRowEnd(struct row_data * row, struct tm end);
void setRowEndDate(struct row_data * row, struct tm end);
void setRowEndTime(struct row_data * row, long end);
void setRowAlarm(struct row_data * row, int alarm);
void setRowAdvance(struct row_data * row, int advance);
void setRowAdvanceUnit(struct row_data * row, int advanceUnit);
void setRowRepeatType(struct row_data * row, int repeatType);
void setRowRepeatForever(struct row_data * row, int repeatForever);
void setRowRepeatEnd(struct row_data * row, struct tm repeatEnd);
void setRowRepeatEndDate(struct row_data * row, struct tm repeatEnd);
void setRowRepeatEndTime(struct row_data * row, long repeatEnd);
void setRowRepeatFrequency(struct row_data * row, int repeatFrequency);
void setRowRepeatDay(struct row_data * row, int repeatDay);
void setRowRepeatWeekdays(struct row_data * row, int repeatWeekdays);
void setRowRepeatWeekstart(struct row_data * row, int repeatWeekstart);
void setRowRepeatExceptionNum(struct row_data * row, int exceptionNum);
void setRowRepeatException(struct row_data * row, struct tm * exception);
void setRowDescription(struct row_data * row, char * description);
void setRowNote(struct row_data * row, char * note);

/* Transient calculation/scratch fields */
void setRowXLong(struct row_data * row, unsigned long xLong);
void setRowYLong(struct row_data * row, unsigned long yLong);
void setRowZLong(struct row_data * row, unsigned long zLong);
void setRowXInt(struct row_data * row, int xInt);
void setRowYInt(struct row_data * row, int yInt);
void setRowZInt(struct row_data * row, int zInt);
void setRowXTime(struct row_data * row, struct tm xTime);
void setRowYTime(struct row_data * row, struct tm yTime);
void setRowZTime(struct row_data * row, struct tm zTime);
void setRowXSeconds(struct row_data * row, long xSeconds);
void setRowYSeconds(struct row_data * row, long ySeconds);
void setRowZSeconds(struct row_data * row, long zSeconds);
void setRowXStr(struct row_data * row, char * xStr);
void setRowYStr(struct row_data * row, char * xStr);
void setRowZStr(struct row_data * row, char * xStr);




#endif
