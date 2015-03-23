//Marcos Gomez
//server
//./server tcp <server port on which to listen>
//The server is required to print out the length of the message received.

/*
  This is a simple server program that uses stream sockets (TCP). It listens on port
  8888, receives 1 short message from a client, then replies to client and exits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/time.h>

/*
 Prints error message and exits.
 */
void error(char *msg){
  perror(msg);
  exit(1);
}

typedef struct packet{
  struct timeval time;
  char payloadLength;
  char payload[159];
}packet;


int main(int argc, char * argv[]){

  int useTCP = -1;//If not tcp, then udp

  // port number to listen on
  int portnum;
  // socket file descriptors, bytes sent and received 
  int sockfd, session, bytes_sent, bytes_received;
  // stores the length of client's sockaddr struct for accept function
  socklen_t clilen;
  // receive buffer
  //packet * buffer = malloc(sizeof(packet));
  char buffer[256]; 
  // structure that contains info about internet addr
  struct sockaddr_in serv_addr, cli_addr;
 

  if (argc < 3) {
    printf ("Usage: server tcp/udp server_port\n");
    exit (0);
  }

  //Set port number
  portnum = atoi(argv[2]);

  //Find out if using TCP or UDP
  if(strcmp(argv[1], "tcp") == 0){
    useTCP = 1;
  }else if(strcmp(argv[1], "udp") == 0){
    useTCP = 0;
  }else{
    printf ("Usage: server tcp/udp server_ip server_port\n");
    exit(0);
  }

  if(useTCP){
    // create a stream socket (TCP)
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
  }else{
    // create a datagram socket (UDP)
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  }
  
  if (sockfd < 0)
    error ("ERROR opening socket");
  // 0 out the struct
  memset (&serv_addr, 0, sizeof(serv_addr));
  // domain is Internet
  serv_addr.sin_family = AF_INET;
  // don't need a specific address, any of the network interface will do
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  // set port number, and stores it in network byte order
  serv_addr.sin_port = htons (portnum);

  // bind socket to an address
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) 
    error("ERROR on binding");

  if(useTCP){
      // listen to socket, set maximum pending connection is 5
      listen (sockfd, 5);
      // set length of client address
      clilen = sizeof (cli_addr);
      // accept first incoming connection on the pending queue, returns a new file descriptor
      session = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (session < 0)
        error("ERROR on accept");
    }else{
      // 0 out the struct
      memset (&cli_addr, 0, sizeof(cli_addr));
      // 0 out the struct
      memset (&cli_addr, 0, sizeof(cli_addr));
      // set length of client address
      clilen = sizeof (cli_addr);
     

      // UDP is connectionless, don't need to listen and accept

    }

  // Keeps running until terminated
  while(1){

    

    if(useTCP){
      // receive data from client and puts it in buffer
      bytes_received = recv (session, buffer, sizeof (packet), 0);
    }else{
      // receive data from client and puts it in buffer
      // let recvfrom fill out where the packet came from and put it in cli_addr
      bytes_received = recvfrom (sockfd, buffer, sizeof (packet), 0, (struct sockaddr *) &cli_addr, &clilen);
    }
    
    if (bytes_received < 0)
      error("ERROR reading from socket");
    //packet * bufPay = (packet *)buffer;

    // printf("seconds: %ld\n", bufPay->time.tv_sec);
    printf ("%lu\n",strlen(buffer));
    // printf("Payload %d\n", strlen(bufPay->payload));

    if(useTCP){
      // send client a reply
      bytes_sent = send (session, buffer, sizeof (buffer), 0);
    }else{
      // send client a reply with newly filled out cli_addr
      bytes_sent = sendto (sockfd, buffer, sizeof (buffer), 0, (struct sockaddr *) &cli_addr, clilen);
    }
    
    if (bytes_sent < 0)
      error("ERROR writing to socket");

    // 0 out the recieve buffer
    memset (buffer, 0, sizeof (buffer));
  }
  //free(buffer);

  if(useTCP){
    close (session);
  }
  close (sockfd); 
  return 0; 
}