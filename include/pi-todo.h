#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#include <time.h>
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct ToDo {
		int indefinite;
		struct tm due;
		int priority;
		int complete;
		char *description;
		char *note;
	} ToDo_t;

	typedef struct ToDoAppInfo {
		struct CategoryAppInfo category;
		int dirty;
		int sortByPriority;
	} ToDoAppInfo_t;

	extern void free_ToDo PI_ARGS((ToDo_t *));
	extern int unpack_ToDo
	    PI_ARGS((ToDo_t *, unsigned char *record, size_t len));
	extern int pack_ToDo
	    PI_ARGS((ToDo_t *, unsigned char *record, size_t len));
	extern int unpack_ToDoAppInfo
	    PI_ARGS((ToDoAppInfo_t *, unsigned char *record, size_t len));
	extern int pack_ToDoAppInfo
	    PI_ARGS((ToDoAppInfo_t *, unsigned char *record, size_t len));

#ifdef __cplusplus
}
#include "pi-todo.hxx"
#endif				/*__cplusplus*/
#endif				/* _PILOT_TODO_H_ */
