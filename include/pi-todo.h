#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#include <time.h>
#include "pi-appinfo.h"
#include "pi-buffer.h"

	typedef enum {
		todo_v1,
	} todoType;

	typedef struct ToDo {
		int indefinite;
		struct tm due;
		int priority;
		int complete;
		char *description;
		char *note;
	} ToDo_t;

	typedef struct ToDoAppInfo {
		todoType type;
		struct CategoryAppInfo category;
		int dirty;
		int sortByPriority;
	} ToDoAppInfo_t;

	extern void free_ToDo PI_ARGS((ToDo_t *));
	extern int unpack_ToDo
	    PI_ARGS((ToDo_t *, pi_buffer_t *record, todoType type));
	extern int pack_ToDo
	    PI_ARGS((ToDo_t *, pi_buffer_t *record, todoType type));
	extern int unpack_ToDoAppInfo
	    PI_ARGS((ToDoAppInfo_t *, unsigned char *record, size_t len));
	extern int pack_ToDoAppInfo
	    PI_ARGS((ToDoAppInfo_t *, unsigned char *record, size_t len));

#endif				/* _PILOT_TODO_H_ */
