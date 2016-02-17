all:
	gcc -c msock.c
	g++ -c client.cpp
	g++ msock.o client.o -o client
	g++ -c server.cpp
	g++ msock.o server.o -o server
clean:
	rm *o client server

