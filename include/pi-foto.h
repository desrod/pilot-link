/*
 * File: include/pi-foto.h - XXX
 * 
 * Copyright (C) 2004 Steve Ratcliffe
 * 
 * Author: Steve Ratcliffe
 * Create date: 23 Feb 2004
 */

#ifndef _PI_FOTO_H_
#define _PI_FOTO_H_

/*
 * Offsets into the thumbnail record header.
 */
#define FOTO_IMAGE_WIDTH  0x04 /* Width of the image jpg. 2 bytes */
#define FOTO_IMAGE_HEIGHT 0x06 /* Height of the image jpg. 2 bytes */
#define FOTO_MODIFY_DATE  0x08 /* Date modified. 4 bytes */
#define FOTO_IMAGE_SIZE   0x0c /* (Related to) the size of the jpg pdb, 4b */
#define FOTO_THUMB_WIDTH  0x10 /* Width of thumbnail. 1 byte */
#define FOTO_THUMB_HEIGHT 0x11 /* Height of thumbnail. 1 byte */
#define FOTO_NAME_LEN     0x16 /* Length of name. 1 byte(?) */
#define FOTO_IMAGE_DATE   0x1c /* Date displayed with image. 4 bytes */
#define FOTO_THUMB_SIZE   0x26 /* Size of thumbnail. 2 bytes */

#endif /* _PI_FOTO_H_ */
