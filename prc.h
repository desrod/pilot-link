/* File header */

typedef struct prc {
  char AppName[32];     /* Application Name, NULL terminated string */
  char PrcMagic1[28];   /* A whole bunch constant bytes.   A key?   */
  char ApplString[4];   /* The 4 bytes "appl"                       */
  char AppID[4];        /* Application ID?  4 bytes                 */
  char PrcMagic2[8];    /* More constant bytes, same for all files  */
  short NumSections;    /* Number of sections in this file          */
} prc_t;

/* Section header */
typedef struct sect {
  char SectName[4];     /* Section "type" name                      */
  short Number;         /* ID of this section within section type   */
  short junk;
  short Offset;         /* Offset, begin of file to section's data  */
} sect_t;

