#ifndef _PILOT_MACROS_H_
#define _PILOT_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long recordid_t;

#define get_long(ptr) ((((unsigned char*)(ptr))[0] << 24) | \
                       (((unsigned char*)(ptr))[1] << 16) | \
                       (((unsigned char*)(ptr))[2] << 8)  | \
                       (((unsigned char*)(ptr))[3] << 0))

#define get_treble(ptr) ((((unsigned char*)(ptr))[0] << 16) | \
                         (((unsigned char*)(ptr))[1] << 8)  | \
                         (((unsigned char*)(ptr))[2] << 0))
                       
#define get_short(ptr) ((((unsigned char*)(ptr))[0] << 8)  | \
                        (((unsigned char*)(ptr))[1] << 0))
                        
#define get_byte(ptr) (((unsigned char*)(ptr))[0])

#define set_long(ptr,val) ((((unsigned char*)(ptr))[0] = ((val) >> 24) & 0xff), \
		          (((unsigned char*)(ptr))[1] = ((val) >> 16) & 0xff), \
		          (((unsigned char*)(ptr))[2] = ((val) >> 8) & 0xff), \
		          (((unsigned char*)(ptr))[3] = ((val) >> 0) & 0xff))

#define set_treble(ptr,val) ((((unsigned char*)(ptr))[0] = ((val) >> 16) & 0xff), \
		             (((unsigned char*)(ptr))[1] = ((val) >> 8) & 0xff), \
		             (((unsigned char*)(ptr))[2] = ((val) >> 0) & 0xff))
                       
#define set_short(ptr,val) ((((unsigned char*)(ptr))[0] = ((val) >> 8) & 0xff), \
		            (((unsigned char*)(ptr))[1] = ((val) >> 0) & 0xff))

#define set_byte(ptr,val) (((unsigned char*)(ptr))[0]=(val))

#define char4(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_MACROS_H_ */
