#ifndef _PI_HEADER_H_
#define _PI_HEADER_H_

#include "pi-util.h"

/* Print the version splash 	*/
void print_splash(const char *progname) PI_DEPRECATED;

/* Connect to the Palm device	*/
int pilot_connect(const char *port) PI_DEPRECATED;

#endif /* _PI_HEADER_H_ */
