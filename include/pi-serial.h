#ifndef _PILOT_SERIAL_H_
#define _PILOT_SERIAL_H_

extern int pi_device_open(char *tty, struct pi_socket *ps);
extern int pi_device_changebaud(struct pi_socket *ps);
extern int pi_device_close(struct pi_socket *ps);
extern int pi_socket_send(struct pi_socket *ps);
extern int pi_socket_flush(struct pi_socket *ps);
extern int pi_socket_read(struct pi_socket *ps, int timeout);
#ifdef OS2
extern int pi_socket_set_timeout(struct pi_socket *ps, int read_timeout, 
			  int write_timeout);
#endif

#endif
