#define TCP_PROTOCOL 6
#define TERMINATE             ("STOP")
#define OK                      0
#define HOSTNAME_LOOKUP_ERROR (-1)
#define PORT_IN_USE_ERROR     (-2)
#define CONNECT_ERROR         (-3)
#define TALK_ERROR            (-4)
#define SOCKET_ERROR          (-5)

int make_connection_with(char *hostname, int port, int *connection);
int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int set_up_listener(int port, int *listener);
int wait_for_connection(int listen_socket, int *connection);
int safe_send(int connection, void *data, size_t length);
int safe_recv(int connection, void *data, size_t max);
int get_my_ip(char *buffer);
int get_ip(int connection, char *buffer);
