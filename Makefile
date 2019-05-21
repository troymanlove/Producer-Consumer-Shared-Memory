CC = gcc
CFLAGS = -I

PA2: main.c 
	$(CC) -Wall -o PA2 main.c -lpthread
