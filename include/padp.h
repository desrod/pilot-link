#ifndef _PILOT_PADP_H_
#define _PILOT_PADP_H_

#include <pi-socket.h>

#define padData   1
#define padWake   0x101
#define padAck    2
#define padTickle 4

#define FIRST 0x80
#define LAST  0x40
#define MEMERROR 0x20

struct padp {
  unsigned char type;
  unsigned char flags;
  unsigned short size;
};

int padp_tx(struct pi_socket *ps, void *msg, int len, int type);
int padp_rx(struct pi_socket *ps, void *buf, int len);
int padp_dump(struct pi_skb *skb, struct padp *padp, int rxtx);

#endif /* _PILOT_PADP_H_ */
