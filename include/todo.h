#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#include <sys/time.h>

struct ToDo {
  int indefinite;
  struct tm due;
  int priority;
  int complete;
  char * description;
  char * note;
};

struct ToDoAppInfo {
  int renamedcategories;
  char CategoryName[16][16];
  unsigned char CategoryID[16];
  unsigned char lastUniqueID;
  int dirty;
  int sortByPriority;
};

void free_ToDo(struct ToDo *);
void unpack_ToDo(struct ToDo *, unsigned char * record, int len);
void pack_ToDo(struct ToDo *, unsigned char * record, int * len);
void unpack_ToDoAppInfo(struct ToDoAppInfo *, unsigned char * record, int len);
void pack_ToDoAppInfo(struct ToDoAppInfo *, unsigned char * record, int * len);

#endif /* _PILOT_TODO_H_ */
