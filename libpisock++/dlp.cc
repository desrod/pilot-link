
/* dlp.cc:
 *
 * Copyright (c) 1997, 1998, Scott Grosch
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

#include <string.h>
#include <iostream.h>
using namespace std;

extern "C" {
#include <stdio.h>
#include "pi-source.h"
#include "pi-socket.h"
}

#include "pi-dlp.hxx"

DLP::DLP(strConst_t device, const int showMsg) 
{
     if (!(_sd = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_DLP))) {
          perror("pi_socket");
          return;
     }
     
     struct pi_sockaddr addr;
     addr.pi_family = PI_AF_PILOT;
     strcpy(addr.pi_device, device);
 
     int ret;
     if ((ret = pi_bind(_sd, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
          perror("pi_bind");
	  _sd = -1;
          return;
     }

     if (showMsg)
	  cout << "Waiting for connection (press the HotSync button)..."
	       << endl;
 
     if ((ret = pi_listen(_sd,1)) == -1) {
          perror("pi_listen");
          pi_close(_sd);
	  _sd = -1;
          return;
     }
 
     if ((_sd = pi_accept(_sd, 0, 0)) == -1) {
          perror("pi_accept");
          pi_close(_sd);
	  _sd = -1;
          return;
     }
}
