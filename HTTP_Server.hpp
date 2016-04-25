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

// Represents the internal state of the an HTTP_Request while parsing
// from a string.
enum class ParseState {
  ON_REQUEST_LINE,
  ON_HEADERS,
  ON_BODY,
  COMPLETE,
  ERROR,
};

// Represents an HTTP Request. Has the required parts as per the HTTP
// protocol. The object has an internal state of type "ParseState"
// which is used during incremental parsing of a request string via
// the "Parse" method. The internal state of the the object can be
// viewed using the getParseState.
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

// Represents an HTTP Response. Has the requred parts as per the HTTP
// protocol and can be serialized to a string using the GetString
// method.
class HTTP_Response {
 public:
  string httpVersion;
  string statusCode;
  string reasonPhrase;
  map<string, string> headers;
  string body;
  string GetString();
};

// Encapsulationn of the TCP/IP communicationn. Note: I only created this
// class because I was uncomfortable (at the time) with the basic IPC
// libraries used herein. Perhaps I will remove this in the future.
class TCPIP {
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

// Interface for handling HTTP_Request objects. Each handler must
// supply a "Process" method.
class HTTP_Handler {
 public:
  // This method should handle the Request and populate the
  // repsonse. If the request was not handled, method should return
  // false
  virtual bool Process(HTTP_Request*, HTTP_Response*) = 0;
};

// The HTTP server. This class holds a list of HTTP_Handlers. To
// handle a request, the server passes the request to each handler in
// succession. If the request is handled by the handler, the response
// is immediatly sent back the the client.
class HTTP_Server {
  TCPIP connection;
 public:
  char socket[5] = "80";
  vector<HTTP_Handler*> handlers;
  bool TryGetRequest(HTTP_Request&);
  void Run();
};

// Simple function to report errors
void error(const char *);

#endif
