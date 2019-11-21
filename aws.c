/* This is AWS
   Process:
    AWS is up and running.
    AWS has received map ID <map ID>, start vertex <vertex ID> and file size <file size> from the client using TCP over port <24998>.
    AWS has sent map ID and starting vertex to server A using UDP over port <23998>.
    AWS has received shortest path from server A: Destination, Min Length; ...
    AWS has sent path length, propagation speed and transmission speed to server B using UDP over port <23998>.
    AWS has received delays from server B: Destination, Tt, Tp, Delay; ...
    AWS has sent calculated delay to client using TCP over port <24998>. */

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
#define AWS_UDP_PORT "23998"
#define ServerA_UDP_PORT "21998"
#define ServerB_UDP_PORT "22998"
#define HOST "localhost"
#define BACKLOG 10 // 10 connections allowed on the incoming queue.

static double result[226]; // assume array's max size is from a complete graph with 10 nodes 226 = 1+45*5

// waitpid() might overwrite errno, so we save and restore it: (from Beej 6.1)
void sigchld_handler(int s) {
  int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}


// UDP send to SeverA & ServerB, AWS play as client, only socket.
int UDP_to_Sever(char Map_ID[], int Source_Vertex_Index, int File_Size){

/*************************** UDP (from Beej 6.3) *****************************/

/***************************** UDP to A starts *******************************/
  /*---- Reference: Beej's Guide to Network Programming, chp 6.3, page29 ----*/

	int sockfd;
  int status;
	struct addrinfo hints, *servinfo, *p;

  /* ----------------------------- getaddrinfo ----------------------------- */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets.

	if ((status = getaddrinfo(HOST, ServerA_UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
	}
  // servinfo now points to a linked list of one or more struct addrinfos

  // -------- socket --------
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {

    // sockfd is the socket file descriptor returned by socket().
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("AWS_UDP: socket");
			continue;
    }
    break;
	}

	if (p == NULL) {
		fprintf(stderr, "AWS_UDP: failed to create socket\n");
    return 2;
	}

  /* -------------------- getaddrinfo eventually all done ------------------ */

  // AWS send Map_ID & Source_Vertex_Index to SeverA
  sendto(sockfd, Map_ID, sizeof ((char *)&Map_ID), 0, p->ai_addr, p->ai_addrlen);
  sendto(sockfd, (char *)&Source_Vertex_Index, sizeof Source_Vertex_Index, 0, p->ai_addr, p->ai_addrlen);

	freeaddrinfo(servinfo); // free the linkd-list

  printf("The AWS has sent map ID and starting vertex to server A using UDP over port %s.\n", AWS_UDP_PORT);

  int size;
  recvfrom(sockfd, (char *)& size, sizeof (size), 0, NULL, NULL);

  int path[2*size];
  recvfrom(sockfd, (char *)& path, sizeof (path), 0, NULL, NULL);

  double SpeedP, SpeedT;
  recvfrom(sockfd, (char *)& SpeedP, sizeof (SpeedP), 0, NULL, NULL);
  recvfrom(sockfd, (char *)& SpeedT, sizeof (SpeedT), 0, NULL, NULL);

  printf("The AWS has received shortest path from server A:\n");
  printf("-----------------------------------------------------------------\n");
  printf("Destination    Min Length\n");
  printf("-----------------------------------------------------------------\n");
  int t;
  for (t=0;t<size;t++){
    if (path[t+size]!=0)printf("%-15d%d\n", path[t], path[t+size]);
  }
  printf("-----------------------------------------------------------------\n");

  close(sockfd);
/******************************** UDP to A End *******************************/

/****************************** UDP to B Starts ******************************/
  /*---- Reference: Beej's Guide to Network Programming, chp 6.3, page29 ----*/

  if ((status = getaddrinfo(HOST, ServerB_UDP_PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 1;
  }
  // servinfo now points to a linked list of one or more struct addrinfos

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {

    // sockfd is the socket file descriptor returned by socket().
    // -------- socket --------
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("AWS_UDP: socket");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "AWS_UDP: failed to create socket\n");
    return 2;
  }

  // never lose the order
  sendto(sockfd, (char *)& File_Size, sizeof (File_Size), 0, p->ai_addr, p->ai_addrlen);
  sendto(sockfd, (char *)& size, sizeof (size), 0, p->ai_addr, p->ai_addrlen);
  sendto(sockfd, (char *)& path, sizeof (path), 0, p->ai_addr, p->ai_addrlen);
  sendto(sockfd, (char *)& SpeedP, sizeof (SpeedP), 0, p->ai_addr, p->ai_addrlen);
  sendto(sockfd, (char *)& SpeedT, sizeof (SpeedT), 0, p->ai_addr, p->ai_addrlen);

  freeaddrinfo(servinfo); // free the linkd-list

  printf("The AWS has sent path length, propagation speed and transmission speed to server B using UDP over port %s.\n", AWS_UDP_PORT);

  double delay[size];
  double Tp[size];
  double Tt[size];
  recvfrom(sockfd, (char *)& delay, sizeof (delay), 0, NULL, NULL);
  recvfrom(sockfd, (char *)& Tp, sizeof (Tp), 0, NULL, NULL);
  recvfrom(sockfd, (char *)& Tt, sizeof (Tt), 0, NULL, NULL);

  printf("The AWS has received delays from server B:\n");
  printf("-----------------------------------------------------------------\n");
  printf("Destination    Tt          Tp          Delay\n");
  printf("-----------------------------------------------------------------\n");
  for (t=0;t<size;t++){
    if (path[t+size]!=0)printf("%-15d%-12.2f%-12.2f%.2f\n", path[t], Tt[t], Tp[t], delay[t]);
  }
  printf("-----------------------------------------------------------------\n");

  result[0] = size; // size, # of nodes
  for (t=0;t<size;t++){
    result[t+1] = path[t]; // Destination
    result[t+size+1] = path[t+size]; // Min length
    result[t+2*size+1] = Tt[t];
    result[t+3*size+1] = Tp[t];
    result[t+4*size+1] = delay[t];
  }

  close(sockfd);
  /****************************** UDP to B Ends ******************************/

	return 0;
}

/*****************************************************************************/

int main(void) {

/*************************** TCP (from Beej 6.1) *****************************/
  /*- Reference: Beej's Guide to Network Programming, chp 6.1 & 6.2, page25 -*/

  int sockfd, new_fd;  // socket file descriptor returned by socket(). listen on sock_fd, new connection on new_fd.
  int status;
  int yes = 1;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information.
	struct sigaction sa;
  socklen_t sin_size;

  /* ----------------------------- getaddrinfo ----------------------------- */

  memset(&hints, 0, sizeof hints); // make sure the struct is empty.
	hints.ai_family = AF_UNSPEC; // not care IPv4 or IPv6.
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets.
	hints.ai_flags = AI_PASSIVE; // assign the address of my localhost to socket structures

	if ((status = getaddrinfo(NULL, AWS_TCP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
  }
  // servinfo now points to a linked list of one or more struct addrinfos

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {

    // -------- socket --------
    // sockfd is the socket file descriptor returned by socket().
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
    // solve "Address already in use"
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
			perror("setsockopt");
			exit(1);
		}

    // -------- bind --------
    // bind the socket to the host the program is running on.
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

  if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	freeaddrinfo(servinfo); // free the linkd-list

  /* -------------------- getaddrinfo eventually all done ------------------ */

	// -------- listen --------
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

  sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("The AWS is up and running.\n");

/* getaddrinfo(); socket(); bind(); listen() */
/* ------------------------------------------------------------------------- */
/* accept() */

  // accept() returns a brand new socket file descriptor to use for this single connection.
  while(1) {
    sin_size = sizeof their_addr; // connector's address information.

    // -------- accept --------
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    char Map_ID[1];
    int Source_Vertex_Index;
    int File_Size;

    if (!fork()) { // this is the child process.
      close(sockfd); // child doesn't need the listener.

      // receive from the client.
      recv(new_fd, Map_ID, sizeof Map_ID , 0);
      recv(new_fd, (char *)&Source_Vertex_Index, sizeof Source_Vertex_Index , 0);
      recv(new_fd, (char *)&File_Size, sizeof File_Size , 0);

      printf("The AWS has received map ID %s; start vertex %d and file size %d from the client using TCP over port %s.\n",
    	Map_ID, Source_Vertex_Index, File_Size, AWS_TCP_PORT);

      // send and receive from Sever
      UDP_to_Sever(Map_ID, Source_Vertex_Index, File_Size);

      // send to client
      send(new_fd, result, sizeof result, 0);

      printf("The AWS has sent calculated delay to client using TCP over port %s.\n\n", AWS_TCP_PORT);

      close(new_fd);
      exit(0);
    }

    close(new_fd);  // parent doesn't need this.
  }
/******************************** TCP END ************************************/
  return 0;
}
