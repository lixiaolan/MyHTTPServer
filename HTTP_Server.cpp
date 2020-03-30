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
    if ((state != ParseState::ON_BODY) && (request[0] == '\n')) {
        request = request.substr(1);
    }
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

bool HTTP_Server::tryGetRequest(HTTP_Request& request) {
  
    const int size = 256;
    char buf[size];
    string result = "";
    int readInt;
    while(1) {
        result = "";
        readInt = read(0, buf, size);
        result += buf;
    
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

bool HTTP_Server::trySendResponse(HTTP_Response& response) {
    // Send string back to client and end the connection.
    cout << response.GetString();
    return true;
}

// HTTP_Server
void HTTP_Server::Run() {
  
    // Create request and response objects
    HTTP_Request request = HTTP_Request();
    HTTP_Response response;
    if (!tryGetRequest(request)) return;
    
    // Loop through available handlers until one of them handles the
    // request
    for (HTTP_Handler * handler : handlers)
        if (handler->Process(&request, &response)) break;

    trySendResponse(response);
    
    return;
}
