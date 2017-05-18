#include <stdio.h>
#include <string.h>
#include <ctype.h>
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


typedef enum {
	PRIVATE_MESSAGE,
	INFO_USERS,
	HELP,
	BROADCAST_MSG,
	COMMAND_ERR
} actions;

struct clientStruct {
	int clientId;
	int conn_desc;
	char nick[40];
	int gotNick;
};
struct clientStruct *clients[MAX_CLIENTS] = {NULL};


void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family ==AF_INET) {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void createNewClient(int new_conn) {
	struct clientStruct *temp = (struct clientStruct *)malloc(sizeof(struct clientStruct));
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(!clients[i]) {
			temp->clientId = i;
			temp->conn_desc = new_conn;
			temp->gotNick = 0;
			clients[i] = temp;
			return;
		}
	}
}

int getClientId(int connId) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i]) {
			if(clients[i]->conn_desc == connId) {
				return i ;
			}
		}
	}
}

int getClientConnDesc(char *client) {
	int i;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i]) {
			if(!strcmp(client, clients[i]->nick)) {
				return clients[i]->conn_desc;
			}
		}
	}
}

void getClientDetails(char *outMsg) {
	int i;
	int count = 1;
	char nick[50];
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i]) {
			memset(nick, '\0', sizeof(nick));
			sprintf(nick, "  [%d] %s \n",count, clients[i]->nick);
			strcat(outMsg, nick);
			count++;
		}
	}
}

void sendPrivate(int desc, char *msg) {
	//msg[strlen(msg)] = '\0';
	if(send(desc, msg, strlen(msg), 0) == -1) {
		perror("Error : Send");
	}
}

void sendAll(int conn, fd_set *tempfd, int fdmax, int sock_desc, char *msg) {
	int j;
	//msg[strlen(msg)] = '\0';
	for(j = 0; j <= fdmax; j++) {
		//Send messege to everyone rather other than sender and sock_desc
		if(FD_ISSET(j, tempfd)) {
			if(j != sock_desc && j != conn && j!=0) {
				if(send(j, msg, strlen(msg), 0) == -1) {
					perror("ERROR : Send");
				}
			}
		}
	}
}

void getNick(int new_conn) {
	printf("[+] Sent req for entering name to join chat \n");
	char enterNick[] = "Enter a Nickname to enter the chat : ";
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
	FD_SET(new_conn, tempfd);
	if(new_conn > fdmax)
		fdmax = new_conn;
	inet_ntop(in_address.ss_family, get_in_addr((struct sockaddr *)&in_address), incoming_IP, INET6_ADDRSTRLEN);
	printf("[+] Received request from the Client : %s on the Socket %d\n", incoming_IP, fdmax);
	createNewClient(new_conn);
	getNick(new_conn);

	return fdmax;
}

void handleServerInput(fd_set *tempfd, int fdmax, int sock_desc) {
	char buf[BUFLEN];
	memset(buf, '\0', sizeof(buf));
	if(fgets(buf, 1023, stdin) == NULL)
		return;
	else {
		char serverMsg[1024];
		sprintf(serverMsg, "[SERVER] : %s", buf);
		sendAll(0, tempfd, fdmax, sock_desc, serverMsg);
	}
}

void handleClientConnection(int sock_desc, int fdmax, int conn, fd_set *tempfd, int msg_len) {
	int clientId = getClientId(conn);
	char buf[BUFLEN];
	if(msg_len == 0) {
	//Connection closed by client
		printf("[%s] : left the chat. \n", clients[clientId]->nick);
		sprintf(buf, "[%s] : left the chat. \n", clients[clientId]->nick);
		sendAll(conn, tempfd, fdmax, sock_desc, buf);
	}
	else {
		perror("Error : recv:");
	}
}

void setNick(char *inMsg, int conn, int sock_desc, fd_set *tempfd, int fdmax) {
	int clientId = getClientId(conn);
	char welcomeMsg[BUFLEN];
	memset(welcomeMsg, '\0', sizeof(welcomeMsg));

	strncpy(clients[clientId]->nick, inMsg, sizeof(inMsg));
	sprintf(welcomeMsg, "Welcome || @%s to #chat : \n [SERVER] : Use !help for HELP anytime during chat \n", clients[clientId]->nick);
	sendPrivate(conn, welcomeMsg);

	printf("@%s joined the chat \n", clients[clientId]->nick);
	memset(welcomeMsg, '\0', sizeof(welcomeMsg));
	sprintf(welcomeMsg, "@%s joined the chat \n", clients[clientId]->nick);
	sendAll(conn, tempfd, fdmax, sock_desc, welcomeMsg);
}


void handleClientInput(int sock_desc, int fdmax, fd_set *tempfd, char *inMsg, int conn, actions action) {
	int clientId = getClientId(conn);
	char outMsg[BUFLEN];
	memset(outMsg, '\0', sizeof(outMsg));
	char chatUser[30];
	int i = 1;
	switch(action) {
		case PRIVATE_MESSAGE :
								while(!isspace(inMsg[i])) {
									chatUser[i-1] = inMsg[i];
									i++;
								}
								chatUser[i-1] = '\0';
								int userConn = getClientConnDesc(chatUser);
								if(userConn) { 
									printf("[%s] : [PRIVATE] : %s \n", clients[clientId]->nick, inMsg);
									sprintf(outMsg, "[%s] : [PRIVATE] : %s \n", clients[clientId]->nick, inMsg);
									sendPrivate(userConn, outMsg);
								}
								else {
									sprintf(outMsg, "The user you entered is not present !! ");
									sendPrivate(conn, outMsg);
								}
								break;

		case INFO_USERS :
								sprintf(outMsg, " ****** USERS INFO ****** \n");
								printf("[*] User asked for users info \n");
								getClientDetails(outMsg);
								sendPrivate(conn, outMsg);
								break;

		case COMMAND_ERR :	
								sprintf(outMsg, "Wrong Command!!! Try again \n");
								sendPrivate(conn, outMsg);

		case HELP :			
								sprintf(outMsg, " *** HELP ***\n [+] @[username] message : for private message\n [+] !users : for getting current users info\n [+] !help : For help \n");
								sendPrivate(conn, outMsg);
								break;

		case BROADCAST_MSG :	
								printf("[%s] : %s \n", clients[clientId]->nick, inMsg);
								sprintf(outMsg, "[%s] : %s \n", clients[clientId]->nick, inMsg);
								sendAll(conn, tempfd, fdmax, sock_desc, outMsg);
								break;
	}
}


void pollSocket(int sock_desc) {

	int i,j;
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
					char buf[BUFLEN];
					memset(buf, '\0', sizeof(buf));
					// Got an activity on one of the existing connections
					if(i == 0) {
						//Handle the data from standard input of server server
						handleServerInput(&masterfd, fdmax, sock_desc);
					}

					else {
						//handle data from the client
						int clientId = getClientId(i);
						if((msg_len = recv(i, buf, 1023, 0)) <= 0) {
							//got an error or connection closed by the client
							handleClientConnection(sock_desc, fdmax, i, &masterfd, msg_len);
							close(i);
							FD_CLR(i, &masterfd);		//remove from the master set
							clients[clientId] = NULL;
						}
						else {
							// A messege recieved from the client
							// netcat attaches a \n at the end of the message we replace it by null character
							buf[strlen(buf)-1] = '\0';
							char *inMsg;
							inMsg = malloc(strlen(buf)+1);
							memset(inMsg, '\0', sizeof(inMsg));
							strncpy(inMsg, buf, strlen(buf));

							if(!clients[clientId]->gotNick) {
								// New client entered his name in the chat
								clients[clientId]->gotNick = 1;
								setNick(inMsg, i, sock_desc, &masterfd, fdmax);	
							}
							else {
								// A new messege is recieved from a user in chat
								char fc = inMsg[0];
								int k = 0;
								char temp[30];
								memset(temp, '\0', sizeof(temp));
								actions action;
								switch(fc) {
									case '@' :  action = PRIVATE_MESSAGE;
												break;

									case '!' :	while(!isspace(inMsg[k])) {
													temp[k-1] = inMsg[k];
													k++;
												}
												temp[k-1] = '\0';
												if(!strcmp(temp, "users")) 
													action = INFO_USERS;
												else
													if(!strcmp(temp, "help"))
														action = HELP;
													else
														action = COMMAND_ERR;
												break;

									default : action = BROADCAST_MSG;
											  break;
								}
								handleClientInput(sock_desc, fdmax, &masterfd, inMsg, i, action);

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
	printf(" [*] Starting Server \n");
	char *port;
	if(argc != 2) {
		printf("[-] Usage %s [port] \n", argv[0]);
		printf("[*] Using default PORT 5000 for server \n");
		port = PORT;
	}
	else
		port = argv[1];

	int sock_desc;						//socket descriptor and listner descriptor
	struct addrinfo hints, *server_info, *result;	//addrinfo structure is used to store host address information
	

	memset(&hints, 0, sizeof hints);	//set hints to 0
	hints.ai_family = AF_UNSPEC;		//Use IPv4 or v6 whatever unspecified AF_INET is for ipv4 and AF_INET6 is for v6
	hints.ai_socktype = SOCK_STREAM;	//sock stream uses TCP protocol
	hints.ai_flags = AI_PASSIVE;		//use my IP		
	int yes = 1;						//flag for setsockopt

	getaddrinfo(NULL, port, &hints, &server_info);	//will store the addrinfo of hints into server_info
	
	for(result = server_info; result!= NULL; result = result->ai_next) {
		sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock_desc < 0) {
			printf(" [?] Socket creation Failed ...\n");
			perror("Error : Socket");
			continue;
		}
		if (setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    		perror("Error : setsockopt");
    		exit(1);
		} 
		if( bind(sock_desc, result->ai_addr, result->ai_addrlen) == -1 ) {
			printf("[?] Server failed to bind address to the socket\n");
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
		printf("[?] Server failed to Listen to the connections \n");
		perror("Error : Listen");
		exit(3);
	}

	printf("[*] Socket listening successfully listner \n");
	printf("[*] Server waiting for connections \n");

	pollSocket(sock_desc);
	
	return 0;

}
