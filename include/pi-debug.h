/* 
 * pi-debug.h: Debugging utilities
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

#ifndef _PILOT_DEBUG_H_
#define _PILOT_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-config.h"
#include "pi-args.h"

#define PI_DBG_NONE 0x000
#define PI_DBG_SYS  0x001
#define PI_DBG_DEV  0x002
#define PI_DBG_SLP  0x004
#define PI_DBG_PADP 0x008
#define PI_DBG_DLP  0x010
#define PI_DBG_NET  0x020
#define PI_DBG_CMP  0x040
#define PI_DBG_SOCK 0x080
#define PI_DBG_API  0x100
#define PI_DBG_USER 0x200
#define PI_DBG_ALL  0x400

#define PI_DBG_LVL_NONE  0x00
#define PI_DBG_LVL_ERR   0x01
#define PI_DBG_LVL_WARN  0x02
#define PI_DBG_LVL_INFO  0x04
#define PI_DBG_LVL_DEBUG 0x08

extern int pi_debug_get_types  PI_ARGS((void));
extern void pi_debug_set_types  PI_ARGS((int types));

extern int pi_debug_get_level  PI_ARGS((void));
extern void pi_debug_set_level  PI_ARGS((int level));

extern void pi_debug_set_file PI_ARGS((const char *path));

extern void pi_log PI_ARGS((int type, int level, char *format, ...));

#ifdef DEBUG
#define ASSERT(expr)                                            \
     do {                                                       \
       if (!(expr))						\
         pi_log (PI_DBG_SYS, PI_DBG_LVL_ERR,              \
	         "file %s: line %d: assertion failed: (%s)",	\
	         __FILE__,                                      \
	         __LINE__,                                      \
	         #expr);                                        \
     } while (0);

#define CHECK(type, level, expr)                                \
     do {                                                       \
       if ((pi_debug_get_types () & type)			\
           && pi_debug_get_level () >= level)			\
         expr;                                                  \
     } while (0);

#define LOG(l, s, f...) pi_log(l, s, f)

#else
#define ASSERT(expr)
#define LOG(l, s, f, a...)
#define CHECK(type, level, expr)
#endif

#ifdef __cplusplus
}
#endif
#endif



