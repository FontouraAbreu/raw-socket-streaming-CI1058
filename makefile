CFLAGS = -Wall -g -std=c99
CC = gcc
LFAGS = -lm

all: main

main: main.o 
	$(CC) $(CFLAGS) -o main main.o $(LFLAGS)