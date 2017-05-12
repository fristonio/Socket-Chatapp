#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define BACKLOG 10			//defines the maximum length to which the queue of pending connections can grow
#define MAX_CLIENTS 10
#define PORT "5000"
#define BUFLEN 1024
#define STDIN 0

struct client {
	char handle[50];
	char clientId;	
};
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family ==AF_INET) {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc ,char *argv[]) {
	int sock_desc, listener;			//socket descriptor and listner descriptor
	struct sockaddr_storage in_address; //information about incoming connections
	socklen_t addr_size;				//contains the size of the incoming address
	struct addrinfo hints, *server_info, *result;	//addrinfo structure is used to store host address information
	memset(&hints, 0, sizeof hints);	//set hints to 0
	hints.ai_family = AF_UNSPEC;		//Use IPv4 or v6 whatever unspecified AF_INET is for ipv4 and AF_INET6 is for v6
	hints.ai_socktype = SOCK_STREAM;	//sock stream uses TCP protocol
	hints.ai_flags = AI_PASSIVE;		

	getaddrinfo(NULL, PORT, &hints, &server_info);	//will store the addrinfo of hints into server_info
	for(result = server_info; result!= NULL; result = result->ai_next) {
		sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock_desc < 0) {
			printf(" [?] Socket creation Failed ...\n");
			perror("Socket");
			continue;
		}
		printf("%d", sock_desc);
		int yes = 1;
		if (setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    		perror("setsockopt");
    		exit(1);
		} 
		if( bind(sock_desc, result->ai_addr, result->ai_addrlen) == -1 ) {
			printf(" [?] Server failed to bind address to the socket\n");
			perror("Bind");
			continue;
		}
		break;
	}
	freeaddrinfo(server_info);	//freees the address information that the getaddrinfo fnction dynamically allocates
	if(result == NULL) {
		fprintf(stderr, "Server failed to bind \n");
		exit(2);
	}
	if( listener = listen(sock_desc, BACKLOG) < 0 ) {
		printf(" [?] Server failed to Listen to the connections \n");
		perror("Listen");
		exit(3);
	}
	printf(" [*] Socket listening successfully listner %d\n", listener);
	printf(" [*] Server waiting for connections \n");

	int i,j;
	int new_conn;		//newly accepted socket descriptor
	char incoming_IP[INET6_ADDRSTRLEN]; //extra size => no problem
	char buf[BUFLEN];	//buffer for client data

	fd_set masterfd;	//master file descriptor
	fd_set readfds;		//temp file descriptor list for select()
	int fdmax;
	int msg_len;		//max file descriptor no
	FD_ZERO(&masterfd);	//clear the master and temp sets
	FD_ZERO(&readfds);	//clear read file descriptor
	FD_SET(listener, &masterfd);	//add listner to the master set since it is the only one till now
	FD_SET(STDIN, &masterfd);
	fdmax = listener;	//keep track of the biggest file descriptor till now
	while(1) {
		printf("inside the while loop with fdmax %d\n", fdmax);
		readfds = masterfd;
		if( select(fdmax+1, &readfds, NULL, NULL, NULL) == -1 ) {
			perror("select");
			exit(4);
		}
		//loop over all the child sockets for data to read
		printf("looping over the child sockets \n");
		for(i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &readfds)) {		//we got a connection available for reading
				if(i == listener) {
					// handle new connection
					addr_size = sizeof in_address;
					if( new_conn = accept(sock_desc, (struct sockaddr *)&in_address, &addr_size) < 0) {
						perror("Accept");
						continue;
					}
					inet_ntop(in_address.ss_family, get_in_addr((struct sockaddr *)&in_address), incoming_IP, INET6_ADDRSTRLEN);
					printf(" [*] Received request from the Client : %s on the Socket %d\n", incoming_IP, sock_desc);
				}
				else {
					//handle data from the client
					if(msg_len = recv(i, buf, BUFLEN, 0) <= 0) {
						if(msg_len == 0)
							printf("Socket %d hung up \n", i);
						else
							perror("recv:");
						close(i);
						FD_CLR(i, &masterfd);		//remove from the master set
					}
					else {
						//got some data from the client
						for(j = 0; j <= fdmax; j++) {
							//send the data to everyone
							if(FD_ISSET(j, &masterfd)) {
								//except the listner and the client itself
								if(j != listener && j != i) {
									if(send(j, buf, msg_len, 0) == -1) {
										perror("send");
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}