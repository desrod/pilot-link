// -*-C-*-

//
//  Socket stuff (from pi-socket.h)
//
#define PI_AF_PILOT             0x00

#define PI_PF_DEV               0x01
#define PI_PF_SLP               0x02
#define PI_PF_SYS               0x03
#define PI_PF_PADP              0x04
#define PI_PF_NET               0x05
#define PI_PF_DLP               0x06

#define PI_SOCK_STREAM          0x0010
#define PI_SOCK_RAW             0x0030

#define PI_CMD_CMP 0x01
#define PI_CMD_NET 0x02
#define PI_CMD_SYS 0x03

#define PI_MSG_PEEK 0x01

enum PiOptLevels {
        PI_LEVEL_DEV,
        PI_LEVEL_SLP,
        PI_LEVEL_PADP,
        PI_LEVEL_NET,
        PI_LEVEL_SYS,
        PI_LEVEL_CMP,
        PI_LEVEL_DLP,
        PI_LEVEL_SOCK
};

enum PiOptDevice {
        PI_DEV_RATE,
        PI_DEV_ESTRATE,
        PI_DEV_HIGHRATE,
        PI_DEV_TIMEOUT
};

enum PiOptSLP {
        PI_SLP_DEST,
        PI_SLP_LASTDEST,
        PI_SLP_SRC,
        PI_SLP_LASTSRC,
        PI_SLP_TYPE,
        PI_SLP_LASTTYPE,
        PI_SLP_TXID,
        PI_SLP_LASTTXID
};

enum PiOptPADP {
        PI_PADP_TYPE,
        PI_PADP_LASTTYPE
};

enum PiOptCMP {
        PI_CMP_TYPE,
        PI_CMP_FLAGS,
        PI_CMP_VERS,
        PI_CMP_BAUD
};

enum PiOptNet {
        PI_NET_TYPE
};

enum PiOptSock {
        PI_SOCK_STATE
};

#define PI_PilotSocketDLP       3
#define PI_PilotSocketConsole   1
#define PI_PilotSocketDebugger  0
#define PI_PilotSocketRemoteUI  2


extern int pi_socket (int domain, int type, int protocol);
extern int pi_connect (int pi_sd, const char *port);
extern int pi_bind (int pi_sd, const char *port);
extern int pi_listen (int pi_sd, int backlog);
extern int pi_accept (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);

extern int pi_accept_to (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT, int timeout);

extern ssize_t pi_send (int pi_sd, void *msg, size_t len, int flags);
extern ssize_t pi_recv (int pi_sd, pi_buffer_t *msg, size_t len, int flags);

extern int pi_read (int pi_sd, pi_buffer_t *msg, size_t len);
extern int pi_write (int pi_sd, void *msg, size_t len);

extern int pi_getsockname (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);
extern int pi_getsockpeer (int pi_sd, struct sockaddr *OUTPUT, size_t *OUTPUT);

extern int pi_setsockopt (int pi_sd, int level, int option_name, const void *option_value, size_t *option_len);
extern int pi_getsockopt (int pi_sd, int level, int option_name, void * option_value, size_t * option_len);

extern int pi_version (int pi_sd);

extern int pi_tickle (int pi_sd);
extern int pi_watchdog (int pi_sd, int interval);

extern int pi_close (int pi_sd);

extern int pi_error (int pi_sd);
