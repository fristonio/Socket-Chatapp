#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define BACKLOG 10
#define MAX_CLIENTS 10
#define PORT "5000"

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
	int sock_desc;
	struct sockaddr_storage in_address; //information about incoming connections
	socklen_t addr_size;
	struct addrinfo hints, *server_info, *result;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;//Use IPv4 or v6 whatever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &server_info);
	for(result = server_info; result!= NULL; result = result->ai_next) {
		sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock_desc < 0) {
			printf(" [?] Socket creation Failed ...\n");
			perror("Socket");
			continue;
		}
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
	freeaddrinfo(server_info);
	if(result == NULL) {
		fprintf(stderr, "Server failed to bind \n");
		exit(1);
	}
	if( listen(sock_desc, BACKLOG) < 0 ) {
		printf(" [?] Server failed to Listen to the connections \n");
		perror("Listen");
		exit(1);
	}
	printf(" [*] Socket listening successfully \n");
	printf(" [*] Server waiting for connections \n");
	int new_conn;
	char incoming_IP[INET6_ADDRSTRLEN]; //extra size => no problem
	while(1) {
		addr_size = sizeof in_address;
		if( new_conn = accept(sock_desc, (struct sockaddr *)&in_address, &addr_size) < 0) {
			perror("Accept");
			continue;
		}
		inet_ntop(in_address.ss_family, get_in_addr((struct sockaddr *)&in_address), incoming_IP, INET6_ADDRSTRLEN);
		printf(" [*] Received request from the Client : %s on the Socket %d\n", incoming_IP, sock_desc);
	}
	return 0;
}