/*
 * pi-error.h:  definitions for errors returned by the SOCKET, DLP and
 *              FILE layers
 *
 * Copyright (c) 2004, Florent Pillet.
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
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 */
#ifndef _PILOT_ERROR_H_
#define _PILOT_ERROR_H_

/*
 * Error code definitions
 *
 * Note that these error codes are tailored to not conflict
 * with dlpErr* codes defined in dlp.h, and which can be
 * checked using pi_palmos_error()
 *
 */

/* PROTOCOL level errors */
#define PI_ERR_PROT_ABORTED			-100		/* aborted by other end */
#define PI_ERR_PROT_INCOMPATIBLE	-101		/* can't talk with other end */
#define PI_ERR_PROT_BADPACKET		-102		/* bad packet (used with serial protocols) */

/* SOCKET level errors */
#define PI_ERR_SOCK_DISCONNECTED	-200		/* connection has been broken */
#define PI_ERR_SOCK_INVALID			-201		/* invalid protocol stack */
#define PI_ERR_SOCK_TIMEOUT			-202		/* communications timeout (but link not known as broken) */
#define	PI_ERR_SOCK_CANCELED		-203		/* last data transfer was canceled */
#define PI_ERR_SOCK_IO				-204		/* generic I/O error */
#define PI_ERR_SOCK_LISTENER		-205		/* socket can't listen/accept */

/* DLP level error */
#define PI_ERR_DLP_BUFSIZE			-300		/* provided buffer is not big enough to store data */
#define PI_ERR_DLP_PALMOS			-301		/* a non-zero error was returned by the device */
#define PI_ERR_DLP_UNSUPPORTED		-302		/* this DLP call is not supported by the connected handheld */
#define PI_ERR_DLP_SOCKET			-303		/* invalid socket */
#define	PI_ERR_DLP_DATASIZE			-304		/* requested transfer with data block too large (>64k) */
#define PI_ERR_DLP_COMMAND			-305		/* command error (the device returned an invalid response) */

/* FILE level error */
#define PI_ERR_FILE_INVALID			-400		/* invalid prc/pdb/pqa/pi_file file */
#define PI_ERR_FILE_ERROR			-401		/* generic error when reading/writing file */
#define	PI_ERR_FILE_ABORTED			-402		/* file transfer was aborted by progress callback
												   see pi_file_retrieve(), pi_file_install(), pi_file_merge() */
#define PI_ERR_FILE_NOT_FOUND		-403		/* record or resource not found */

/* GENERIC errors */
#define PI_ERR_GENERIC_MEMORY		-500		/* not enough memory */
#define PI_ERR_GENERIC_ARGUMENT		-501		/* invalid argument(s) */
#define PI_ERR_GENERIC_SYSTEM		-502		/* generic system error */

/* Macros */
#define IS_PROT_ERR(error)			((error)<=-100 && (error)>-200)
#define IS_SOCK_ERR(error)			((error)<=-200 && (error)>-300)
#define IS_DLP_ERR(error)			((error)<=-300 && (error)>-400)
#define IS_FILE_ERR(error)			((error)<=-400 && (error)>-500)
#define IS_GENERIC_ERR(error)		((error)<=-500 && (error)>-600)

#endif
