/* Print the version splash 	*/
void print_splash(const char *progname);

/* Connect to the Palm device	*/
int pilot_connect(const char *port);

/* Open the listen port for connecting to the Palm device */
int pilot_listen_open(const char *port);

/* Close the listen port */
void pilot_listen_close(int *parent_sd);

/* Wait for a new connection to the listen port */
int pilot_connect_wait(int parent_sd, const char *port);
