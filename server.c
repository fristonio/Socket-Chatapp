#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>

#define BACKLOG 10
#define MAX_CLIENTS 10

struct client {
	char handle[50];
	char clientId;	
}

int main(int argc ,char *argv[]) {
	//Socket descriptor
	int sock_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_desc < 0) {
		printf(" [?] Socket creation Failed ...\n");
		exit(0);
	}
	printf(" [*] Socket created successfully \n");

	//Structure to store the details
	struct sockaddr_in server;
	memset(&server, '0', sizeof(server));
	//Initialize the strucute
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(5000);
	server.sin_family = AF_INET;
	int addrlen = sizeof(server);
	if( bind(sock_desc, (struct sockaddr*)&server, sizeof(server)) == -1) {
		printf(" [?] Cannot bind address to the socket\n");
		exit(0);
	}

	if( listen(sock_desc, BACKLOG) < 0 ) {
		printf(" [?] Cannot Listen to the connections \n");
		exit(0);
	}

	int new_socket;
	if( new_socket = accept(sock_desc, (struct sockaddr *)&server, (socklen_t*)&addrlen) < 0) {
		printf(" [?] Cannot accept connections \n");
		exit(0);
	}

	return 0;
}