#include "HTTP_Server.hpp"

using namespace std;

ParseState HTTP_Request::getParseState() {
  return state;
}

// HTTP_Request
void HTTP_Request::Parse(string request) {

  // Validate request string and state before continuing
  if ((request == "") ||
      (state == ParseState::COMPLETE) ||
      (state == ParseState::ERROR))
    return;

  // Add bufferString to request and store new bufferString
  request = bufferString + request;
  bufferString = "";
  
  // Parse lines until there are no more or parsing is complete or
  // there is an error
  istringstream iss(request);
  string line;  
  while ((state != ParseState::COMPLETE) &&
         (state != ParseState::ERROR)) {

    // If we are ON_BODY, get all remaining chars, append to body and
    // return.
    if (state == ParseState::ON_BODY) {
      if (!getline(iss, line, '\0')) return;
    }
    else {
      if (!getline(iss, line, '\n')) return;
      if (line.back() != '\r') {
        bufferString = line;
        return;
      }
    }

    ParseLine(line);
  }
  
  return;
}

void HTTP_Request::ParseLine(string line) {

  switch (state) {
  case ParseState::ON_REQUEST_LINE:
    ParseRequestLine(line);
    break;
  case ParseState::ON_HEADERS:
    ParseHeaderLine(line);
    break;    
  case ParseState::ON_BODY:
    ParseBodyLine(line);
    break;
  case ParseState::COMPLETE:
    break;
  case ParseState::ERROR:
    break;
  }
  return;
}

// Parses an HTML request line. This assumes the request line pieces
// are space separated and all three are present (these are guaranteed
// by the protocol). However, if there are erroneous pieces in the
// request line, they will be ignored.
void HTTP_Request::ParseRequestLine(string line) {
  istringstream iss(line);

  if ((iss >> method) && (iss >> URI) && (iss >> httpVersion)) {
    state = ParseState::ON_HEADERS;
    return;
  }

  state = ParseState::ERROR;
  return;
}

// Reads a line of the format <name> : <attribute> and populates the
// headers map with this information.
void HTTP_Request::ParseHeaderLine(string line) {
  // Handle the blank line case
  if (line == "\r") {

    // Check if content length exists
    if (headers.find("Content-Length") != headers.end()) {
      istringstream iss(headers["Content-Length"]);
      iss >> contentLength;
      state = ParseState::ON_BODY;
    }

    // If no content length, stop parsing is done.
    else {
      state = ParseState::COMPLETE;
    }

    return;
  }

  // Find location of the : and return false if not found.
  size_t colonLoc = line.find(":");
  if (colonLoc == string::npos) {
    state = ParseState::ERROR;
    return;
  }

  // Get strings on either side of the : and return false if either is
  // ""
  string lhs = line.substr(0,colonLoc);
  if (line.size() <= colonLoc + 3) {
    state = ParseState::ERROR;
    return;
  }
     
  string rhs = line.substr(colonLoc+2); // +2 to remove leading space
  rhs.erase(rhs.find_last_not_of(" \n\r\t")+1); // Trim of any white space garbage

  if ((lhs == "")||(rhs == "")) {
    state = ParseState::ERROR;
    return;
  }

  // Finally populate the headers map with the parsed strings and
  // return true to indicate success.
  headers[lhs] = rhs;
  return;
}

void HTTP_Request::ParseBodyLine(string line) {
  // Finally grab rest of content and assign to body.
  body += line;

  // If our content is long enough, we are done parsing
  if (body.size() >= contentLength) {
    state = ParseState::COMPLETE;
  }

  return;
}

void error(const char *msg) {
  perror(msg);
  exit(1);
};

// HTTP Response

string HTTP_Response::GetString() {
  string response;
  ostringstream oss;
  // Append Status Line
  oss <<  httpVersion << " " << statusCode << " " << reasonPhrase << "\r\n";

  // Add response headers
  for (auto & pair : headers) {
    oss << pair.first << ": " << pair.second << "\r\n";
  }

  oss << "\r\n";
  
  // Add body
  oss << body;

  // Return response
  return oss.str();
}

// TCPIP:

int TCPIP::Init(char* portNUM)  {
     
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
     
  if (sockfd < 0) 
    error("ERROR opening socket");
     
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(portNUM);
     
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
     
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  return 1;
}

void TCPIP::Listen()  {
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, 
                     (struct sockaddr *) &cli_addr, 
                     &clilen);
  if (newsockfd < 0) 
    error("ERROR on accept");  
}

int TCPIP::Read(const int size, string &result)  {
  char buffer[size];
  int readInt = 0;
  fd_set rfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&rfds);
  FD_SET(newsockfd, &rfds);
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  retval = select(newsockfd+1, &rfds, NULL, NULL, &tv);

  if (retval == -1) {
    cout << "retval == -1" << endl;
  }
  else if (retval) {
    readInt = read(newsockfd, buffer, size-1);
  }
  else {
    cout << "Timed out" << endl;
  }

  buffer[readInt] = '\0';
  result += buffer;
  
  return readInt;
}    

void TCPIP::Write(char* buffer, int size)  {
  fd_set wfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&wfds);
  FD_SET(newsockfd, &wfds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  retval = select(newsockfd+1, NULL, &wfds, NULL, &tv);

  if (retval == -1) {
    // Error
  }
  else if (retval) {
    write(newsockfd, buffer, size);
  }
  else {
    // timed out
  }
}

void TCPIP::End() {
  close(newsockfd);
}

void TCPIP::Close()  {
  close(sockfd);
  close(newsockfd);
}

bool HTTP_Server::TryGetRequest(HTTP_Request& request) {
  
  const int size = 256;
  string result = "";
  int readInt;
  while(1) {
    result = "";
    readInt = connection.Read(size, result);
    
    // Handle error case
    if (readInt <= 0) {
      return false;
    }

    request.Parse(result);
    ParseState state = request.getParseState();
  if ((state == ParseState::COMPLETE) || (state == ParseState::ERROR)) break;
  }
  
  return true;
}

// HTTP_Server

void HTTP_Server::Run() {
  
  connection.Init(socket);
  while (1) {
    // Create request and response objects
    HTTP_Request request = HTTP_Request();
    HTTP_Response response;

    // Wait for a request the read and parse it
    connection.Listen();
    if (!TryGetRequest(request)) continue;
    
    // Loop through available handlers until one of them handles the
    // request
    for (HTTP_Handler * handler : handlers)
      if (handler->Process(&request, &response)) break;
    
    // Send string back to client and end the connection.
    string responseStr = response.GetString();
    connection.Write(const_cast<char *>(responseStr.c_str()), responseStr.length());
    connection.End();
  }
    
  connection.Close();
  return;
}
