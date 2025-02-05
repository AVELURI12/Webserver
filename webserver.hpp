#include <iostream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "httprequest.hpp"
#include <functional>

// const std::string hello_response = "HTTP/1.1 400 OK\r\nContent-Type: text/HTML\r\nConnection: close\r\n\r\n<HTML><HEAD><TITLE>Hello World</TITLE><BODY><H3>Hello World</H3></BODY></HTML>";

const std::string dummypayload = "<HTML><HEAD><TITLE>Hello world!</TITLE><BODY><H3>Hello World!</H3></BODY></HTML>";

class HTTPResponder;

class WebServer
{
public:
    const unsigned int port;
    WebServer(unsigned int _port = 0);

    ~WebServer();

    // Virtual so we can have a test version that overwrites and doesn't actually send any information.
    // If port == 0 it doesn't actually open a socket on the constructor so this version
    // will be OK.
    virtual void serve(uint64_t pages = 0xFFFFFFFFFFFFFFFF);

    // This is the public function used to register handlers.  Handlers are functions
    // that take an HTTP request object and an HTTP Responder object.  The request is the
    // request as sent to the user.  The responder is an object that can be used to send
    // the response.
    void RegisterHandler(std::string path, std::function<void(HTTPRequest &, HTTPResponder &)> f);

protected:
    int serverSocket;
    struct sockaddr_in serverAddress;

    void serveSocket(int socket);
    void DispatchResponse(HTTPRequest &request, HTTPResponder &responder);

    std::map<std::string, std::function<void(HTTPRequest &, HTTPResponder &)>> handlerFunctions;
};

class HTTPResponder
{

public:
    // This is const so although its public it doesn't matter.
    const int socket;

    // Major HTTP response codes.
    static const int OK = 200;
    static const int FORBIDDEN = 403;
    static const int NOTFOUND = 404;
    static const int BADREQUEST = 400;

    HTTPResponder(int _socket) : socket(_socket)
    {
        // For now we are always closing the connection, so we
        // will always set the "Connection" header to "Close"
        headers["Connection"] = "Close";
    }

    // And this is a map of extra headers, allowing handlers to
    // set various fields.  In particular one important one is
    // "Content-Type", which specifies what type of data is being
    // returned.

    std::map<std::string, std::string> headers;

    // Virtual so we can do a test version that doesn't actually send/receive data
    // for unit testing as well.
    virtual void sendResponse(std::string &response, int status = HTTPResponder::OK);

private:
};

// A couple of dummy handler functions.  This first one is a hello world...
void dummyHandler(HTTPRequest &r, HTTPResponder &resp);

// And these will return errors or not found depending.
// They can also be used manually to send particular error codes.
void respondError(HTTPRequest &r, HTTPResponder &resp);
void respondNotFound(HTTPRequest &r, HTTPResponder &resp);
void responseForbidden(HTTPRequest &r, HTTPResponder &resp);

// And this is a generator function for generating file responders.
std::function<void(HTTPRequest &, HTTPResponder &)> generateFileResponder(std::string path);

std::string mimetype(std::string filename);
