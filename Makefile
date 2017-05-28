CC=gcc

all: client server

client: client.c
	$(CC) -o client client.c

server: server.c
	$(CC) -o server server.c

clean:
	rm -f client server
