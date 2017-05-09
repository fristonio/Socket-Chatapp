#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>

#define BACKLOG 10
#define MAX_CLIENTS 10
#define PORT "5000"

struct client {
	char handle[50];
	char clientId;	
};

int main(int argc ,char *argv[]) {
	int sock_desc;
	struct sockaddr_storage address;
	socklen_t addr_size;
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;//Use IPv4 or v6 whatever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &result);

	sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock_desc < 0) {
		printf(" [?] Socket creation Failed ...\n");
	}
	printf(" [*] Socket created successfully \n");
	if( bind(sock_desc, result->ai_addr, result->ai_addrlen) == -1 ) {
		printf(" [?] Cannot bind address to the socket\n");
	}
	if( listen(sock_desc, BACKLOG) < 0 ) {
		printf(" [?] Cannot Listen to the connections \n");
	}
	int new_socket;
	if( new_socket = accept(sock_desc, (struct sockaddr *)&address, &addr_size) < 0) {
		printf(" [?] Cannot accept connections \n");
	}
	
	return 0;
}