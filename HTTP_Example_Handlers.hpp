#ifndef HTTP_EXAMPLE_HANDLERS
#define HTTP_EXAMPLE_HANDLERS

#include "HTTP_Server.hpp"

// Handler to return a file in the servers local running
// directory.
class HTTP_File_Handler : public HTTP_Handler {
  bool Process(HTTP_Request* request, HTTP_Response* response) {
    if (request->method == "GET") {
    
      string file = "." + request->requestURI;
      string line;
      ifstream myfile(file);

      if (myfile.is_open()) {
        while (getline(myfile,line)) {
          response->body += line + "\n";
        }
        myfile.close();
      }
      else {
        return false;
      }
      
      return true;
    }
    
    if (request->method == "PUT" || "POST") {
      istringstream iss(request->body);
      string filePath = "." + request->requestURI;
      ofstream ofs(filePath);
      string line;
      while (getline(iss,line)) {
        ofs << line << "\n";
      }

      // Make sure to have a non-zero response body
      response->body = "done!";

      return true;
    }

    // If method wat not "GET", "PUT" or "POST" don't handle
    return false;
  }
};

// Prints info about a request but never handles the request.
class PrintInfoHandler : public HTTP_Handler {
  bool Process(HTTP_Request* request, HTTP_Response* response) {
      
    cout << "method: " << request->method << endl;
    cout << "requestURI: " << request->requestURI << endl;
    cout << "httpVersion: " << request->httpVersion << endl;

    for (auto p : request->headers) {
      cout << p.first << ": " << p.second << endl;
    }

    cout << request->body << endl;
      
    return false;
  }
};


#endif
