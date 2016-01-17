#Define the compile command to be used:
CC=g++ -std=c++11
#Define the flags to be used with the compile statment:
CFLAGS = -g -Wall

#Define the rules in the dependancy tree:
progs :  HTTP_Server.o

HTTP_Server.o : HTTP_Server.cpp HTTP_Server.hpp
	$(CC) -fPIC -c -o $@ $< $(CFLAGS)

.PHONY: install clean

clean:
	rm *.o

install:
	$(CC) -shared -fPIC -o libljjhttpserver.so HTTP_Server.o
	install -m 0755 libljjhttpserver.so /usr/local/lib/
	install -m 0755 HTTP_Server.hpp /usr/local/include/
	install -m 0755 HTTP_Example_Handlers.hpp /usr/local/include/
	rm libljjhttpserver.so
