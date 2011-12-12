#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include "constants.h"
#include "network.h"

int build_address(char *hostname, int port, struct sockaddr_in *inet_address) {
  struct hostent *host_entries;
  host_entries = gethostbyname(hostname);
  if (host_entries == NULL || host_entries->h_addrtype != AF_INET || host_entries->h_length != 4) {
    return HOSTNAME_LOOKUP_ERROR;
  } else {
    // success!
    inet_address->sin_family = AF_INET;
    inet_address->sin_addr.s_addr = *((uint32_t *)host_entries->h_addr_list[0]);
    inet_address->sin_port = port;
    return OK;
  }
}

int set_up_listener(int port, int *listener) {
  int listen_socket;
  char hostname[128];
  struct sockaddr_in address;
  int lookup_result, bind_result;

  // get a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL); 
  if (listen_socket == -1) return CONNECT_ERROR;

  if (gethostname(hostname,sizeof(hostname)) < 0) return CONNECT_ERROR;
  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;
  
  // bind it to a port on this machine
  bind_result = bind(listen_socket,(struct sockaddr *)&address,sizeof(address)); 
  if (bind_result == EADDRINUSE) return PORT_IN_USE_ERROR;
  if (bind_result < 0) return CONNECT_ERROR;

  // return the listener's socket descriptor
  *listener = listen_socket;
  return OK;
}


int wait_for_connection(int listen_socket, int *connection) {
  int bulletin_socket;

  // listen for a client's "connect" request
  if (listen(listen_socket, 1) < 0) return CONNECT_ERROR;

  // accept it, get a dedicated socket for connection with this client
  bulletin_socket = accept(listen_socket,NULL,NULL);
  if (bulletin_socket < 0) return CONNECT_ERROR;

  // return that client connection's socket descriptor
  *connection = bulletin_socket;
  return OK;
}
  
int make_connection_with(char *hostname, int port, int *connection) {
  int bulletin_socket;
  struct sockaddr_in address;
  int lookup_result, connect_result;

  // grab a new socket to connect with the server
  bulletin_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL);
  if (bulletin_socket == -1) return SOCKET_ERROR;

  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;

  // connect with that server
  connect_result = connect(bulletin_socket,(struct sockaddr *)&address,sizeof(address));
  if (connect_result < 0) return CONNECT_ERROR;

  // return the connection's socket descriptor
  *connection = bulletin_socket;
  return OK;
}

int get_ip(int connection, char *buffer) {
    struct sockaddr_in their_address;
    socklen_t length;
    int err;
    length = sizeof their_address;
    memset(&their_address, 0, sizeof(struct sockaddr_in));
    err = getpeername(connection, (struct sockaddr *)&their_address, &length);
    if(err < 0) {
      problem("get peer name failed %d\n", errno);
      return FAILURE;
    }
    if(!inet_ntop(AF_INET, &(their_address.sin_addr), buffer, INET_ADDRSTRLEN))
      problem("inet_ntop failed %d\n", errno);
    return OKAY;
}

int get_my_ip(char *buffer) {
  struct hostent *h;
  char name[BUFFER_SIZE];
  int i;
  gethostname(name, BUFFER_SIZE);
  h = gethostbyname(name);
  if(h && h->h_addr_list[0]) {
    inet_ntop(AF_INET, h->h_addr_list[0], buffer, INET_ADDRSTRLEN);
  } else {
    problem("gethostbyname did not provide an address for %s \n", name);
    return FAILURE;
  }
  return OKAY;
}

int safe_send(int connection, void *data, size_t length) {
  char *bits;
  size_t transmitted;
  int err;
  bits = data;
  err = 0;
  
  transmitted = htonl(length);
  err = send(connection, &transmitted, sizeof(transmitted), 0);
  errno = 0;
  if(err < sizeof(transmitted)) {    
    problem("Error sending size of transmission.  Only %d bytes sent.  errno: %d\n"
	    , err, errno);
    return TRANSMISSION_ERROR;
  }
  
  err = FAILURE;
  recv(connection, &err, sizeof(err), 0);
  if(err) {
    problem("Error in communication.  receiver was not ready. err was: %d\n", err);
    return RECEIVER_ERROR;
  }
    
  
  err = 0;
  transmitted = 0;
  while(transmitted < length) {
    err = send(connection, (void *)(data+transmitted), (length-transmitted), 0);
    if(err < 0) {
      problem("Error transmitting data.  Only %lu bytes sent. errno: %d\n", transmitted, errno);
      return TRANSMISSION_ERROR;
    }
    transmitted += err;
  }
  return transmitted;
}

int safe_recv(int connection, void *data, size_t max) {
  char *bits;
  size_t received;
  uint32_t err;
  bits = data;
  err = 0;
  received = 0;

  err = recv(connection, &received, sizeof(received), 0);
  received = ntohl(received);
  if(err < sizeof(received)) {    
    problem("Error receiving size of transmission.  Only %d bytes received.  errno: %d\n"
	    , err, errno);
    return TRANSMISSION_ERROR;
  }
  
  if(received > max) {
    problem("Size of data to be sent was to too large. max: %lu, recv: %lu\n", max, received);
    err = 0;
    send(connection, &err, sizeof(err), 0);
    return RECEIVER_ERROR;
  }
  
  err = OKAY;
  send(connection, &err, sizeof(err), 0);
  max = received;
  received = 0;

  while(received < max) {
    err = recv(connection, (void *)(data + received), (max-received), 0);
    if(err < 0) {
      problem("Error received data.  Only %lu bytes received. errno: %d\n", received, errno);
      return TRANSMISSION_ERROR;
    }
    received += err;
  }
  return received;
}
