/* File header */

#pragma pack(2)

typedef struct prc {
  char AppName[32];     /* Application Name, NULL terminated string */
  short prcversion;     /* Guess: The version of .prc format used   */
  short version;        /* File version, as set during creation     */
  long date1;		/* Unknown date (Mac epoch!)                */
  long date2;		/* Unknown date (Mac epoch!)                */
  char PrcMagic1[16];   /* A whole bunch constant bytes. A key?     */
  long Type;            /* File 'creator'                           */
  long Creator;         /* File 'type'                              */
  char PrcMagic2[8];    /* More constant bytes, same for all files  */
  short NumSections;    /* Number of sections in this file          */
} prc_t;

/* In my opinion, both "Magic" chunks are simply unused fields that weren't
   cleared to nulls by the Mac tools. - KJA */

/* Section header */
typedef struct sect {
  long Type;     /* Section "type" name                      */
  short ID;
  long Offset;
} sect_t;

#pragma pack(4)
