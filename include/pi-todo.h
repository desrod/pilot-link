#ifndef _PILOT_TODO_H_
#define _PILOT_TODO_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct ToDo {
		int 	indefinite,
			priority,
			complete;
		char 	*description,
			*note;
		struct 	tm due;
	};

	struct ToDoAppInfo {
		int 	dirty,
			sortByPriority;
		struct 	CategoryAppInfo category;
	};

	extern void free_ToDo PI_ARGS((struct ToDo *));
	extern int unpack_ToDo
		PI_ARGS((struct ToDo *, unsigned char *record, int len));
	extern int pack_ToDo
		PI_ARGS((struct ToDo *, unsigned char *record, int len));
	extern int unpack_ToDoAppInfo
		PI_ARGS((struct ToDoAppInfo *, unsigned char *record,
			int len));
	extern int pack_ToDoAppInfo
		PI_ARGS((struct ToDoAppInfo *, unsigned char *record,
			int len));

#ifdef __cplusplus
}
#include "pi-todo.hxx"
#endif				/* __cplusplus */
#endif				/* _PILOT_TODO_H_ */
