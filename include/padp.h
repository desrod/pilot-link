#ifndef _PILOT_PADP_H_
#define _PILOT_PADP_H_

#define padData   1
#define padWake   0x101
#define padAck    2
#define padTickle 3 /* or is it 4? */

#define FIRST 0x80
#define LAST  0x40

struct padp {
  unsigned char type;
  unsigned char flags;
  unsigned short size;
};


#endif /* _PILOT_PADP_H_ */
