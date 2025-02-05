#include "webserver.hpp"
#include <unistd.h>
#include "httprequest.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <signal.h>

unsigned int abort_port = 0;

#define HERE()                                   \
    do                                           \
    {                                            \
        std::cerr << "NEED TO IMPLEMENT HERE\n"; \
    } while (false)

void clean_close(int s)
{
    std::cerr << "Caught signal " << s << ".  Closing and exiting\n";
    if (abort_port != 0)
        close(abort_port);
    exit(1);
}

// You don't need to modify this function.
// This constructor starts up the web server, listening on
// the indicated port.  However, you do need to
// understand this function, as it may inspire items on the
// test.
WebServer::WebServer(unsigned int _port) : port(_port)
{
    if (port == 0)
    {
        // Port should never be 0, but testing code may override
        // other functions for testing purposes so we want to
        // be able to make dummies.
        return;
    }
    // This creates a "socket" (a File descriptor type object)
    // that is used to listen to connections.   By default we use
    // port 8080.
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)))
    {
        std::cerr << "Bind returned an error, error: " << strerror(errno) << "\n";
        exit(-1);
    }
    std::cerr << "Listening For Connection\n";
    if (listen(serverSocket, 10))
    {
        std::cerr << "Listen returned an error, error: " << strerror(errno) << "\n";
        exit(-1);
    }
    abort_port = serverSocket;
    // Register a signal handler so we close the socket cleanly
    // on control-C and a couple other cases
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = clean_close;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGHUP, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
}

WebServer::~WebServer()
{
    std::cerr << "Shutting down webserver\n";
    if (port != 0)
    {
        close(serverSocket);
    }
}

// Again, you do not need to modify this function, but you do need to understand it.

// A limit for testing, can set it to only serve a few pages and quit.
// This is important for e.g. valgrind checking, as valgrind will have
// an opportunity to formally check things only if the program quits normally
// rather than being killed with control-C.
void WebServer::serve(uint64_t pages)
{
    if (port == 0)
    {
        std::cerr << "Not actually running a server, this should not happen\n";
        exit(-1);
    }
    while (pages > 0)
    {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1)
        {
            std::cerr << "Accept returned an error, error: " << strerror(errno) << "\n";
        }
        else
        {
            serveSocket(clientSocket);
        }
        pages--;
    }
}

// And this reads data up until \r\n\r\n, hands it to the constructor
// and then dispatches the result.  Again, you do not need to modify this
// function.
void WebServer::serveSocket(int socket)
{
    const size_t buflen = 10000;
    char buffer[buflen];
    size_t recv_count = 0;
    std::string received = "";
    while (true)
    {
        auto result = recv(socket, buffer + recv_count, buflen - 1 - recv_count, 0);
        if (result == -1)
        {
            std::cerr << "Socket error: " << strerror(errno) << "\n";
            return;
        }
        // Returns 0 if no more data avilable, so break out...
        if (result == 0)
            break;
        // The return value we want to add a null terminator
        recv_count += result;
        buffer[recv_count + 1] = '\0';
        std::string data(buffer, recv_count);
        if (data.find("\r\n\r\n") != data.npos ||
            data.find("\n\n") != data.npos)
        {
            received = data;
            break;
        }
    }
    try
    {
        HTTPRequest request(received);
        HTTPResponder response(socket);
        DispatchResponse(request, response);
    }
    catch (MalformedRequestException &e)
    {
        std::cerr << "Malformed request caught\n";
    }
    close(socket);
}

// Registers a handler function into the system.
void WebServer::RegisterHandler(std::string path, std::function<void(HTTPRequest &r, HTTPResponder &resp)> responder)
{
    handlerFunctions[path] = responder;
}

// This is the first major function YOU need to write.
// You should examine the path.  the path must start with "/", otherwise
// you should respond with a respondError() response and return.

// Then you need to check to see if there is a bound responder for the
// specified item.  It should first check the handlerFuncitons to see if the
// absolute name is in there.  If so, it should call the handlerFunction and
// return.

// If that is not the case it needs to search for a "path responder",
// One bound to the entire path.  So if the request was for "/this/is/a/test",
// it should check for responders for "/this/is/a/test", "/this/is/a/",
// "/this/is/", "/this/", and "/" and use the first one found.

// There will always be A responder for "/" in the configuration.
void WebServer::DispatchResponse(HTTPRequest &request, HTTPResponder &responder)
{
    

    // get path
    std::string path = request.get_resource();
    
    // path must start with "/" and should not be empty
    auto pos = path.find_first_of('/');
    if(pos != 0 || path.empty()){
        respondError(request, responder);
        return;
    }

    //check if path is in handlerFunctions
    auto handler = handlerFunctions.find(path);
    if(handler != handlerFunctions.end()){
        handler -> second(request, responder);
        return;
    }

    //check for path responder and if it is in handlerFunctions
    auto path_responder = path;
    while(!path_responder.empty()){
        auto last = path_responder.find_last_of('/');
        if(last == std::string::npos){
            break;
        }
        path_responder = path_responder.substr(0, last + 1);
        handler = handlerFunctions.find(path_responder);
        if(handler != handlerFunctions.end()){
            handler -> second(request, responder);
            return;
        }
        path_responder = path_responder.substr(0,last);
    }

    // there will always be "/"
    handlerFunctions["/"](request, responder);
}

// You don't need to change this function, but it is
// another one you should understand.
void HTTPResponder::sendResponse(std::string &response, int status)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << status << " \r\n";

    for (auto a : headers)
    {
        out << a.first << ": " << a.second << "\r\n";
    }
    out << "\r\n"
        << response;
    std::string data = out.str();

    auto payload = data.c_str();
    int length = data.length();
    auto sent = send(socket, payload, length, 0);
    if (sent == -1)
    {
        std::cerr << "Send error : " << strerror(errno) << "\n";
    }
    else if (sent < length)
    {
        std::cerr << "HERE needs fixing!" << "\n";
    }
}

// These three accept HTTPRequest objects too
// so they can be directly bound as handlers, as well
// as used standalone.
void dummyHandler(HTTPRequest &r, HTTPResponder &resp)
{
    (void)r;
    std::string payload = dummypayload;
    resp.sendResponse(payload);
}

void respondNotFound(HTTPRequest &r, HTTPResponder &resp)
{
    (void)r;
    std::string payload = "<HTML><HEAD><TITLE>File Not Found!</TITLE><BODY><H3>File Not Found!</H3></BODY></HTML>";
    resp.sendResponse(payload, resp.NOTFOUND);
}

void respondForbidden(HTTPRequest &r, HTTPResponder &resp)
{
    (void)r;
    std::string payload = "<HTML><HEAD><TITLE>FORBIDDEN!</TITLE><BODY><H3>BAD USER, NO ACCESS!</H3></BODY></HTML>";
    resp.sendResponse(payload, resp.FORBIDDEN);
}

void respondError(HTTPRequest &r, HTTPResponder &resp)
{
    (void)r;
    std::string payload = "<HTML><HEAD><TITLE>File Not Found!</TITLE><BODY><H3>File Not Found!</H3></BODY></HTML>";
    resp.sendResponse(payload, resp.BADREQUEST);
}

// This is the helper function used below to set the
// "Content-Type" header field that you need to write.
//
// It should examine the file extension of the filename itself, so it
// should first take the end of the string to extract the file
// (e.g. /aoeu/aoet.eu/fubar.baz) has fubar.baz as the filename.
//
// Then, if there is an extension (in this case, .baz), you turn
// it into the appropriate mimetype.
//
// This process is case insensitive, so .html, .htML, and .HTML are
// all treated as .html.
//
// You should follow the guidelines here:
// https://developer.mozilla.org/en-US/docs/Web/HTTP/MIME_types/Common_types
// for the exact mapping.
//
// You need to support the following extensions, and the matching
// should be case insensitive (that is, a file ending in .html, .htML, and .hTML is
// all the same mimetype of "text/html"):
// .htm
// .html
// .gif
// .jpg
// .png
// .svg
// .txt
// For all other extensions, return the string "application/octet-stream".

// A hint:  You can create a static map of standard strings by
// std::map<std::string, std::string> foo = {{"a", "one"}, {"b", "two"}};

// function to make string lowercase
std::string tolower(std::string &s){
        for(auto &c: s){
            c = tolower(c);
        }
        return s;
}
std::string mimetype(std::string filename)
{
    // map with all the extensions
    std::map<std::string, std::string> mimetypes = {
        {".htm", "text/html"},
        {".html", "text/html"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".svg", "image/svg+xml"},
        {".txt", "text/plain"},
    };

    // get extension
    auto extension_pos = filename.find_last_of('.');
    auto extension = filename.substr(extension_pos);

    // find and return extension
    extension = tolower(extension);
    auto find_extension = mimetypes.find(extension);
    if(find_extension != mimetypes.end()){
        return(find_extension -> second);
    }

    return("application/octet-stream");

}

// This is the final major function you need to write.

// When created it takes a "path", a starting point to the initial files, and should return
// a lambda that takes an HTTPRequest & and HTTPResponder& reference to process requests.

// For processing the requests, if the request ENDS with "/", you must append the name "index.html" onto the filename.

// If the request contains the exact string "/../" anywhere in it you MUST instead do responseForbidden.

// You should then check if the full path to the file exists using std::filesystem::exists.
// If the file doesn't exist do respondNotFound and return.

// Otherwise, load the file as a binary blob into a std::string.  The easiest way to do this
// is open the file as an ifstream() in std::ifstream::binary mode.  If the ifstream is not
// good you should then do respondNotFound and return

// Then you can create a stringstream as a buffer, and read in all the data and then get
// the std::string from the stringstream...

std::function<void(HTTPRequest &, HTTPResponder &)> generateFileResponder(std::string path)
{
    return [=](HTTPRequest &request, HTTPResponder &responder)
    {
        
        //get path
        auto Path = path + request.get_resource();
        
        //check if path ends with /
        if (Path.back() == '/'){
            Path += "index.html";
        }
        // check if path has /../
        if(Path.find("/../") != std::string::npos){
            respondForbidden(request, responder);
            return;
        }
         // check if file exists
        if(!std::filesystem::exists(Path)){
            respondNotFound(request, responder);
            return;
        }
        //open file, check if input is good
        std::ifstream input{Path, std::ifstream::binary};
        if(!input.good()){
            respondNotFound(request, responder);
            return;
        }
        
        //load file into std::stringstream 
        std::stringstream buf;
        buf << input.rdbuf();
        std::string file = buf.str();

        responder.headers["Content-Type"] = mimetype(Path);
        responder.sendResponse(file);
    };
}