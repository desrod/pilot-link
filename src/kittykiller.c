/* 
 * kittykiller.c - Kill the HotSync Manager
 *
 * Copyright (c) 1999, Tilo Christ
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <windows.h>
#include <string.h>

#define WM_POKEHOTSYNC            WM_USER + 0xbac5

/* Terminate without displaying the "Are you sure?"
   confirmation dialog. */
#define PHS_QUIET_WM_CLOSE        1
#define HOTSYNC_APP_CLASS         "KittyHawk"

int main(int argc, char *argv[])
{
	/* obtain the handle for the HotSync Manager's window */
	HWND hWnd = FindWindow(HOTSYNC_APP_CLASS, NULL);

        printf("   .--------------------------------------------.\n"
               "   | (c) Copyright 1996-2004, pilot-link team   |\n"
               "   |   Join the pilot-link lists to help out.   |\n"
               "   `--------------------------------------------'\n\n");

	if (!hWnd) {
		printf("    HotSync Manager was not running... exiting.\n");
	} else {
		printf("    Shutting down HotSync Manager...\n");
		SendMessage(hWnd, WM_POKEHOTSYNC, PHS_QUIET_WM_CLOSE, 0);
	}

	printf("    Thank you for using pilot-link.\n\n"); 

	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
