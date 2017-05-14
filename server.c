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
#define PORT "5000"			//Port to listen on
#define BUFLEN 1024
#define STDIN 0				//Standard Input


struct clientStruct {
	int clientId;
	int conn_desc;
	char nick[40];
	bool gotNick = FALSE;
};
clientStruct *clients["MAX_CLIENTS"] = {NULL};




void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family ==AF_INET) {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void createNewClient(int new_conn) {
	int i;
	for(i=0; i<MAX_CLIENTS; i++) {
		if(!clients[i]) {
			clients[i]->clientId = i;
			clients[i]->conn_desc = new_conn;
			return;
		}
	}
}

int getClientId(int connId) {
	int i;
	for(i=0; i<MAX_CLIENTS; i++) {
		if(clients[i]) {
			if(clients[i]->conn_desc == connId) {
				return i ;
			}
		}
	}
}

void sendPrivate(int desc, char *msg) {
	msg[strlen(msg)] = '\0';
	if(send(desc, msg, strlen(msg), 0) == -1)
		perror("Error : Send");
}

void getNick(int new_conn) {

	chat enterNick[] = "Enter a Nickname to enter the chat : ";
	sendPrivate(new_conn, enterNick);

}

int handleNewConnection(int sock_desc, fd_set *tempfd, int fdmax) {

	struct sockaddr_storage in_address; //information about incoming connections
	socklen_t addr_size;				//contains the size of the incoming address
	char incoming_IP[INET6_ADDRSTRLEN]; //extra size => no problem
	int new_conn;						//newly accepted socket descriptor

	addr_size = sizeof in_address;
	if(( new_conn = accept(sock_desc, (struct sockaddr *)&in_address, &addr_size )) < 0) {
		perror("Error : Accept");
	}
	// Add the new connection received to the master set
	FD_SET(new_conn, &tempfd);
	if(new_conn > fdmax)
		fdmax = new_conn;
	inet_ntop(in_address.ss_family, get_in_addr((struct sockaddr *)&in_address), incoming_IP, INET6_ADDRSTRLEN);
	printf(" [+] Received request from the Client : %s on the Socket %d\n", incoming_IP, fdmax);
	createNewClient(new_conn);
	getNick(new_conn);

	return fdmax;
}

void pollSocket(int sock_desc) {

	int i,j;
	char buf[BUFLEN];					//buffer for client data
	fd_set masterfd;					//master file descriptor
	fd_set readfds;						//temp file descriptor list for select()
	int fdmax;							//to store maximum file descriptor
	int msg_len;						//max file descriptor no
	FD_ZERO(&masterfd);					//clear the master and temp sets
	FD_ZERO(&readfds);					//clear read file descriptor
	FD_SET(sock_desc, &masterfd);		//add listner to the master set since it is the only one till now
	FD_SET(STDIN, &masterfd);			//Add standard input to the master set
	fdmax = sock_desc;					//keep track of the biggest file descriptor till now
	
	while(1) {
		readfds = masterfd;
		if( select(fdmax+1, &readfds, NULL, NULL, NULL) == -1 ) {
			perror("Error : Select");
			exit(4);
		}
		//Loop over all the descriptor in master set
		for(i = 0; i <= fdmax; i++) {

			if(FD_ISSET(i, &readfds)) {		//Got a descriptor available for reading
				
				if(i == sock_desc) {
					// New connection Received
					fdmax = handleNewConnection(sock_desc, &masterfd, fdmax);	
				}

				else {
					// Got an activity on one of the existing connections
					if(i == 0) {
						//Handle the data from standard input of server server
						if(fgets(buf, 1023, stdin) == NULL)
							return;
						else {
							char serverMsg[1024];
							sprintf(serverMsg, "[SERVER] : %s", buf);
							sendAll(i, &masterfd, fdmax, sock_desc, serverMsg);
						}
					}
					else {
						//handle data from the client
						int clientId = getClientId(i);
						if((msg_len = recv(i, buf, 1024, 0)) <= 0) {
							//got an error or connection closed by the client
							if(msg_len == 0) {
								//Connection closed by client
								printf("%s left the chat. \n", clients[clientId]->name);
								sprintf(buf, "%s left the chat. \n", clients[clientId]->name);
								sendAll(i, &masterfd, fdmax, sock_desc, buf);
							}
							else {
								perror("Error : recv:");
							}
							close(i);
							FD_CLR(i, &masterfd);		//remove from the master set
							clients[clientId] = NULL;
						}
						else {
							// A messege recieved from the client
							int clientId = getClientId(i);
							buf[strlen(buf)] = '\0';
							if(!clients[clientId]->gotNick) {
								// New client entered his name in the chat
								clients[clientId]->gotNick = TRUE;
								char welcomeMsg[BUFLEN];
								sprintf(welcomeMsg, "Welcome || @%s", clients[clientId]->nick);
								sendPrivate(i, welcomeMsg);
								sprintf(welcomeMsg, "@%s joined the chat ", clients[clientId]->nick);
								sendAll(i, &masterfd, fdmax, sock_desc, welcomeMsg);
							}
							else {
								// A new messege is recieved from a user in chat
								sendAll(i, &masterfd, fdmax, sock_desc, buf);
							}
						}
					}
				}
			}
		}
	}
}


/*===============================
          MAIN FUNCTION
===============================*/

int main(int argc, char *argv[]) {
	int sock_desc;						//socket descriptor and listner descriptor
	struct addrinfo hints, *server_info, *result;	//addrinfo structure is used to store host address information
	

	memset(&hints, 0, sizeof hints);	//set hints to 0
	hints.ai_family = AF_UNSPEC;		//Use IPv4 or v6 whatever unspecified AF_INET is for ipv4 and AF_INET6 is for v6
	hints.ai_socktype = SOCK_STREAM;	//sock stream uses TCP protocol
	hints.ai_flags = AI_PASSIVE;		//use my IP		
	int yes = 1;						//flag for setsockopt

	getaddrinfo(NULL, PORT, &hints, &server_info);	//will store the addrinfo of hints into server_info
	
	for(result = server_info; result!= NULL; result = result->ai_next) {
		sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock_desc < 0) {
			printf(" [?] Socket creation Failed ...\n");
			perror("Error : Socket");
			continue;
		}
		printf(" [*] Scoket descriptor  :  %d\n", sock_desc);
		if (setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    		perror("Error : setsockopt");
    		exit(1);
		} 
		if( bind(sock_desc, result->ai_addr, result->ai_addrlen) == -1 ) {
			printf(" [?] Server failed to bind address to the socket\n");
			perror("Error : Bind");
			continue;
		}
		break;
	}

	freeaddrinfo(server_info);	//freees the address information that the getaddrinfo fnction dynamically allocates
	if(result == NULL) {
		fprintf(stderr, "Server failed to bind \n");
		exit(2);
	}

	if( listen(sock_desc, BACKLOG) < 0 ) {
		printf(" [?] Server failed to Listen to the connections \n");
		perror("Error : Listen");
		exit(3);
	}

	printf(" [*] Socket listening successfully listner \n");
	printf(" [*] Server waiting for connections \n");

	pollSocket(sock_desc);
	
	return 0;

}