#include <stdio.h>

#include "pi-socket.h"

void main(void)
{
	char dir[80],cmd[256];
	sprintf(dir, "pilot-link.%d.%d.%d", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
	sprintf(cmd, "rm tarball; cd ..; rm %s; ln -s plk %s", dir, dir);
	printf("%s\n",cmd);
	system(cmd);
	sprintf(cmd, "cd ..; tar cvf %s.tar %s/", dir, dir);
	printf("%s\n",cmd);
	fflush(stdout);
	system(cmd);
	sprintf(cmd, "cd ..; gzip %s.tar", dir);
	printf("%s\n",cmd);
	system(cmd);
	sprintf(cmd, "cd ..; rm %s", dir);
	printf("%s\n",cmd);
	system(cmd);
}
