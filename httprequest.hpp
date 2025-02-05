#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H

#include <map>
#include <memory>

// This exception is raised for any time things
// fail to parse correctly.  You don't have to 
// have the string be "meaningful", but you can
class MalformedRequestException: public std::exception {
public:
    MalformedRequestException(std::string m) {
        message = "Malformed HTTP Request: " + m;
    }

    MalformedRequestException(){
        message = "Malformed HTTP Request";
    }
    
    // A "lovely" bit of C++ism.  This is safe as it is
    // used (get a view from a live exception object), but
    // the lifespan of this string can not exceed the lifespan
    // of the exception object itself, and being a pointer you
    // could see some other programmer keeping it around.  
    
    // Plus, also, its a C-style string not a proper string.
    // This is, once again, a legacy issue.  C++ didn't add a
    // proper string type for 2 decades, so a lot of code in
    // C++ is set to use C strings.
    virtual const char *view(){
        return message.c_str();
    }

private:
    std::string message;
};

// This is the class for an HTTP header, as a single entry.
// A header has two fields, the name and the value.  For both
// these should be read-only fields.  Unfortunately this means that
// C++'s vaunted efficiency is once again face-planting here:
// because it is copying the strings needlessly...
class HTTPHeader {
    public:
    HTTPHeader(std::string s);

    const std::string get_name(){return name;}
    const std::string get_value(){return value;}

    private:
    std::string name;
    std::string value;
};

// And this is the class for an HTTP request itself.
// The constructor accepts the string, and it has four fields of note:
// the command, the resource being accessed, the payload, and the headers.
//
// The headers is a std::map that maps strings (header names) to the header fields.
class HTTPRequest {
    public:
    HTTPRequest(std::string s);

    // This has to be a reference because we don't want to copy the entire
    // map.  It should not be modified by whoever calls this.
    
    // Unfortunately we can't return it as "const" because we want the
    // [] operator to work and the [] operator doesn't work on const-declared
    // maps because the compiler can't distinguish between [] for setting and 
    // [] for getting.
    std::map<std::string, std::shared_ptr<HTTPHeader>> & get_headers() {return headers;}

    const std::string get_command(){return command;}
    const std::string get_resource(){return resource;}

    // Likewise, the same for payload here, because the payload can be big.
    const std::string &get_payload(){return payload;}

    // For both this is safe in practice but not in theory:  In theory this
    // means that external data (the map and payload) can escape the control of
    // the shared_ptr reference counting.  But in practice because the references are
    // used right away and not kept around (unlike how pointers often are),
    // a programmer would really really have to try to have this escape from the
    // lifespan of the enclosing object.


    private:
    std::map<std::string, std::shared_ptr<HTTPHeader>> headers;
    std::string command;
    std::string resource;
    std::string payload;

    void parse_command(std::string &command_string);
};

#endif
