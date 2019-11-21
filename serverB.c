/* This is Server B
   Process:
    Server B is up and running using UDP on port <22998>.
    Server B has received data for calculation: Propagation speed, Transmission speed, Path length for destination <vertex index>: <Min Length>; ...
    Server B has finished the calculation of the delays: Destination, Delay; ...
    Server B has finished sending the output to AWS. */

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

#define ServerB_UDP_PORT "22998"
#define HOST "localhost"


int main(void){

/*************************** UDP (from Beej 6.3) *****************************/
  /*---- Reference: Beej's Guide to Network Programming, chp 6.3, page29 ----*/

	int sockfd;
  int status;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t addr_len;

  /* ----------------------------- getaddrinfo ----------------------------- */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets.

	if ((status = getaddrinfo(HOST, ServerB_UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
  // servinfo now points to a linked list of one or more struct addrinfos

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {

    // -------- socket --------
    // sockfd is the socket file descriptor returned by socket().
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("serverB: socket");
			continue;
		}

    // -------- bind --------
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("serverB: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "serverB: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo); // free the linkd-list

  /* -------------------- getaddrinfo eventually all done ------------------ */

	printf("The Server B is up and running using UDP on port %s.\n", ServerB_UDP_PORT);

/********************* UDP Receive From AWS (Beej 6.3) **********************/

  while(1) {
	  addr_len = sizeof their_addr;
		int size, i;
		int path[20]; // less than 10 nodes
		int File_Size;
		double SpeedP, SpeedT;

    recvfrom(sockfd, (char *)& File_Size, sizeof (File_Size), 0, (struct sockaddr *)&their_addr, &addr_len);
	  recvfrom(sockfd, (char *)& size, sizeof (size), 0, (struct sockaddr *)&their_addr, &addr_len);
	  recvfrom(sockfd, (char *)& path, sizeof (path), 0, (struct sockaddr *)&their_addr, &addr_len);
	  recvfrom(sockfd, (char *)& SpeedP, sizeof (SpeedP), 0, (struct sockaddr *)&their_addr, &addr_len);
	  recvfrom(sockfd, (char *)& SpeedT, sizeof (SpeedT), 0, (struct sockaddr *)&their_addr, &addr_len);

	  printf("The Server B has received data for calculation:\n");

		printf("* Propagation speed: %.2f km/s;\n",  SpeedP);
		printf("* Transmission speed: %.2f Bytes/s;\n",  SpeedT);
		for (i=0;i<size;i++){
			if (path[i+size]!=0){
				printf("* Path length for destination %d: %d;\n", path[i], path[i+size]);
			}
		}

    // calculate the delays
    double delay[size];
		double Tp[size];
		double Tt[size];
		for (i=0;i<size;i++){
			if (path[i+size]!=0){
				Tp[i] = path[i+size] / SpeedP;
				Tt[i] = File_Size / (8*SpeedT);
				delay[i] = Tp[i] + Tt[i];
			}
		}

		printf("The Server B has finished the calculation of the delays:\n");
		printf("-----------------------------------------------------------------\n");
		printf("Destination    Delay\n");
		printf("-----------------------------------------------------------------\n");
		for (i=0;i<size;i++){
			if (path[i+size]!=0){
				printf("%-15d%.2f\n", path[i], delay[i]);
			}
		}
		printf("-----------------------------------------------------------------\n");

		/*-------------------------- UDP Send to AWS ----------------------------*/

    sendto(sockfd, (char *)& delay, sizeof (delay), 0, (struct sockaddr *)&their_addr, addr_len);
		sendto(sockfd, (char *)& Tp, sizeof (Tp), 0, (struct sockaddr *)&their_addr, addr_len);
		sendto(sockfd, (char *)& Tt, sizeof (Tt), 0, (struct sockaddr *)&their_addr, addr_len);
	 	printf("The Server B has finished sending the output to AWS.\n\n");

		/*-------------------------- UDP Sent to AWS ----------------------------*/
	}

  close(sockfd);
	return 0;
}
