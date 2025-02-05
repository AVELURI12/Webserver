#include <gtest/gtest.h>

#include "httprequest.hpp"

// Well, this test actually isn't that simple...
// It tests that the header parsing is correct:
// the key should be lower case, but the value 
// should be as is but trimmed for whitespace (isspace)
TEST(HTTPRequestTest, TestSimple) {
  HTTPRequest request("GET / HTTP/1.1\r\nHoST  : www.fubar.com\t\tUgh\t\r\n\r\n");
  EXPECT_EQ(request.get_command(), "GET");
  EXPECT_EQ(request.get_headers()["host"]->get_value(), "www.fubar.com\t\tUgh");
  EXPECT_TRUE(request.get_headers().contains("host"));
  EXPECT_FALSE(request.get_headers().contains("HoST"));
}

TEST(HTTPRequestTest, TestPayload) {
  HTTPRequest r("POST /fubar HTTP/1.1\r\nHost: aoeaoeu\r\n\r\nPayload!");
  EXPECT_EQ(r.get_command(), "POST");
  EXPECT_EQ(r.get_resource(), "/fubar");
  EXPECT_EQ(r.get_payload(), "Payload!");
  HTTPRequest s("POST /fubar HTTP/1.1\r\nHost: aoeaoeu\r\n\r\nPayload!  \r\n\r\n\r\nhey!");
  EXPECT_EQ(s.get_payload(), "Payload!  \r\n\r\n\r\nhey!");
}

TEST(HTTPRequestTest, TestExceptions){
  EXPECT_THROW(HTTPRequest(""), MalformedRequestException) << "Must have at least a command";
  EXPECT_THROW(HTTPRequest("POST / HTTP/1.1\r\nHost: aoeaoeu\r\n\r"), MalformedRequestException) 
    << "Needs to end with \r\n\r\n";
  EXPECT_THROW(HTTPRequest("GET / HTTp/1.1\r\nHost: aoeaoeu\r\n\r\n"), MalformedRequestException) 
    << "Must be HTTP/1.1 exactly";
  EXPECT_THROW(HTTPRequest("GeT / HTTP/1.1\r\nHost: aoeaoeu\r\n\r\n"), MalformedRequestException) 
    << "Must be recognized command (GET/POST/HEAD only)";
  EXPECT_THROW(HTTPRequest("GET  / HTTP/1.1\r\nHost: aoeaoeu\r\n\r\n"), MalformedRequestException) 
    << "Only one space separating components in the command!";
  EXPECT_THROW(HTTPRequest("GET /  HTTP/1.1\r\nHost: aoeaoeu\r\n\r\n"), MalformedRequestException) 
    << "Only one space separating!";
  EXPECT_THROW(HTTPRequest("GET / HTTP/1.1\r\nHost: aoeaoeu\r\nhost: aoeu\r\n\r\n"), MalformedRequestException) 
    << "No duplicate headers!";
}