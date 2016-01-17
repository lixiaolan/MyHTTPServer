#ifndef LJJ_HTTP_SERVER
#define LJJ_HTTP_SERVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <map>
#include <vector>
#include <fstream>

using namespace std;

// Represents an HTTP Request
class HTTP_Request {
 public:
  string method;
  string requestURI;
  string httpVersion;
  map< string, string > headers;
  string body;  
  bool Parse(string);
 private:
  bool ParseRequestLine(string);
  bool ParseHeaderLine(string);
};

// Represents an HTTP Response
class HTTP_Response {
 public:
  string httpVersion;
  string statusCode;
  string reasonPhrase;
  map<string, string> headers;
  string body;
  string GetString();
};

// Encapsulationn of the TCP/IP communicationn
class TCPIP  {
 private:
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
 public:
  int Init(char*);
  void Listen();
  string Read();
  void Write(char*, int);
  void End();
  void Close();
};

// Interface for handling HTTP_Request objects
class HTTP_Handler {
 public:
  // This method should handle the Request and populate the
  // repsonse. If the request was not handled, method should return
  // false
  virtual bool Process(HTTP_Request*, HTTP_Response*) = 0;
};

// The HTTP server
class HTTP_Server {
 public:
  char socket[5] = "80";
  vector<HTTP_Handler*> handlers;
  void Run();
};

void error(const char *);

#endif
