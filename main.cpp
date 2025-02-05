#include <iostream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "webserver.hpp"

int main(int argc, char **argv)
{
  // Technically we don't serve "forever", but this is close 
  // enough.  We use the first argument to override this so
  // we can quit early for things like leak checking through valgrind.

  uint64_t servecount = 0xFFFFFFFFFFFFFFFF;
  if (argc > 1 ){
    servecount = (uint64_t) std::atol(argv[1]);
  }
  std::cout << "Starting up ECS 36B webserver\n";
  if (argc > 1) {
    std::cout << "Limiting lifetime to " << servecount << " pages before quitting\n";
  }
  WebServer server(8080);
  server.RegisterHandler("/dummy", dummyHandler);
  server.RegisterHandler("/dummypath/is/great/", dummyHandler);
  server.RegisterHandler("/", generateFileResponder("../webcontent"));
  server.serve(servecount);
  return 0;
}
