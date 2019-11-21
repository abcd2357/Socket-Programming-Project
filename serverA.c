/* This is Server A
   Process:
    Server A is up and running using UDP on port <21998>.
    Server A has constructed a list of <number> maps: Map ID, Num Vertices, Num Edges; ...
    Server A has received input for finding shortest paths: starting vertex <index> of map <ID>.
    Server A has identified the following shortest paths: Destination, Min Length; ...
    Server A has sent shortest paths to AWS. */

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

#define ServerA_UDP_PORT "21998"
#define HOST "localhost"
#define INF 10000


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

	if ((status = getaddrinfo(HOST, ServerA_UDP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
  // servinfo now points to a linked list of one or more struct addrinfos

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {

    // -------- socket --------
    // sockfd is the socket file descriptor returned by socket().
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("serverA: socket");
			continue;
		}

    // -------- bind --------
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("serverA: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "serverA: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo); // free the linkd-list

  /* -------------------- getaddrinfo eventually all done ------------------ */

	printf("The Server A is up and running using UDP on port %s.\n", ServerA_UDP_PORT);

 /******************************** MAP CONS **********************************/

  /* ----------------------------- file reading ---------------------------- */

	FILE *map;
  if ((map = fopen("map.txt", "r")) == NULL) {
		printf("Couldn't open map.txt for reading\n");
		exit(1);
	} // Reference: Beej's Guide to C Programming, chp13.1, page62.

	char map_id[1]; // not change this or countless bugs
	int source_node, dest_node, node_dist;
	double prop, trans;

  // define triple e.g. i=0 j=1 dist=10
	typedef struct{
		int i, j, dist;
	}triple;

  // define map struct
  struct map_struct{
		char map_name;
		triple data[45];
		double P_speed, T_speed;
		int num_edges;
		int num_vertices;
		int vertex[15]; // the vertex set, not change this though 15 > 10
	};

	struct map_struct mapstr[10]; // *** WARNING! UP TO 10 MAPS in map.txt ***
	int N = 0;

	while(!feof(map)){
		fscanf(map, "%s", map_id);
		mapstr[N].map_name = *map_id;
		fscanf(map, "%lf", &prop);
		mapstr[N].P_speed = prop;
		fscanf(map, "%lf", &trans);
		mapstr[N].T_speed = trans;
		int k = 0;
		while(fscanf(map,"%d %d %d", &source_node, &dest_node, &node_dist) == 3){
			mapstr[N].data[k].i = source_node;
			mapstr[N].data[k].j = dest_node;
			mapstr[N].data[k].dist = node_dist;
			k++;
		};
		mapstr[N].num_edges = k;

    // use array a[] to count different nodes in the map.
		// how many nodes exist ----------
		int q,m,t=0;
		int a[90]; // # of edges are between 1 to 45

		for(q=0;q<k;q++){
			a[q] = mapstr[N].data[q].i;
			a[q+k] = mapstr[N].data[q].j;
		};

		for (q=0;q<2*k;q++){
			for (m=q+1;m<2*k;m++){
				if ((a[q] == a[m])&&(a[q] != -1)){
					a[m] = -1;};
			};
		};

    // make sure the ID in vertex set is from small to big
    int disorder[15];
		for (q=0;q<2*k;q++){
			if (a[q] != -1){
				disorder[t] = a[q];
				t++;};
		};
		mapstr[N].num_vertices = t;

		int icount = 0, label;
		int max = 40000; // *** WARNING! THE NODE ID IS UP TO 40000 ***
		while (icount < mapstr[N].num_vertices){
			for (t=0;t<mapstr[N].num_vertices;t++){
				if (max > disorder[t]){
					max = disorder[t];
					label = t;
				}
			}
			mapstr[N].vertex[icount] = max;
			max = 40000;
			disorder[label] = max;
			icount++;
		}

		// this is how many nodes exist ----------
    N++;
	};

	fclose(map);
	/* ---------------------------- file is read ----------------------------- */

  printf("The Server A has constructed a list of %d maps:\n", N);
	printf("-----------------------------------------------------------------\n");
	printf("MAP ID    Num Vertices    Num Edges\n");
	printf("-----------------------------------------------------------------\n");
	int h=0;
	while(h<N){
		printf("%-10s%-16d%d\n", (char*)&mapstr[h].map_name, mapstr[h].num_vertices, mapstr[h].num_edges);
		h++;
	};
	printf("-----------------------------------------------------------------\n");

 /******************************* MAP is CONed *******************************/

 /************************** Dijkstra define Function ************************/
	/*------------- Partial Referenced From cnblogs /skywang12345 -------------*/
	/*---------- https://www.cnblogs.com/skywang12345/p/3711512.html ----------*/

	int Dijkstra(struct map_struct* G, int A){
		int t,x,y,n,sv,p,min,tmp;
		int prev[G->num_vertices]; // previous node(aka vertex)
		int flag[G->num_vertices]; // determine if a node is in SPT
		int dist[G->num_vertices][G->num_vertices]; // dist between nodes

	  /*
  	mapstr[c].data[k].i // source_node
	  mapstr[c].data[k].j // dest_node
  	mapstr[c].data[k].dist // node_dist
	  mapstr[c].num_edges
	  mapstr[c].num_vertices
	  mapstr[c].vertex[] // vertex set
	  */

    // make sure dist(x,y) = dist(y,x), use subscript to replace node_id
		for (x=0;x<G->num_vertices;x++){
			for (y=0;y<G->num_vertices;y++){
				dist[x][y] = INF;
				if (x == y){dist[x][y] = 0;};
				for (t=0;t<G->num_edges;t++){
					if ((G->vertex[x] == G->data[t].i) && (G->vertex[y] == G->data[t].j)){dist[x][y] = G->data[t].dist;};
					if ((G->vertex[y] == G->data[t].i) && (G->vertex[x] == G->data[t].j)){dist[x][y] = G->data[t].dist;};
				}
			}
		}

	  // obtain source vertex
	  for (t=0;t<G->num_vertices;t++){if (G->vertex[t] == A)sv = t;}

	  for (t=0;t<G->num_vertices;t++){
	   	prev[t] = 0; // at first not know previous node
	  	flag[t] = 0; // a flag to determine if a node is in SPT
	  }
	  flag[sv] = 1; // Source Vertex is in SPT

    // find the next nearest node and update the Dist-Prev table
	  for(t=0;t<G->num_vertices;t++){
	  	min = INF;

	  	for (n=0;n<G->num_vertices;n++){
	  		if ((flag[n]==0)&&(dist[sv][n]<min)){
			  	min = dist[sv][n];
			  	p = n;
			  }
		  }
		  flag[p] = 1;

	  	for (n=0;n<G->num_vertices;n++){
		  	tmp = (dist[p][n] == INF ? INF:(min + dist[p][n]));
	  		if ((flag[n]==0)&&(tmp<dist[sv][n])){
	  			dist[sv][n] = tmp; // update the Dist-Prev table
	  			prev[n] = p;
	  		}
	  	}
	  }

  	printf("The Server A has identified the following shortest paths:\n");
		printf("-----------------------------------------------------------------\n");
		printf("Destination    Min Length\n");
		printf("-----------------------------------------------------------------\n");
  	for (t=0;t<G->num_vertices;t++){
  		if (t!=sv)printf("%-15d%d\n", G->vertex[t], dist[sv][t]);
  	}
		printf("-----------------------------------------------------------------\n");

    /*--------------------------- UDP Send to AWS ---------------------------*/
		/*--- Reference: Beej's Guide to Network Programming, chp 6.3, page29 ---*/

    int size;
    size = G->num_vertices;
    int path[2*size];

		for (t=0;t<size;t++){
				path[t] = G->vertex[t];
				path[t+size] = dist[sv][t];
  	}

		double SpeedP, SpeedT;
		SpeedP = G->P_speed;
		SpeedT = G->T_speed;

    sendto(sockfd, (char *)& size, sizeof (size), 0, (struct sockaddr *)&their_addr, addr_len);
		sendto(sockfd, (char *)& path, sizeof (path), 0, (struct sockaddr *)&their_addr, addr_len);
		sendto(sockfd, (char *)& SpeedP, sizeof (SpeedP), 0, (struct sockaddr *)&their_addr, addr_len);
		sendto(sockfd, (char *)& SpeedT, sizeof (SpeedT), 0, (struct sockaddr *)&their_addr, addr_len);
	 	printf("The Server A has sent shortest paths to AWS.\n\n");

		/*--------------------------- UDP Sent to AWS ---------------------------*/

		return 0;
  }
 /************************ Dijkstra define is ended **************************/

	while(1) {
		addr_len = sizeof their_addr;
		char Map_ID[1];
	  int Source_Vertex_Index;

		recvfrom(sockfd, Map_ID, sizeof Map_ID, 0, (struct sockaddr *)&their_addr, &addr_len);
		recvfrom(sockfd, (char *)&Source_Vertex_Index, sizeof Source_Vertex_Index, 0, (struct sockaddr *)&their_addr, &addr_len);

	  printf("The Server A has received input for finding shortest paths: starting vertex %d of map %s.\n",
		Source_Vertex_Index, Map_ID);

		/*------------------------- Dijkstra perform ----------------------------*/
		// get the struct id
		int c = 0;
		while(c<N){
			if(*Map_ID == mapstr[c].map_name)break;
			c++;
		};

		Dijkstra(&mapstr[c], Source_Vertex_Index);
		continue;
		/*--------------------------- Dijkstra end ------------------------------*/
	}

  close(sockfd);
/******************************** UDP END ************************************/

	return 0;
}
