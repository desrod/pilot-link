#ifndef _PILOT_SOCKADDR_H_
#define _PILOT_SOCKADDR_H_

struct pi_sockaddr {
  unsigned short pi_family;
  char 	pi_device[255];
};

#endif /* _PILOT_SOCKADDR_H_ */
