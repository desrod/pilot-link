#ifndef _PILOT_MACROS_H_
#define _PILOT_MACROS_H_

#define get_long(ptr) (((ptr)[0] << 24) | \
                       ((ptr)[1] << 16) | \
                       ((ptr)[2] << 8)  | \
                       ((ptr)[3] << 0))

#define get_treble(ptr) (((ptr)[0] << 16) | \
                         ((ptr)[1] << 8)  | \
                         ((ptr)[2] << 0))
                       
#define get_short(ptr) (((ptr)[0] << 8)  | \
                        ((ptr)[1] << 0))
                        
#define get_byte(ptr) ((ptr)[0])

#define set_long(ptr,val) (((ptr)[0] = ((val) >> 24) & 0xff), \
		          ((ptr)[1] = ((val) >> 16) & 0xff), \
		          ((ptr)[2] = ((val) >> 8) & 0xff), \
		          ((ptr)[3] = ((val) >> 0) & 0xff))

#define set_treble(ptr,val) (((ptr)[0] = ((val) >> 16) & 0xff), \
		             ((ptr)[1] = ((val) >> 8) & 0xff), \
		             ((ptr)[2] = ((val) >> 0) & 0xff))
                       
#define set_short(ptr,val) (((ptr)[0] = ((val) >> 8) & 0xff), \
		            ((ptr)[1] = ((val) >> 0) & 0xff))

#define set_byte(ptr,val) ((ptr)[0]=(val))

#define char4(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

#endif /* _PILOT_MACROS_H_ */
