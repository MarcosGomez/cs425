#Marcos Gomez Project 1
CC = gcc
CFLAGS = -g -Wall -std=gnu99


all:	client server


client: client.o
		${CC} ${CFLAGS} -o client client.o


server: server.o
		${CC} ${CFLAGS} -o server server.o


# Add a target for .o
server.o: server.c
	${CC} ${CFLAGS} -c server.c

client.o: client.c
	${CC} ${CFLAGS} -c client.c

# Add a clean target that removes the .o files and executables
clean: 
	/bin/rm -f *.o server client
