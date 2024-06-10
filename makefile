CFLAGS = -Wall -g -std=c99
CC = gcc
LFLAGS = -lm

all: server client

server: server.o socket.o
	$(CC) $(CFLAGS) -o raw_server server.o socket.o $(LFLAGS)

server.o: server/server.c server/server.h socket/socket.h
	$(CC) $(CFLAGS) -c server/server.c -o server.o

client: client.o socket.o
	$(CC) $(CFLAGS) -o raw_client client.o socket.o $(LFLAGS)

client.o: client/client.c client/client.h socket/socket.h
	$(CC) $(CFLAGS) -c client/client.c -o client.o

socket.o: socket/socket.c socket/socket.h
	$(CC) $(CFLAGS) -c socket/socket.c -o socket.o



clean:
	rm -f *.o

purge: clean
	rm -f server client

