#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#include <sys/time.h>

struct ToDo {
  struct tm due;
  int priority;
  int complete;
  char * description;
  char * note;
};

void free_ToDo(struct ToDo *);
void unpack_ToDo(struct ToDo *, unsigned char * record, int len);
void pack_ToDo(struct ToDo *, unsigned char * record, int * len);


#endif /* _PILOT_TODO_H_ */
