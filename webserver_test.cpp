#include <gtest/gtest.h>

#include "webserver.hpp"

class MockHTTPResponder : public HTTPResponder
{
public:
    std::string payload;
    int responseCode;

    virtual void sendResponse(std::string &response, int status = HTTPResponder::OK)
    {
        payload = response;
        responseCode = status;
    }
    MockHTTPResponder() : HTTPResponder(0) {}
};

std::string requestStart = "GET /";
std::string requestEnd = " HTTP/1.1\r\nHost: localhost:8080\r\nAccept: Everything\r\n\r\n";

class MockWebServer : public WebServer
{
public:
    std::vector<std::shared_ptr<MockHTTPResponder>> responses;
    MockWebServer() : WebServer(0)
    {
    }

    std::shared_ptr<MockHTTPResponder> testWithRequest(std::string request){
        return testServe(requestStart + request + requestEnd);
    }

    std::shared_ptr<MockHTTPResponder> testServe(std::string received)
    {
        try
        {
            HTTPRequest request(received);
            auto responder = std::make_shared<MockHTTPResponder>();
            DispatchResponse(request, *responder);
            return responder;
        }
        catch (MalformedRequestException &e)
        {
            return nullptr;
        }
    }
};

TEST(WebserverTests, TestBasic)
{
    MockWebServer server;
    server.RegisterHandler("/dummy", dummyHandler);
    server.RegisterHandler("/dummypath/is/great/", dummyHandler);
    server.RegisterHandler("/", generateFileResponder("../webcontent"));
    auto response = server.testWithRequest("dummy"); 
    //ASSERT_NE(response, nullptr); i added this
    EXPECT_EQ(response->responseCode, 200);
    EXPECT_EQ(response->payload, dummypayload );
    response = server.testWithRequest("");
    EXPECT_EQ(response->responseCode, 200);
    EXPECT_TRUE(response->payload.find("Webslobber") != std::string::npos);
    EXPECT_EQ(response->headers["Content-Type"], "text/html");
}
