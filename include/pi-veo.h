#ifndef _PILOT_VEO_H_
#define _PILOT_VEO_H_

#include "pi-appinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VeoAppInfo {
	int 	dirty,
	sortByPriority;
	struct 	CategoryAppInfo category;
} VeoAppInfo_t;

/* Actions */
#define VEO_ACTION_OUTPUT      0x01
#define VEO_ACTION_LIST        0x02
#define VEO_ACTION_OUTPUT_ONE  0x04
   
/* Output type */
#define VEO_OUT_PPM       0x01
#define VEO_OUT_PNG       0x02

typedef struct Veo {
   unsigned char   res1[1];

   /* 0 = high, 1 = med, 2 = low this must mean something 
      to the conduit because it doesn't change anything 
      on the Palm */
   unsigned char   quality;

   /* 0 = 640x480, 1 = 320x240 */
   unsigned char   resolution;
   unsigned char   res2[12];
   unsigned long   picnum;
   unsigned short  day, month, year;
   
   /* These are not in the palm db header. They're used by the decoder */
   unsigned short  width, height;
   int  sd, db;
   char            name[32];
} Veo_t;

void free_Veo(Veo_t *v );
int unpack_Veo(Veo_t *v, unsigned char *buffer, size_t len);
int unpack_VeoAppInfo(VeoAppInfo_t *vai, unsigned char *record, size_t len);
int pack_Veo(Veo_t *v, unsigned char *buffer, size_t len);
int pack_VeoAppInfo(VeoAppInfo_t *vai, unsigned char *record, size_t len);

#ifdef __cplusplus
}
#endif				/*__cplusplus*/

#endif
