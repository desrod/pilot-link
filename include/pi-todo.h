#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#ifdef __cplusplus
extern "C" {
#endif

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

extern void free_ToDo(struct ToDo *);
extern void unpack_ToDo(struct ToDo *, unsigned char * record, int len);
extern void pack_ToDo(struct ToDo *, unsigned char * record, int * len);
extern void unpack_ToDoAppInfo(struct ToDoAppInfo *, unsigned char * record, int len);
extern void pack_ToDoAppInfo(struct ToDoAppInfo *, unsigned char * record, int * len);

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_TODO_H_ */
