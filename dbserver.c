/*
	By: Sadia Khan
	Server side
	
*/


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common_threads.h"
#include <fcntl.h>     // for open, O_RDONLY
#include <inttypes.h>  // for SCNd32, PRId32
#include "msg.h"


struct parameters{
	int cfd;
};


void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
int  Listen(char *portnum, int *sock_family);
void *HandleClient(void* c_fd);
				  

int 
main(int argc, char **argv) {
  // Expect the port number as a command line argument.
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);
  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }
  
  

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
	  
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_fd,
                           (struct sockaddr *)(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      break;
    }
	
	
    pthread_t p1;
	

	struct parameters *par;
	par = (struct parameters *)malloc(sizeof(struct parameters));
	par->cfd = client_fd;
	
    Pthread_create(&p1, NULL, HandleClient, (void *)par);
	pthread_detach(p1);
	

  }

  // Close socket
  close(listen_fd);
  return EXIT_SUCCESS;
}

void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
	printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
		//PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}

void *
HandleClient(void * arg){
    struct parameters* c_fd = (struct parameters*) arg;
	
    // open input file
    int32_t filefd;
    filefd = open("database.txt", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (filefd == -1) {
      perror("open failed");
      exit(EXIT_FAILURE);
    }
	
  // Print out information about the client.
  printf("\nNew client connection \n" );
  
  // Loop, reading data and echo'ing it back, until the client
  // closes the connection.
  while (1) {
    struct msg clientbuf;
    ssize_t res = read(c_fd->cfd, &clientbuf, sizeof(clientbuf));
    if (res == 0) {
      printf("[The client disconnected.] \n");
      break;
    }

    if (res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR))
        continue;

      printf(" Error on client socket:%s \n ", strerror(errno));
      break;
    }
	
	//create reply struct
	struct msg reply;
	
	//put
	if(clientbuf.type==1){
		
		//get to where we want to put record
		lseek(filefd, sizeof(clientbuf.rd), SEEK_END);
		
		//check if write is success or fail and save that in record and write back to client
		if(write(filefd, &clientbuf.rd, sizeof(clientbuf.rd))==-1){
			reply.type=5;
			write(c_fd->cfd, &reply, sizeof(reply));
		}
		else{
			reply.type=4;
			write(c_fd->cfd, &reply, sizeof(reply));
		}
	}
	
	//get
	if(clientbuf.type==2){
		
		 uint32_t index= clientbuf.rd.id;
		 for(int i=0; i<(lseek(filefd, 0, SEEK_END)); i=i+sizeof(clientbuf.rd)){
			
			 //file is currently at end so bring back to start
			lseek(filefd, i, SEEK_SET);
			
 		    //get record from file
 			read(filefd, &reply.rd, sizeof(reply.rd));
			
			if(reply.rd.id == index){
				reply.type=4;
				write(c_fd->cfd, &reply, sizeof(reply));
				break;
			}
			
			reply.type=5;
			 
		 }
		 
		 if(reply.type == 5){
	 		 //didint find record so write back fail
	 		write(c_fd->cfd, &reply, sizeof(reply));
		 	
		 }
		
		
	}
	

  }
  //close the sockets!
  close(c_fd->cfd);
  close(filefd);
  return NULL;
}
