#ifndef _PILOT_PADP_H_
#define _PILOT_PADP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pi-socket.h"
#ifdef OS2
#include <time.h>
#endif

#define padData   1
#define padWake   0x101
#define padAck    2
#define padTickle 4
#define padAbort 8 /* PalmOS 2.0 only */

#define FIRST 0x80
#define LAST  0x40
#define MEMERROR 0x20

struct padp {
  unsigned char type;
  unsigned char flags;
  unsigned short size;
};

#define SIZEOF_PADP 4

extern int padp_tx(struct pi_socket *ps, void *msg, int len, int type);
extern int padp_rx(struct pi_socket *ps, void *buf, int len);
extern void padp_dump(struct pi_skb *skb, struct padp* padp, int rxtx);

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_PADP_H_ */
