#include <stdio.h>
#include <pi-socket.h>
#include <pi-source.h>
#include <pi-dlp.h>

int pilot_connect(const char *port);

int main(int argc, char *argv[])
{
	int sd = -1;
	char *port = argv[1];
	struct SysInfo S;
	struct PilotUser U;
	
	sd = pilot_connect(port);

	if (dlp_OpenConduit(sd) < 0) {
		exit(1);
	}

	if (dlp_ReadUserInfo (sd, &U) < 0)
		printf ("DLP UserInfo: ERROR\n");
	else
		printf ("**** DLP UserInfo: %lu %lu %lu %lu %lu %s\n", U.userID, U.viewerID,
			U.lastSyncPC, U.successfulSyncDate, U.lastSyncDate, U.username);

	if (dlp_ReadSysInfo (sd, &S) < 0)
		printf ("DLP SysInfo: ERROR\n");
	else
		printf ("**** DLP SysInfo: 0x%8.8lX, locale: 0x%8.8lX, name: '%s'\n", S.romVersion, S.locale, S.name);
	
	pi_close (sd);

	return 0;
}
