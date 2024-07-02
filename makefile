CFLAGS = -Wall -g -std=c99
CC = gcc
LFLAGS = -lm

all: server client

server: server.o socket.o utils.o
	$(CC) $(CFLAGS) -o raw_server server.o socket.o utils.o $(LFLAGS)

client: client.o socket.o
	$(CC) $(CFLAGS) -o raw_client client.o socket.o utils.o $(LFLAGS)

utils: utils.o socket.o
	$(CC) $(CFLAGS) -o utils utils.o socket.o $(LFLAGS)

server.o: server/server.c server/server.h socket/socket.h utils/utils.h
	$(CC) $(CFLAGS) -c server/server.c -o server.o

client.o: client/client.c client/client.h socket/socket.h 
	$(CC) $(CFLAGS) -c client/client.c -o client.o

socket.o: socket/socket.c socket/socket.h utils/utils.h
	$(CC) $(CFLAGS) -c socket/socket.c -o socket.o

utils.o: utils/utils.c utils/utils.h
	$(CC) $(CFLAGS) -c utils/utils.c -o utils.o
	
clean:
	rm -f *.o

purge: clean
	rm -f raw_server raw_client

