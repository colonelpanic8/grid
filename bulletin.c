// Math 442 DS: Distributed and Networked Systems
// Reed College
// Fall 2011
//
// bulletin.c
//
// This code implements a Unix executable that 
// can act either as a client or a server of a
// remote bulletin posting service.
//
// Run as "bulletin 20000", it will set up a
// bulletin server that receives a posts from
// a series of clients that connect to it, 
// listening on port 20000.
//
// Run as "bulletin bob.reed.edu", it will connect
// to a bulletin server listening on port 20000
// on the machine named bob.reed.edu, and send
// a single post on the machine.  The post can
// be entered by the program's user as a series
// of lines in STDIN, terminated by the single
// line with the four letters "STOP".
//
// It is a simple example of the Berkeley socket
// API for performing networked communications via
// TCP/IP.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include "constants.h"
#include "bulletin.h"

#define TCP_PROTOCOL 6

#define BULLETIN_TERMINATE             ("STOP")

#define BULLETIN_OK                      0
#define BULLETIN_HOSTNAME_LOOKUP_ERROR (-1)
#define BULLETIN_PORT_IN_USE_ERROR     (-2)
#define BULLETIN_CONNECT_ERROR         (-3)
#define BULLETIN_TALK_ERROR            (-4)

//
// recv_string
//
// Receives a series of bytes from a socket, of at most 
// maxlen, and terminated by a NULL character.  It 
// packages up the bytes and returns them in the memory 
// referenced by "buffer".
//
// It assumes that there is space for maxlen bytes in 
// the memory referenced at buffer.
//
ssize_t recv_string(int socket, char *buffer, size_t maxlen) {
  ssize_t rc;
  char c;
  int pos;

  for (pos = 0; pos <= maxlen; pos++) {
    if ((rc = read(socket, &c, 1))==1) {
      buffer[pos] = c;
      if (c==0) break;
    } else if (rc==0) {
      break;
    } else {
      if (errno != EINTR ) continue;
      return -1;
    }
  }
  buffer[pos] = 0;
  return pos;
}


//
// send_string
//
// Sends a series of messages over a socket in order
// to transmit the characters referenced by "buffer".
// The last character it sends is the NULL character
// terminating that string.
//
ssize_t send_string(int socket, char *buffer) {
  ssize_t     nwritten;
  int pos = 0;
  int len = strlen(buffer)+1;
  while (pos < len) {
    if ((nwritten = write(socket, &(buffer[pos]), len-pos)) <= 0) {
      if (errno == EINTR) {
	nwritten = 0;
      } else {
	return -1;
      }
    }
    pos += nwritten;
  }
  return pos;
}

int build_address(char *hostname, int port, struct sockaddr_in *inet_address) {
  struct hostent *host_entries;
  host_entries = gethostbyname(hostname);
  if (host_entries == NULL || host_entries->h_addrtype != AF_INET || host_entries->h_length != 4) {
    return BULLETIN_HOSTNAME_LOOKUP_ERROR;
  } else {
    // success!
    inet_address->sin_family = AF_INET;
    inet_address->sin_addr.s_addr = *((uint32_t *)host_entries->h_addr_list[0]);
    inet_address->sin_port = port;
    return BULLETIN_OK;
  }
}

int bulletin_set_up_listener(int port, int *listener) {
  int listen_socket;
  char hostname[128];
  struct sockaddr_in address;
  int lookup_result, bind_result;

  // get a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL); 
  if (listen_socket == -1) return BULLETIN_CONNECT_ERROR;

  if (gethostname(hostname,sizeof(hostname)) < 0) return BULLETIN_CONNECT_ERROR;
  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;
  
  // bind it to a port on this machine
  bind_result = bind(listen_socket,(struct sockaddr *)&address,sizeof(address)); 
  if (bind_result == EADDRINUSE) return BULLETIN_PORT_IN_USE_ERROR;
  if (bind_result < 0) return BULLETIN_CONNECT_ERROR;

  // return the listener's socket descriptor
  *listener = listen_socket;
  return BULLETIN_OK;
}


int bulletin_wait_for_connection(int listen_socket, int *connection) {
  int bulletin_socket;

  // listen for a client's "connect" request
  if (listen(listen_socket, 1) < 0) return BULLETIN_CONNECT_ERROR;

  // accept it, get a dedicated socket for connection with this client
  bulletin_socket = accept(listen_socket,NULL,NULL);
  if (bulletin_socket < 0) return BULLETIN_CONNECT_ERROR;

  // return that client connection's socket descriptor
  *connection = bulletin_socket;
  return BULLETIN_OK;
}
  
int bulletin_make_connection_with(char *hostname, int port, int *connection) {
  int bulletin_socket;
  struct sockaddr_in address;
  int lookup_result, connect_result;

  // grab a new socket to connect with the server
  bulletin_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL);
  if (bulletin_socket == -1) return BULLETIN_CONNECT_ERROR;

  lookup_result = build_address(hostname,port,&address);
  if (lookup_result < 0) return lookup_result;

  // connect with that server
  connect_result = connect(bulletin_socket,(struct sockaddr *)&address,sizeof(address));
  if (connect_result < 0) return BULLETIN_CONNECT_ERROR;

  // return the connection's socket descriptor
  *connection = bulletin_socket;
  return BULLETIN_OK;
}

void bulletin_exit(int errcode) {
  fprintf(stderr,"Exiting with error code %d (%d).\n",errcode, errno);
  exit(errcode);
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
    problem("Error in communication.  receiver was not ready.\n");
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
