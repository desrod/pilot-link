/*
 * md5.h:  Header file for Colin Plumb's MD5 implementation.
 *         Modified by Ian Jackson so as not to use Colin Plumb's
 *         'usuals.h'.
 * 
 *         This file is in the public domain.
 */

#ifndef MD5_H
#define MD5_H

#define UINT8 unsigned char
#define UINT32 unsigned long

struct MD5Context {
	UINT32 buf[4];
	UINT32 bytes[2];
	UINT32 in[16];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, UINT8 const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(UINT32 buf[4], UINT32 const in[16]);

void byteSwap(UINT32 * buf, unsigned words);

#endif				/* !MD5_H */
