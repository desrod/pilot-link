#ifndef _PILOT_PDB_H_
#define _PILOT_PDB_H_

typedef struct pdb {
  char AppName[32];     /* Application Name, NULL terminated string */
  int prcversion;       /* Guess: The version of .prc format used   */
  int version;          /* File version, as set during creation     */
  long date1;		/* Unknown date (Mac epoch!)                */
  long date2;		/* Unknown date (Mac epoch!)                */
  char PrcMagic1[16];   /* A whole bunch constant bytes. A key?     */
  long Type;            /* File 'type'                              */
  long Creator;         /* File 'creator'                           */
  char PrcMagic2[8];    /* More constant bytes, same for all files  */
  int NumSections;      /* Number of sections in this file          */
} pdb_t;

#define SIZEOF_PDB 78

/* In my opinion, both "Magic" chunks are simply unused fields that weren't
   cleared to nulls by the Mac tools. - KJA */

/* Section header */
typedef struct sect {
  long Offset;
  unsigned char Attr;
  unsigned char Category;
  long ID;
} pdb_sect_t;

#define SIZEOF_PDB_SECT 8

int LoadPDB(int sd, char * fname, int cardno);
int RetrievePDB(int sd, char * dname, char * fname, int cardno);

#endif