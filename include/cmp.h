
#ifndef _PILOT_CMP_H_
#define _PILOT_CMP_H_

#define OurCommVersion 0x01000000

struct cmp {
  unsigned char type;
  unsigned char flags;
  unsigned long commversion;
  unsigned long baudrate;
};

int cmp_rx(struct pi_socket *ps, struct cmp * c);

int cmp_init(struct pi_socket *ps, int baudrate);

int cmp_abort(struct pi_socket *ps, int reason);

int cmp_dump(unsigned char * cmp, int rxtx);

#endif /* _PILOT_CMP_H_ */
