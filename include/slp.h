#ifndef _PILOT_PADP_SLP_H_
#define _PILOT_PADP_SLP_H_

#define SLP_RDCP_T 0
#define SLP_PADP_T 2
#define SLP_LOOP_T 3

struct slp {
  unsigned char _be;
  unsigned char _ef;
  unsigned char _ed;
  unsigned char dest;
  unsigned char src;
  unsigned char type;
  unsigned short dlen;
  unsigned char id;
  unsigned char csum;
};

int slp_tx(struct pi_socket *ps, struct pi_skb *nskb, int len);
int slp_rx(struct pi_socket *ps);

#endif /* _PILOT_PADP_SLP_H_ */
