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

#define WM_POKEHOTSYNC            WM_USER + 0xbac5
#define PHS_QUIET_WM_CLOSE        1
#define HOTSYNC_APP_CLASS         "KittyHawk"

int main(int argc, char *argv[])
{

	/* obtain the handle for the HotSync manager's window */
	HWND hWnd = FindWindow(HOTSYNC_APP_CLASS, NULL);

	/* if the HotSync manager is already running, send the exit message */
	if (hWnd != NULL) {

		/* Send a message to the HotSync manager asking it to
		   terminate without displaying the "Are you sure?"
		   confirmation dialog. */
		SendMessage(hWnd, WM_POKEHOTSYNC, PHS_QUIET_WM_CLOSE, 0);
	}
	return 0;
}
