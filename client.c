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


#define PORT "5000"
#define IP "127.0.0.1"
#define BUFLEN 1024

typedef enum {
	HELP,
	QUIT,
	SEND_MSG,
	COMMAND_ERR
} actions;

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
    	return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handleAction(int sock_desc, actions action, char *buf) {
	switch(action) {
		case QUIT : 		printf("See Ya Later ... \n");
							exit(4);

		case COMMAND_ERR :  printf("[-] Sorry not a valid command Try again..\n");

		case HELP : 		printf(" ***** HELP *****\n  ?quit or ?exit : To exit chat\n  ?help : For help\n  !help : For server help\n");
							break;

		case SEND_MSG :		if(send(sock_desc, buf, strlen(buf), 0) == -1) {
  								perror("Error : send");
  							}
  							break;
	}
}

void handleServerResponse(int sock_desc) {
	char buf[BUFLEN];
	memset(buf, '\0', sizeof(buf));
	int msg_len;
	if((msg_len = recv(sock_desc, buf, 1023, 0)) <= 0) {
		perror("recv :error");
	}
	buf[msg_len] = '\0';
	printf("%s", buf);
	fflush(stdout);
}

void handleUserInput(int sock_desc) {
	char buf[BUFLEN];
	memset(buf, '\0', sizeof(buf));
	if(fgets(buf, 1023, stdin) == NULL) {
  		return;
  	}
  	actions action;
  	char temp[30];
  	memset(temp, '\0', sizeof(temp));
  	int k = 1;
  	switch(buf[0]) {
  		case '?' :  while(!isspace(buf[k])) {
  						temp[k-1] = buf[k];
  						k++;
  					}
  					temp[k-1] = '\0';
  					if(!(strcmp(temp, "quit") && strcmp(temp, "exit")))
  						action = QUIT;
  					else
  						if(!strcmp(temp, "help"))
  							action = HELP;
  						else
  							action = COMMAND_ERR;
  					break;

  		default :	action = SEND_MSG;
  					break;
  	}
  	handleAction(sock_desc, action, buf);
}

void pollSocket(int sock_desc) {
	int i;
	fd_set masterfd, readfds;
	int fdmax;
	int msg_len;
	FD_ZERO(&masterfd);
	FD_ZERO(&readfds);
	FD_SET(sock_desc, &masterfd);
	FD_SET(STDIN_FILENO, &masterfd);
	fdmax = sock_desc;
	fflush(stdin);

	while(1) {
		readfds = masterfd;
		if( select(fdmax+1, &readfds, NULL, NULL, NULL) == -1 ) {
			perror("Error : Select");
			exit(3);
		}
		for(i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &readfds)) {
				if(i == sock_desc) {
					// Handle input from the server
					handleServerResponse(sock_desc);
				}
				else {
					// Handle input from the user
					handleUserInput(sock_desc);
				}
			}
		}
	}
}


/*===============================
          MAIN FUNCTION
===============================*/
int main(int argc, char *argv[]) {
	// Usage ./client [IP] [PORT]
	char *port, *ip;
	if(argc != 3) {
		printf("[*] Usage  :  %s [IP] [PORT] \n", argv[0]);
		printf("Starting client with default IP and PORT \n");
		ip = IP;
		port = PORT;
	}
	else {
		ip = argv[1];
		port = argv[2];
	}
	int sock_desc;
	struct addrinfo hints, *servinfo, *result;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, port, &hints, &servinfo);
	for(result = servinfo; result!= NULL; result = result->ai_next) {
		sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock_desc < 0) {
			printf("[?] Socket creation Failed ...\n");
			perror("Error : Socket");
			continue;
		}

		if(connect(sock_desc, result->ai_addr, result->ai_addrlen) == -1) {
			close(sock_desc);
			perror("Client : connect");
			continue;
		}
		break;
	}
	if(result == NULL) {
		fprintf(stderr, "Client failed to connect \n");
		exit(2);
	}

	char servAddr[INET6_ADDRSTRLEN];
	inet_ntop(result->ai_family, get_in_addr((struct sockaddr *)result->ai_addr), servAddr, sizeof servAddr);
	printf("[*] Connecting to %s \n", servAddr);
	pollSocket(sock_desc);
	freeaddrinfo(servinfo);
}
