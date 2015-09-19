#ifndef HTTP_EXAMPLE_HANDLERS
#define HTTP_EXAMPLE_HANDLERS

#include "HTTP_Server.hpp"

// Handler to return a file in the servers local running
// directory.
class HTTP_File_Handler : public HTTP_Handler {
  bool Process(HTTP_Request* request, HTTP_Response* response) {
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
};  

#endif
