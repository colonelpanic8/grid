void bulletin_exit(int errcode);
int bulletin_make_connection_with(char *hostname, int port, int *connection);
int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int bulletin_set_up_listener(int port, int *listener);
int bulletin_wait_for_connection(int listen_socket, int *connection);
int safe_send(int connection, void *data, size_t length);
int safe_recv(int connection, void *data, size_t max);
int get_my_ip(char *buffer);
int get_ip(int connection, char *buffer);
