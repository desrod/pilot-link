#ifndef _PILOT_VEO_H_
#define _PILOT_VEO_H_

#include "pi-args.h"
#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VeoAppInfo {
   int 	dirty,
     sortByPriority;
   struct 	CategoryAppInfo category;
};

/* Actions */
#define VEO_ACTION_OUTPUT  0x01
#define VEO_ACTION_LIST    0x02
   
/* Output type */
#define VEO_OUT_PPM       0x01
#define VEO_OUT_PNG       0x02

typedef struct Veo {
   unsigned char   res1[1];
   unsigned char   quality;  // 0 = high, 1 = med, 2 = low this must mean
                             // something to the conduit because it doesn't
                             // change anything on the Palm
   unsigned char   resolution;     // 0 = 640x480, 1 = 320x240
   unsigned char   res2[12];
   unsigned long   picnum;
   unsigned short  day, month, year;
   
   // These are not in the palm db header. They're used by the decoder
   unsigned short  width, height;
   int  sd, db;
} Veo_t;

void free_Veo( struct Veo *v );
int unpack_Veo(struct Veo *v, unsigned char *buffer, int len);
int unpack_VeoAppInfo(struct VeoAppInfo *vai, unsigned char *record, int len);
int pack_Veo(struct Veo *v, unsigned char *buffer, int len);
int pack_VeoAppInfo(struct VeoAppInfo *vai, unsigned char *record, int len);

#ifdef __cplusplus
}
#endif				/*__cplusplus*/

#endif
