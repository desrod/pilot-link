#ifndef _PILOT_SOCKADDR_H_
#define _PILOT_SOCKADDR_H_

#include <config.h>

#ifdef HAVE_SA_LEN
struct pi_sockaddr {
  unsigned char pi_len;
  unsigned char pi_family;
  char pi_device[255];
};
#else
struct pi_sockaddr {
  unsigned short pi_family;
  char pi_device[255];
};
#endif

#endif /* _PILOT_SOCKADDR_H_ */
