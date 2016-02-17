JRWatson/Jacob Watson
Readme
Project 3

HOW TO COMPILE
Make sure that client.cpp, server.cpp, msock.c, msock.h, and makefile are in the directory.
Then type:
        make
OR
        gcc -c msock.c
        g++ -c client.cpp
        g++ msock.o client.o -o client
        g++ -c server.cpp
        g++ msock.o server.o -o server
HOW TO RUN:
The client and server sections are two seperate programs.
This program has been tested on CCCWORK4 and CCCWORK3.
To run the client, compile, then type:
         ./client
You will then be prompted to type a movie name. Type the movie name and hit enter to play the movie.
The client will then try to find and play the movie from a server. If it can, it will play the movie, and if it cant, it will display a time out message.
The program will then prompt for another movie.
To run the server, compile, then type:
./server [directory]
WHERE directory is the directory where your movie files are stored. Leave out the directory to use the current working directory.
NOTE: Everything in the selected directory is assumed by the program to be a movie file. If a file is in that directory, the program will treat it like a movie, and stream it like one. Be careful.

NOTES:
This program has been tested with some of the movies from the proj3 webpage:
pong
tiger cub
walk
These files should work with this streaming program.
Running a server and client instance in the same shell is not recommended, as output from one may interfere with the output fron the other.
IMPORTANT: Any movie with a frame that is larger than 2048 characters will not work properly with this program.