#include "msock.h"
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
using namespace std;

#define PORT 9001
#define ADDRS "239.1.2.3"
#define ADDRR "239.1.1.1"

//sendall routine taken from Beej's guide to network programming
int sendall(int s, char *buf, int *len)
{
  int total = 0;        // how many bytes we've sent
  int bytesleft = *len; // how many we have left to send
  int n;

  while(total < *len) {
    n = send(s, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }

  *len = total; // return number actually sent here

  return n==-1?-1:0; // return -1 on failure, 0 on success
}


int main( int argc, const char* argv[] )
{
  int movsock;
  struct sockaddr_in mov_addr;
  struct hostent *hp;
  string requeststr; //holds users request
  char request[256]; //holds cstring of users request
  int sockchk; //used to check if a socket functioned correctly
  int sock; //socket to send on
  int newsock; //socket to recieve on
  int bytes; //number of bytes recieved
  fd_set fdset; //used to check if a port has recieved any data
  struct timeval timeout; //holds the receivers timeout values
  char * sendptr; //send pointer
  char send[256]; //send buffer
  char message[256]; //incoming message buffer
  char address[256]; //unicast address buffer
  char movport[256]; //movie buffer
  char * pch; //tokenizer pointer

  cout << "CLIENT PEER \nClient started \n";

  //create multicast out socket
  if ((sock=msockcreate(SEND, ADDRS, PORT)) < 0) 
    {
    perror("msockcreate");
    exit(1);
    }

  //set up multicast receive port
  if ((newsock=msockcreate(RECV, ADDRR, 9003)) < 0)
    {
      msockdestroy(sock);
      perror("msockcreate");
      exit(1);
    }
  //everything past this point loops
  while(1)
    {
  cout << "Movie name: ";
  getline(cin, requeststr); //get movie name
  if (requeststr.c_str() == NULL) //make sure it isnt null
    {
      cout << "Bad input. Please enter a movie name. \n";
      continue;
    }

  strcpy(request, requeststr.c_str()); //pull out request
  cout << "Sending search request for " << requeststr << "...\n";
  //send request
  if(msend(sock, request, strlen(request)) < 0)
    {
      perror("msend");
      msockdestroy(sock);
      msockdestroy(newsock);
      exit(1);
    }
  //set up timeout pointer
  FD_ZERO(&fdset);

  //set up a 3 second timeout
    FD_SET(newsock, &fdset);
    timeout.tv_sec = 3;        
    timeout.tv_usec = 0;


    int testbug; //used to hold selects output, dummy variable
    //set the timeout on the recieve socket
    if((testbug = select(newsock + 1, &fdset, NULL, NULL, &timeout)) == -1)
      {
	perror("select");
	msockdestroy(sock);
	msockdestroy(newsock);
	exit(1);
      }
    //if a message has been recieved on the multicast socket
    if(FD_ISSET(newsock, &fdset))
      {
	bytes = mrecv(newsock, message, 256); //read in message
	    message[bytes] = '\0'; 
	    // cout << message << " \n";
	    //tokenize the message into request, address, and port strings
	    pch = strtok(message, " ");
	    pch = strtok(NULL, " ");
	    strcpy(address, pch);
	    pch = strtok(NULL, " ");
	    strcpy(movport, pch);
	    // cout << address << " \n" << movport << "\n";

	    //locate the host name
	    bzero((void *) &mov_addr, sizeof(mov_addr)); //prepare mov_addr
	    if ((hp = gethostbyname(address)) == NULL) { //get the host address
	      perror("host name error"); //bad host name
	      msockdestroy(sock);
	      msockdestroy(newsock);
	      exit(1);
	    }
	    bcopy(hp->h_addr, (char *) &mov_addr.sin_addr, hp->h_length); //copy over the host address

	    mov_addr.sin_family = AF_INET;
	    mov_addr.sin_port = htons(atoi(movport)); //set port

	    //set up the socket
	    if ((movsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	      perror("creating socket");
	      msockdestroy(sock);
	      msockdestroy(newsock);
	      exit(1);
	    }

	    //try to connect to the server
	    if (connect(movsock, (struct sockaddr *) &mov_addr, sizeof(mov_addr)) < 0) {
	      perror("can't connect");
	      close(movsock);
	      msockdestroy(sock);
	      msockdestroy(newsock);
	      exit(1);
	    }
	    //set up the timeout for the unicast socket
	    FD_SET(movsock, &fdset);

	    cout << "connection established \n";
	    int len; //holds length of request, needed for sendall
	    len = strlen(request);
	    if(sendall(movsock, request, &len) < 0) //send the message
	      {
		perror("sendall");
		close(movsock);
		msockdestroy(sock);
		msockdestroy(newsock);
	      }
	    //set a timeout on the movie unicast request
	    if((testbug = select(movsock + 1, &fdset, NULL, NULL, &timeout)) == -1)
	      {
		perror("select");
		close(movsock);
		msockdestroy(sock);
		msockdestroy(newsock);
		exit(1);
	      }
	    char movbuf[2048]; //buffer for the movie. movies with frames larger than this wont work right
	    int movcount; //holds the number of characters recieved per frame
	    strcpy(message, " "); //blanks message as a precaution
	    if(FD_ISSET(movsock, &fdset)) //if unicast bytes are waiting to be recieved
	      {
		while (1) //loop until the movie is finished
		  {
		    movcount = read(movsock, movbuf, 2048); //get a frame
		    movbuf[movcount] = '\0'; //prevent bad characters in the frame
		  // cout << movcount << "\n";
		    if (movcount < 1) //if the socket is done recieving movie data, exit the loop
		    {
		      break;
		    }
		    cout << string( 100, '\n' ); //clear the screen for the next frame
		    cout << movbuf << "\n"; //output next frame
		  // cout << "!!! \n";
		  }
	      } else { //if no movie bytes are recieved from the server, time out
	      cout << "Timeout, movie not found. \n";
	    }
	    close(movsock); //close the movie socket for the next run
      } else //if no response is recieved from the server over multicast, time out
      {
	cout << "Timeout, movie not found. \n"; 
      }

    // cout << "gets here3 "  << testbug << " \n";
    }

  if(msockdestroy(sock) < 0)
    {
      perror("msockdestroy");
      exit(1);
    }
  if(msockdestroy(newsock) < 0)
    {
      perror("msockdestroy");
      exit(1);
    }
  if(close(movsock) < 0)
    {
      perror("close");
      exit(1);
    }

   }
