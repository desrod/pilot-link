#ifndef _PILOT_MACROS_H_
#define _PILOT_MACROS_H_

#include <sys/time.h> /* for struct tm */

typedef unsigned long recordid_t;

#ifndef __cplusplus

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

#else /*ifdef __cplusplus*/

inline unsigned long get_long(const void *buf) 
{
     unsigned char *ptr = (unsigned char *) buf;

     return (*ptr << 24) | (*(++ptr) << 16) | (*(++ptr) << 8) | *(++ptr);
}

inline unsigned long get_treble(const void *buf) 
{
     unsigned char *ptr = (unsigned char *) buf;

     return (*ptr << 16) | (*(++ptr) << 8) | *(++ptr);
}

inline int get_short(const void *buf) 
{
     unsigned char *ptr = (unsigned char *) buf;

     return (*ptr << 8) | *(++ptr);
}

inline int get_byte(const void *buf) 
{
     return *((unsigned char *) buf);
}

inline void set_long(void *buf, const unsigned long val) 
{
     unsigned char *ptr = (unsigned char *) buf;

     *ptr = (val >> 24) & 0xff;
     *(++ptr) = (val >> 16) & 0xff;
     *(++ptr) = (val >> 8) & 0xff;
     *(++ptr) = val & 0xff;
}

inline void set_treble(void *buf, const unsigned long val) 
{
     unsigned char *ptr = (unsigned char *) buf;
     
     *ptr = (val >> 16) & 0xff;
     *(++ptr) = (val >> 8) & 0xff;
     *(++ptr) = val & 0xff;
}

inline void set_short(void *buf, const int val) 
{
     unsigned char *ptr = (unsigned char *) buf;

     *ptr = (val >> 8) & 0xff;
     *(++ptr) = val & 0xff;
}

inline void set_byte(void *buf, const int val) 
{
     *((unsigned char *)buf) = val;
}

inline struct tm *getBufTm(struct tm *t, const void *buf, bool setTime) 
{
     unsigned short int d = get_short(buf);
     
     t->tm_year = (d >> 9) + 4;
     t->tm_mon = ((d >> 5) & 15) - 1;
     t->tm_mday = d & 31;

     if (setTime) {
	  t->tm_hour = 0;
	  t->tm_min = 0;
	  t->tm_sec = 0;
     }
     
     t->tm_isdst = -1;

     mktime(t);
     
     return t;
}

inline void setBufTm(void *buf, const struct tm *t)
{
     set_short(buf,
	       ((t->tm_year - 4) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday);
}

inline unsigned long char4(char c1, char c2, char c3, char c4)
{
     return (c1<<24)|(c2<<16)|(c3<<8)|c4;
}

#endif /*__cplusplus*/

#endif /* _PILOT_MACROS_H_ */
