#include <stdio.h>
#include <pi-socket.h>
#include <pi-source.h>
#include <pi-dlp.h>

int
main(int argc, char *argv[])
{
	int sd;
	struct pi_sockaddr addr;
	char *port = "/dev/pilot";
	struct SysInfo sys;
	struct PilotUser user;
	
	if (!(sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_PADP))) {
		return -1;
	}

	addr.pi_family = PI_AF_PILOT;
	strncpy(addr.pi_device, port, sizeof(addr.pi_device));

	if (pi_bind(sd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		printf("\n   Unable to bind to port %s\n", port);
		perror("   Reason: pi_bind");
		printf("\n");
		pi_close(sd);
		return 0;
	}

	if (pi_listen(sd, 1) == -1) {
		printf("\n   Error listening on %s\n", port);
		perror("   Reason: pi_listen");
		printf("\n");
		pi_close(sd);
		return 0;
	}

	sd = pi_accept(sd, 0, 0);
	if (sd == -1) {
		printf("\n   Error accepting data on %s\n", port);
		perror("   Reason: pi_accept");
		printf("\n");
		pi_close(sd);
		return 0;
	}

	if (dlp_ReadSysInfo (sd, &sys) < 0)
		printf ("DLP SysInfo: ERROR");
	else
		printf ("**** DLP SysInfo: %lu %lu %d\n", sys.romVersion, sys.locale, sys.nameLength);
	
	if (dlp_ReadUserInfo (sd, &user) < 0)
		printf ("DLP UserInfo: ERROR");
	else
		printf ("**** DLP UserInfo: %lu %lu %lu %lu %lu %s\n", user.userID, user.viewerID,
			user.lastSyncPC, user.successfulSyncDate, user.lastSyncDate, user.username);
	pi_close (sd);

	return 0;
}
