#ifndef _PILOT_INET_H_
#define _PILOT_INET_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PI_NET_DEV     1

	typedef struct pi_inet_data {
		/* Time out */
		int timeout;
		
		/* Statistics */
		int rx_bytes;
		int rx_errors;

		int tx_bytes;
		int tx_errors;
	} pi_inet_data_t;

	extern pi_device_t *pi_inet_device
            PI_ARGS((int type));

#ifdef __cplusplus
}
#endif
#endif
