/*
 * threadsafe.c: utilities for thread-safe behavior
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

#include "pi-threadsafe.h"

int pi_mutex_lock(pi_mutex_t *mutex)
{
#if HAVE_PTHREAD
	return pthread_mutex_lock(mutex);
#else
	return 0;
#endif
}

int pi_mutex_trylock(pi_mutex_t *mutex)
{
#if HAVE_PTHREAD
	return pthread_mutex_trylock(mutex);
#else
	return 0;
#endif
}

int pi_mutex_unlock(pi_mutex_t *mutex)
{
#if HAVE_PTHREAD
	return pthread_mutex_unlock(mutex);
#else
	return 0;
#endif
}

unsigned long pi_thread_id()
{
#if HAVE_PTHREAD
	return (unsigned long)pthread_self();
#else
	return 0;
#endif
}

#include "pi-threadsafe.h"

