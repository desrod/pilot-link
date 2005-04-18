/*
 * pi-header.h
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */
#ifndef _PI_HEADER_H_
#define _PI_HEADER_H_

#include "pi-util.h"

#ifndef SWIG
/* Print the version splash 	*/
void print_splash(const char *progname) PI_DEPRECATED;

/* Connect to the Palm device	*/
int pilot_connect(const char *port) PI_DEPRECATED;
#endif

#endif /* _PI_HEADER_H_ */
