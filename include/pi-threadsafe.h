/*
 * pi-threadsafe.h: utilities for thread-safe behavior
 *
 * Copyright (c) 2005, Florent Pillet.
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 *
 * -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 */
#ifndef _PILOT_THREADSAFE_H
#define _PILOT_THREADSAFE_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#if HAVE_PTHREAD

	#include <pthread.h>

	#define PI_THREADSAFE 1

	#define PI_MUTEX_DECLARE(mutex_name)	pthread_mutex_t mutex_name
	#define PI_MUTEX_DEFINE(mutex_name)	pthread_mutex_t mutex_name = PTHREAD_MUTEX_INITIALIZER

	typedef pthread_mutex_t pi_mutex_t;

#else
	/* when not in thread-safe mode, we still use dummy variables
	 * the code will simply do nothing
	 */
	#define PI_THREADSAFE 0

	#define PI_MUTEX_DECLARE(mutex_name)	int mutex_name
	#define	PI_MUTEX_DEFINE(mutex_name)	int mutex_name = 0	/* dummy declaration for the code to compile */

	typedef int pi_mutex_t;						/* ditto */

#endif

extern int pi_mutex_lock(pi_mutex_t *mutex);

extern int pi_mutex_trylock(pi_mutex_t *mutex);

extern int pi_mutex_unlock(pi_mutex_t *mutex);

extern unsigned long pi_thread_id();

#endif
