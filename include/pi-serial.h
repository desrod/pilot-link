int pi_device_open(char *tty, struct pi_socket *ps);
int pi_device_changebaud(struct pi_socket *ps);
int pi_device_close(struct pi_socket *ps);
int pi_socket_send(struct pi_socket *ps);
int pi_socket_flush(struct pi_socket *ps);
int pi_socket_read(struct pi_socket *ps);
