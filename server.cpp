#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <fstream>
#include "msock.h"
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

int main(int argc, char *argv[]) {
  int sock; //multicast recieve socket
  int newsock; //multicast send socket
  int movsock; //movie unicast socket
  int clisock; //movie sending socket
  struct sockaddr_in serv_addr, portget, mov_addr; //structures to manage the socket
  socklen_t clilen; //holds socket length
  socklen_t movlen; //holds movie socket length
  struct sockaddr_in * cli_addr; //socket addr pointer
  int bytes; //bytes transferred through socket
  fd_set fdset, fdset1; //socket timeout pointers
  struct timeval timeout; //holds timeout information on socket
  char dir[256]; //holds directory to use to send movies
  char message[256]; //message recieved on multicast
  char response[256]; //message sent on multicast
  char mv1[256];
  char mv2[256];
  char mv3[256];
  char bindport[256]; //holds unicast port info
  struct ifaddrs *ifap, *ifa; //structs for getting port and address info
  char *addr; //address pointer
  socklen_t addrlen = sizeof(portget); //holds the size of portget, used to get port
  int movport; //holds the movie send port

  cout << "SERVER PEER \nServer started. \n";

  getcwd(dir, 256); //set home directory
  if(dir == NULL) //if getcwd fails, stop the program
    {
      //couldn't get working directory
      perror("getcwd");
      exit(1);
    }

  if(argc == 2) //if the user has entered a custom movie directory
    {
      strcpy(dir, argv[1]); //set dir to that directory
      if(chdir(dir) != 0) //switch working directory to custom directory
	{
	  perror("chdir");
	  exit(1);
	}
      cout << "Using directory " << dir << "\n";
    } else {
    cout << "Using current working directory \n";
  }
  //set the timeout information
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
  strcpy(message, " ");

  //set up multicast recieve port
  if ((sock=msockcreate(RECV, ADDRS, PORT)) < 0)
    {
      perror("msockcreate");
      exit(1);
    }

  //set up multucast send port
  if ((newsock=msockcreate(SEND, ADDRR, 9003)) < 0)
    {
      perror("msockcreate");
      exit(1);
    }

  //set up unicast movie socket
  if ((movsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("creating socket");
    exit(1);
  }
  int i = 9001; //iterator for socket port to bind to

  /* bind our local address so client can connect to us */
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(i);
  //loop through available ports and select one that is open
  while(1)
    {
      if (bind(movsock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
	i++; //increase port number
    if (i == 9201) //tried 200 ports, should give up
      {
	perror("bind");
	exit(1);
      }
    continue; //keep trying
      } else //port bound, move on
    {
      break;
    }
    }

  /* mark socket as passive, with backlog num */
  if (listen(movsock, 5) == -1) {
    perror("listen");
    close(sock);
    exit(1);
  }
  //set up time out pointers
  FD_ZERO(&fdset);
  FD_ZERO(&fdset1);
  //loop all from here on out
  while(1)
    {
      FD_SET(sock, &fdset1); //set up time out pointer
      int testmulti; //select return value dummy variable
      //try the multicast recieve port for incoming signals, process if they are there, or move on
  if((testmulti = select(sock + 1, &fdset1, NULL, NULL, &timeout)) == -1)
    {
      perror("select");
      exit(1);
    }
  //multicast signal recieved
  if(FD_ISSET(sock, &fdset1))
    {
      //get message
    bytes = mrecv(sock, message, 256);
    if (bytes < 0) {
      perror("mrecv");
      exit(1);
    }
    message[bytes] = '\0';
    cout << "Recieved search request for " << message << "\n";  
    
    int filefound=0; //used to check if the file is real
    ifstream filecheck (message); //try to find the file being requested
    if(filecheck.is_open()) //try to open file
      {
	cout << "Movie here \n";
	filefound =1; //file found
	strcpy(response, message);
	strcat(response, " ");
	filecheck.close();
      } else
      {
	cout << "Movie not here, so no response \n";
	filefound = 0;
	strcpy(response, " ");
      }
    //pull out the unicast ip address
  getifaddrs(&ifap);
  for(ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
      if(ifa->ifa_addr->sa_family==AF_INET)
	{
	  if(strcmp(ifa->ifa_name, "eth0") == 0)
	    {
	      cli_addr = (struct sockaddr_in *) ifa->ifa_addr; 
	      addr = inet_ntoa(cli_addr->sin_addr);
	      break;
	    }
	}
    }
  //add the ip address to the response
  strcat(response, addr);
  strcat(response, " ");
  //get the unicast port number
  if(getsockname(movsock, (struct sockaddr *)&portget, &addrlen) == 0 && portget.sin_family == AF_INET && addrlen == sizeof(portget))
    {
      movport = ntohs(portget.sin_port);
      // cout << movport << "\n";
    } else
    {
      perror("getsockname");
      exit(1);
    }
   
  sprintf(bindport,"%d",movport);
  //add the uniicast port value to the response
  strcat(response, bindport);
    
  freeifaddrs(ifap);
  //if file was found, send out mulicast response
  if(filefound == 1)
    {
      //cout << "Sending response \n";
      // cout << "response: " << response << "\n";
      if(msend(newsock, response, strlen(response)) < 0)
	{
	  perror("msend");
	  exit(1);
	}
    } else {
    //continue;
  }
  cout << "Listening... \n";
    }
  //set up the time out for the unicast socket
  FD_SET(movsock, &fdset);
  // cout << "gets here \n";
  int seluni; //dummy variable for unicast select
  //look for incoming signals on the unicast socket, process them or move on if none
   if((seluni = select(movsock + 1, &fdset, NULL, NULL, &timeout)) == -1)
    {
      perror("select");
      exit(1);
    }
   //if a movie request exists
  if(FD_ISSET(movsock, &fdset))
    {
      // cout << "gets here 2\n";
      int clisock; //child socket
      socklen_t  movlen; //child socket length
      struct sockaddr_in accaddr; //child socket info
      clilen = sizeof(cli_addr); //child socket address size
      //accept child socket
  clisock = accept(movsock, (struct sockaddr *) &accaddr, &movlen);
  if (clisock < 0) {
    perror("accepting connection");
    exit(1);
  }
  //cout << "Reading in information \n";
  int len; //length of frame being sent
  int movcount; //length of message recieved
  char inbuf[512]; //buffer for movie request
  char movbuf[2048]; //buffer for frame to be sent, frames bigger than this wont work
  string movline; //line read in from movie file
  char movdir[256]; //directory to look for movie in
  struct timespec delay; //frame sending delay time
  delay.tv_sec = 2; //wait 2 seconds between frames
      delay.tv_nsec = 0;
      //strcpy(movdir, dir); //set base directory in movdir
      // strcat(movdir, "/");
      if ((movcount = read(clisock, inbuf, 512)) > 0) { //read in request
	inbuf[movcount] = '\0'; 
	cout << "Connection request recieved. \nStreaming movie " << inbuf << " to client. \n";
      } else {
	perror("read");
	close(clisock);
      }
      // strcat(movdir, inbuf);
      //      cout << movdir << "\n";
      ifstream movfile (inbuf); //open movie file
      if(movfile.is_open()) //if the movie exists
	{ //loop through the file pulling out a line each iteration
	  while(getline(movfile,movline)) //get line
	    {
	      // cout << "movline:" << movline << "\n"; 
	      // strcat(movbuf, movline.c_str());
	      // cout << movbuf << "\n";
	      //  strcat(movbuf, "\n");
	      if(strcmp(movline.c_str(), "end") == 0) //if the frame is complete
		{
		  getline(movfile,movline); //discard end line
		  nanosleep(&delay, NULL); //wait 2 seconds
		  len = strlen(movbuf); //get the frame size
		  //cout << "++++++++++++++++++++++++++++++++\n";
		  if(sendall(clisock, movbuf, &len) < 0) //send the frame
		{
		  perror("sendall");
		  close(clisock);
		  movfile.close();
		  break;
		}
		  //cout << movbuf;
		  // cout << "++++++++++++++++++++++++++++++++\n";
		  movbuf[0] = '\0'; //clear the frame buffer
		}
	      strcat(movbuf, movline.c_str()); //put the c string line version into the frame
	      strcat(movbuf, "\n"); //go tho the next line in the buffer
	    }
	  movfile.close(); //close the file
	  close(clisock); //close the client socket
	} else { //someone somehow requested a movie that isnt here, so ignore this request
	    cout <<"bad file name \n"; 
	    close(clisock);
	  }
      cout << "Listening... \n";
    }
    } //loop back to the beginning
}

