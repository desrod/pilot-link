/**
 * KittyKiller - Kill the HotSyncManager.
 * Written by Tilo Christ, 1999.
 */

#include <windows.h>

#define WM_POKEHOTSYNC            WM_USER + 0xbac5
#define PHS_QUIET_WM_CLOSE        1
#define HOTSYNC_APP_CLASS         "KittyHawk"

int main(int argc, char *argv[])
{

   // obtain the handle for the HotSync manager's window
   HWND hWnd = FindWindow(HOTSYNC_APP_CLASS, NULL);

   // if the HotSync manager is already running, send the exit message
   if (hWnd != NULL) {

      // Send a message to the HotSync manager asking it to terminate
      // without displaying the "Are you sure?" confirmation dialog.
      SendMessage(hWnd, WM_POKEHOTSYNC, PHS_QUIET_WM_CLOSE, 0);
   }
   return 0;
}
