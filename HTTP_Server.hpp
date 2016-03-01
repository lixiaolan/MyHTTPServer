#ifndef LJJ_HTTP_SERVER
#define LJJ_HTTP_SERVER

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
#include <fcntl.h>
#include <time.h>

using namespace std;

enum class ParseState {
  ON_REQUEST_LINE,
  ON_HEADERS,
  ON_BODY,
  COMPLETE,
  ERROR,
};

// Represents an HTTP Request
class HTTP_Request {
 public:
  string method;
  string URI;
  string httpVersion;
  map<string, string> headers;
  string body;  
  void Parse(string);
  ParseState getParseState();
 private:
  unsigned contentLength;
  string bufferString;
  ParseState state;
  void ParseLine(string);
  void ParseRequestLine(string);
  void ParseHeaderLine(string);
  void ParseBodyLine(string);
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
  int Read(const int, string&);
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
  TCPIP connection;
 public:
  char socket[5] = "80";
  vector<HTTP_Handler*> handlers;

  bool TryGetRequest(HTTP_Request&);
  void Run();
};

void error(const char *);

#endif
