#ifndef __PILOT_ARGS_H__
#define __PILOT_ARGS_H__

#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus) || defined(USE_PROTOTYPE) || defined(CAN_PROTOTYPE)
#   define PI_ARGS(x)       x
#   define PI_CONST const
#else
#   define PI_ARGS(x)       ()
#   define PI_CONST
#endif

#endif
