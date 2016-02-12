#include "HTTP_Server.hpp"

using namespace std;

// HTTP_Request
bool HTTP_Request::Parse(string request) {
  istringstream iss(request);
  string line;

  // Clear out all fields:
  method = "";
  requestURI = "";
  httpVersion = "";
  headers = map<string, string>();
  body = "";
  
  // Get header line and send to ParseRequestLine
  if (!getline(iss, line)) return false;
  if (!ParseRequestLine(line)) {
    cout << "Parse failed on request line: " + line << endl;
    return false;
  }

  // Loop on getting header lines and send each ParseHeaderLine
  while(1) {
    cout << "TEST ONE" << endl;
    if (!getline(iss,line)) {
      cout << "Parse failed to get line during headers parsing" << endl;
      return false;
    }
    if (!ParseHeaderLine(line)) break;
  } 

  // If we did not run out of lines than we must have failed a header
  // line parse. If the offending line is not blank, we must have had
  // a bad header line.
  if ((line != "") && (line != "\r")) {
    cout << "Parse failed on header line: >>>" + line + "<<<" << endl;
    return false;
  }

  if (method == "GET") {
    return true;
  }
      
  // Finally grab rest of content and assign to body.
  while (getline(iss, line)) {
    cout << "TEST TWO" << endl;
    body += line + "\n";
  }

  // If method is POST or PUT the body content must be at least the
  // length specified in the "Content-Length" header
  if ((method == "POST") || (method == "PUT")) {
    unsigned contentLength;
    istringstream iss(headers["Content-Length"]);
    iss >> contentLength;
    
    if (body.length() < contentLength) {
      cout << "Parse failed on content length" << endl;
      cout << "body.length()  : " << body.length() << endl;
      cout << "content length : " << contentLength << endl;
      return false;
    }
  }
    
  return true;
}

// Parses an HTML request line. This assumes the request line pieces
// are space separated and all three are present (these are guaranteed
// by the protocol). However, if there are erroneous pieces in the
// request line, they will be ignored.
bool HTTP_Request::ParseRequestLine(string line) {
  cout << "in parse request line" << endl;
  istringstream iss(line);

  if ((iss >> method) && (iss >> requestURI) && (iss >> httpVersion)) {
    cout << "exit parse request line true" << endl;
    return true;
    }

  cout << "exit parse request line false" << endl;
  return false;
}

// Reads a line of the format <name> : <attribute> and populates the
// headers map with this information.
bool HTTP_Request::ParseHeaderLine(string line) {
  // Find location of the : and return false if not found.
  size_t colonLoc = line.find(":");
  if (colonLoc == string::npos) return false;

  // Get strings on either side of the : and return false if either is
  // ""
  string lhs = line.substr(0,colonLoc);
  if (line.size() <= colonLoc + 3) return false;
  string rhs = line.substr(colonLoc+2); // +2 to remove leading space
  rhs.erase(rhs.find_last_not_of(" \n\r\t")+1); // Trim of any whitespace garbage

  if ((lhs == "")||(rhs == "")) return false;

  // Finally populate the headers map with the parsed strings and
  // return true to indicate success.
  headers[lhs] = rhs;
  return true;
}

void error(const char *msg) {
  perror(msg);
  exit(1);
};

// HTTP Response

// Method to genereate the html response. See
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html#sec6
string HTTP_Response::GetString() {
  string response;
  // Append Status Line
  if ((httpVersion != "")&&
      (statusCode != "")&&
      (reasonPhrase != ""))
    response += httpVersion + " " + statusCode + " " + reasonPhrase + "\n";

  // Add response headers
  for (auto & pair : headers) {
    response += pair.first + ": " + pair.second + "\n";
  }
  
  // Add body
  response += body;

  // Return response
  return response;
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
  
  // Make socket non-blocking
  // int x;
  // x = fcntl(newsockfd, F_GETFL, 0);
  // fcntl(newsockfd, F_SETFL, x | O_NONBLOCK);
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
    cout << "READ ERROR!!!!!!!!!" << endl;
  }
  else if (retval) {
    readInt = read(newsockfd, buffer, size-1);
  }
  else {
    cout << "Read Socket timed out :(" << endl;
  }

  buffer[readInt] = '\0';
  result += buffer;
  
  return readInt;
}    

void TCPIP::Write(char* buffer, int size)  {
  int writeInt = 0;
  fd_set wfds;
  struct timeval tv;
  int retval;

  FD_ZERO(&wfds);
  FD_SET(newsockfd, &wfds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  retval = select(newsockfd+1, NULL, &wfds, NULL, &tv);

  if (retval == -1) {
    cout << "WRITE ERROR!!!!!!!!!" << endl;
  }
  else if (retval) {
    writeInt = write(newsockfd, buffer, size);
  }
  else {
    cout << "Write Socket timed out :(" << endl;
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
    cout << "Before Read" << endl;
    readInt = connection.Read(size, result);
    cout << "After Read" << endl;
    
    // Handle error case
    if (readInt <= 0) {
      return false;
    }
    
    cout << "before parse" << endl;
    if (request.Parse(result)) break;
    cout << "after parse" << endl;
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
    cout << "Before listen" << endl;
    connection.Listen();
    cout << "After listen" << endl;
    if (!TryGetRequest(request)) continue;
    
    // Loop through aviliable handlers until one of them handles the
    // request
    cout << "Handling" << endl;
    for (HTTP_Handler * handler : handlers)
      if (handler->Process(&request, &response)) break;
    cout << "Done handling" << endl;
    
    // Send string back to client and end the connection.
    string responseStr = response.GetString();
    cout << "before writing" << endl;
    connection.Write(const_cast<char *>(responseStr.c_str()), responseStr.length());
    cout << "after writing" << endl;
    connection.End();
  }
    
  connection.Close();
  return;
}
