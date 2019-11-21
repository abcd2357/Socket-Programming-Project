/* This is client
  Process：
    client is up and running.
    client has sent query to AWS using TCP over port <port number>: start vertex <vertex index>; map <map ID>; file size <file size>.
    client has received results from AWS: Destination, Min Length, Tt, Tp, Delay; ... */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define AWS_TCP_PORT "24998"
#define HOST "localhost"


int main(int argc, char *argv[]) {

/*************************** TCP (from Beej 6.1) *****************************/
  /*- Reference: Beej's Guide to Network Programming, chp 6.1 & 6.2, page25 -*/

  int sockfd;
	int status;
  int sin_size;
  int getsock_check;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in my_addr; // my address information.

  /* ----------------------------- getaddrinfo ----------------------------- */

  memset(&hints, 0, sizeof hints); // make sure the struct is empty.
  hints.ai_family = AF_UNSPEC; // not care IPv4 or IPv6.
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets.

  if ((status = getaddrinfo(HOST, AWS_TCP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
  // stderr: default file descriptor where process writes error messages.
  // servinfo now points to a linked list of one or more struct addrinfos.

  // loop through all the results and connect to the first we can.
  for(p = servinfo; p != NULL; p = p->ai_next) {

    // -------- socket --------
    // sockfd is the socket file descriptor returned by socket().
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    // -------- connect --------
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: connect");
      continue;
    }
    break;
  }

  if (p == NULL)  {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

	// Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure (from Man).
  sin_size = sizeof my_addr;
	if ((getsock_check = getsockname(sockfd, (struct sockaddr *)&my_addr, (socklen_t *)&sin_size)) == -1) {
		perror("getsockname");
		exit(1);
	}

	int CLIENT_DYNAMIC_PORT;
	CLIENT_DYNAMIC_PORT = my_addr.sin_port; // get client dynamic port (now not required to print)

  freeaddrinfo(servinfo); // free the linkd-list

  /* -------------------- getaddrinfo eventually all done ------------------ */

  printf("The client is up and running.\n");

  // send to AWS.
	char Map_ID[1]; // Here if use char MPA_ID, countless bugs.
  strcpy(Map_ID,argv[1]); // get Map_ID input.
  int Source_Vertex_Index;
  Source_Vertex_Index = atoi(argv[2]); // get Source Vertex Index.
  int File_Size;
  File_Size = atoi(argv[3]); // get File Size input.

  // send to AWS
  send(sockfd, Map_ID, sizeof Map_ID , 0);
  send(sockfd, (char *)&Source_Vertex_Index, sizeof Source_Vertex_Index , 0);
  send(sockfd, (char *)&File_Size, sizeof File_Size , 0);

  printf("The client has sent query to AWS using TCP over port %d: start vertex %d; map %s; file size %d.\n",
	CLIENT_DYNAMIC_PORT, Source_Vertex_Index, Map_ID, File_Size);

  // receive from AWS
  double result[226];
  recv(sockfd, result, sizeof result, 0);

  printf("The client has received results from AWS:\n");
  printf("-----------------------------------------------------------------\n");
  printf("Destination    Min Length       Tt          Tp          Delay\n");
  printf("-----------------------------------------------------------------\n");
  int size = result[0], t;
  for (t=0;t<size;t++){
    if (result[t+size+1]!=0)printf("%-15.0f%-17.0f%-12.2f%-12.2f%.2f\n", result[t+1],
    result[t+size+1], result[t+2*size+1], result[t+3*size+1], result[t+4*size+1]);
  }
  printf("-----------------------------------------------------------------\n\n");

/*****************************************************************************/
  close(sockfd);
  return 0;
}
