
#ifndef _PILOT_CMP_H_
#define _PILOT_CMP_H_

#define CommVersion_1_0 0x0100L
#define CommVersion_2_0 0x0101L

struct cmp {
  unsigned char type;
  unsigned char flags;
  unsigned int version;
  int reserved;
  unsigned long baudrate;
};

extern int cmp_rx(struct pi_socket *ps, struct cmp * c);

extern int cmp_init(struct pi_socket *ps, int baudrate);

extern int cmp_abort(struct pi_socket *ps, int reason);

extern int cmp_wakeup(struct pi_socket *ps, int maxbaud);

extern void cmp_dump(unsigned char * cmp, int rxtx);


#endif /* _PILOT_CMP_H_ */
