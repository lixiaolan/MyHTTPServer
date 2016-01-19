#include "HTTP_Server.hpp"

using namespace std;

// HTTP_Request
bool HTTP_Request::Parse(string request) {
  istringstream iss(request);
  string line;
  
  // Get header line and send to ParseRequestLine
  getline(iss, line);
  if (!ParseRequestLine(line)) return false;

  // Loop on getting header lines and send each ParseHeaderLine
  do {
    getline(iss, line);
  } while (ParseHeaderLine(line));
    
  // Finally grab rest of content and assign to body.
  while (getline(iss, line)) {
    body += line + "\n";
  }

  return true;
}

// Parses an HTML request line. This assumes the request line pieces
// are space separated and all three are present (these are guaranteed
// by the protocol). However, if there are erroneous pieces in the
// request line, they will be ignored.
bool HTTP_Request::ParseRequestLine(string line) {
  istringstream iss(line);

  if ((iss >> method) && (iss >> requestURI) && (iss >> httpVersion))
    return true;

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
}

string TCPIP::Read()  {

  const int size = 256;
  string result = "";
  string test = "";
  char buffer[size];
  size_t readInt;
  
  while(1) {
    bzero(buffer,size+1);
    readInt = read(newsockfd, buffer, size-1);
    if (readInt < 1) break;
    result += buffer;
  }
  return result;
}

void TCPIP::Write(char* buffer, int size)  {
  n = write(newsockfd, buffer, size);
  if (n < 0) error("ERROR writing to socket");
}

void TCPIP::End() {
  close(newsockfd);
}

void TCPIP::Close()  {
  close(sockfd);
  close(newsockfd);
}

// HTTP_Server
void HTTP_Server::Run() {
  TCPIP connection;
  
  connection.Init(socket);
  while (1) {
    // Create request and response objects
    HTTP_Request request = HTTP_Request();
    HTTP_Response response;

    // Wait for a request the read and parse it
    connection.Listen();
    string requestString = connection.Read();
    request.Parse(requestString);

    // Loop through aviliable handlers until one of them handles the
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
