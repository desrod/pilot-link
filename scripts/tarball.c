#include <stdio.h>

#include "pi-version.h"

int main(void)
{
	char dir[80],cmd[256];
	sprintf(dir, "pilot-link-%d.%d.%d", PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
	sprintf(cmd, "rm -f tarball; cd ..; rm -f %s; rm -f %s.tar.gz; ln -s pilot-link %s", dir, dir, dir);
	printf("%s\n",cmd);
	system(cmd);
	sprintf(cmd, "cd ..; tar chvf %s.tar %s/", dir, dir);
	printf("%s\n",cmd);
	fflush(stdout);
	system(cmd);
	sprintf(cmd, "cd ..; gzip %s.tar", dir);
	printf("%s\n",cmd);
	system(cmd);
	sprintf(cmd, "cd ..; rm -f %s", dir);
	printf("%s\n",cmd);
	system(cmd);
}
