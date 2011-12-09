#define TCP_PROTOCOL 6
#define BULLETIN_TERMINATE             ("STOP")
#define BULLETIN_OK                      0
#define BULLETIN_HOSTNAME_LOOKUP_ERROR (-1)
#define BULLETIN_PORT_IN_USE_ERROR     (-2)
#define BULLETIN_CONNECT_ERROR         (-3)
#define BULLETIN_TALK_ERROR            (-4)
#define BULLETIN_SOCKET_ERROR          (-5)

void bulletin_exit(int errcode);
int bulletin_make_connection_with(char *hostname, int port, int *connection);
int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int bulletin_set_up_listener(int port, int *listener);
int bulletin_wait_for_connection(int listen_socket, int *connection);
int safe_send(int connection, void *data, size_t length);
int safe_recv(int connection, void *data, size_t max);
int get_my_ip(char *buffer);
int get_ip(int connection, char *buffer);
