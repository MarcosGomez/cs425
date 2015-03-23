//Marcos Gomez
//client
//./client tcp <server ip> <server port>
//The output of the client program must be in the following format –
// <message received from the server>
// <round trip time in milliseconds upto 3 decimal places>

/*
  This is a simple client program that uses stream sockets (TCP) to communicate to a given server
  who is listening on port 8888. It sends 1 short message to the server and, then receives the
  server's reply.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <unistd.h>

#include <sys/time.h>

/*
  Prints error message and then exits.
 */
void error(char *msg) {
  perror(msg);
  exit(0);
}

// typedef struct packetHeader{
//   struct timeval time;
//   int payloadLength;
// } packetHeader;
typedef struct packet{
  struct timeval time;
  short payloadLength;
  char payload[159];
}packet;
// struct timeval {
//   time_t      tv_sec;      seconds 
//   suseconds_t tv_usec;    /* microseconds */
// };

int main(int argc, char *argv[]) {

  int useTCP = -1;//If not tcp, then udp
  //char input[256];

  // socket file descriptor, bytes sent and received
  int sockfd, bytes_sent, bytes_received;
  // port number the server is listening on
  int portnum = 888;
  // structure that contains info about internet addr
  struct sockaddr_in serv_addr;

  // message to send to server
  char send_buffer[256];
  // buffer used to receive a message from server
  char recv_buffer [256];

  // Exit if no ip address is provided
  if (argc < 4) {
    printf ("Usage: client tcp/udp server_ip server_port\n");
    exit (0);
  }

  //Set port number
  portnum = atoi(argv[3]);

  //Find out if using TCP or UDP
  if(strcmp(argv[1], "tcp") == 0){
    useTCP = 1;
  }else if(strcmp(argv[1], "udp") == 0){
    useTCP = 0;
  }else{
    printf ("Usage: client tcp/udp server_ip server_port\n");
    exit(0);
  }
  

  // create socket
  if(useTCP){
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
  }else{
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  }
  
  if (sockfd < 0) 
    error ("ERROR opening socket\n");
  // 0 out the struct
  memset (&serv_addr, 0, sizeof (serv_addr));
  // domain is Internet
  serv_addr.sin_family = AF_INET;

  // copy convert and copy server's ip addr into serv_addr
  if(inet_pton(AF_INET, argv [2], &serv_addr.sin_addr) < 0)
    error ("ERROR, copying ip\n");
  // set server's port number, stores it in network byte order
  serv_addr.sin_port = htons (portnum);

  if(useTCP){
     // connect to server
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) 
      error ("ERROR connecting");
  }

  //Keeps running until terminated
  while(1){


    fgets(send_buffer, sizeof(send_buffer), stdin);
    if(send_buffer[strlen(send_buffer) - 1] == '\n'){
      send_buffer[strlen(send_buffer) - 1] = '\0';
    }

    if(useTCP){
      // send message to server
      bytes_sent = send (sockfd, send_buffer, strlen(send_buffer), 0);
    }else{
      // send message to server
      bytes_sent = sendto (sockfd, send_buffer, strlen(send_buffer), 0, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
    }
    
    if (bytes_sent < 0) 
      error ("ERROR writing to socket");
    // 0 out receive buffer
    memset (recv_buffer, 0, sizeof (char) * 256);
    
    struct timeval beforeTime = {0};
    gettimeofday(&beforeTime, NULL);
    
    if(useTCP){
      // receive message from server
      bytes_received = recv (sockfd, recv_buffer, sizeof (recv_buffer), 0);
    }else{
      // receive message from server
      // arg 5 and 6 (src sockaddr and its len) are filled in by recvfrom
      // since there is not need for this to be filled out in the context of this program
      // we just pass in NULL
      bytes_received = recvfrom (sockfd, recv_buffer, sizeof (recv_buffer), 0, NULL, NULL);
    }
    struct timeval afterTime = {0};
    gettimeofday(&afterTime, NULL);

    
    if (bytes_received < 0) 
      error("ERROR reading from socket");

    printf("%s\n",recv_buffer);

    //Print change of time in milliseconds
    printf("%.3lf\n\n",  (afterTime.tv_sec - beforeTime.tv_sec)*1000.0 + 
      ((afterTime.tv_usec - beforeTime.tv_usec)/ 1000.0));
    
    
  }

  close (sockfd);
  return 0;
}