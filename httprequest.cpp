
#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <ranges>

#include "httprequest.hpp"

// This is the heart of the HTTP parsing.

// You should first take the request string and break it up
// into substrings wherever there is a '\r\n' pair.  The 
// easiest way to do this is to iterate through the entire input,
// keeping track of the current character, where in the string you are
// and the last character. When you get a '\n' when the previous was '\r',
// you have found the end of the substring (well, 2 past the end 
// the substring), allowing you to use std::string's substr function.
//
// But at the same time you shouldn't ever have a '\n' without a prior '\r', or 
// a '\r' without a subsequent '\n', however this is something we are not going to
// require you to check for.
// 
// (Note that substr takes the starting index and the length, so you will
// want to keep track of the current index, the last start point, and possibly
// the length as well).

// Finally, if you have '\r\n\r\n' in a row that is the end of the headers and you
// can use a break to exit the loop.  This ending must be present and if it isn't you
// should raise a MalformedRequestException.  

// Anything past the \r\n\r\n is the "payload".

// Now that it is split up, you can then parse both the command (the first part)
// and then the headers.


HTTPRequest::HTTPRequest(std::string request){
  char last = '\0';
  unsigned int length = 0;
  unsigned int last_index = 0;
  unsigned int index = 0;
  std::vector<std::string> substrings;
  bool all_headers = false;
  for(auto c: request){
    if(c == '\n' && last == '\r'){
      auto segment = request.substr(last_index, length-1);
      // If there are two \r\n\r\n it is the end of the
      // request and just the payload
      if (segment == "") {
        payload = request.substr(last_index+2);
        all_headers = true;
        break;
      }
      substrings.push_back(segment);
      last_index = index + 1;
      length = -1; // Because we end up incrementing it below
      // and we want to skip things...
    }
    last = c;
    index += 1;
    length += 1;
  }

  // Now that things are broken up into the lines its time to do the real parsing...
  if(substrings.empty()){
    throw MalformedRequestException("No headers!");
  }
  if(!all_headers){
    throw MalformedRequestException("No terminating \\r\\n\\r\\n");
  }

  parse_command(substrings.at(0));

  for(int i = 1; i < (int) substrings.size(); ++i){
    auto header = std::make_shared<HTTPHeader>(substrings.at(i));
    if (headers.contains(header->get_name())) {
      throw MalformedRequestException("Duplicate headers!");
    }
    headers[header->get_name()] = header;
  }

}

auto valid_commands = {"GET", "POST", "HEAD"};

void HTTPRequest::parse_command(std::string &command_string){
  auto space = command_string.find_first_of(" ");
  auto space2 = command_string.find_first_of(" ", space + 1);
  auto space3 = command_string.find_first_of(" " , space2 + 1);
  bool valid_command = false;
  if (space == std::string::npos || space2 == std::string::npos || space3 != std::string::npos) {
    throw MalformedRequestException("Command is not correct!");
  }
  command = command_string.substr(0,space);
  resource = command_string.substr(space+1, space2-space-1);
  for (auto cmd : valid_commands){
    if(cmd == command) {
      valid_command = true;
    }
  }
  if (!valid_command) {throw MalformedRequestException("Invalid command!");}
  auto protocol = command_string.substr(space2+1, space3-space2-1);
  if (protocol != "HTTP/1.1") {throw MalformedRequestException("Unknown Protocol!");}
}

// Here is a cute little utility function
// that you will probably both want and want to
// understand.  In it you can see us overloading
// "tolower"
std::string tolower(std::string s){
  for(char &c : s){
    c = tolower(c);
  }
  return s;
}

// This function should trim whitespace (all characters that
// are isspace(c) from both the front and the END of the string,
// but NOT remove any space from the middle.
std::string trim(std::string s){
  unsigned int startat = 0;
  for(auto c : s){
    if(!isspace(c)) break;
    startat += 1;
  }
  s = s.substr(startat);
  auto len = s.length();
    for (auto c : s | std::views::reverse ){
        if(!isspace(c)) {
            break;
        }
        len--;
    }
  return s.substr(0, len);
}

// This is the portion for a header.  The name should be the
// header name (the part before the :) in all lower case, and
// should have no whitespace (no character matiching C's is_space)
// The value is ALL the data after the :, including any whitespace!
HTTPHeader::HTTPHeader(std::string s){
  auto colon = s.find_first_of(":");
  if(colon == std::string::npos) {
    throw MalformedRequestException("Bad Header!");
  }
  name = trim(tolower(s.substr(0, colon)));
  value = trim(s.substr(colon+1));
  for (auto c: name){
    if(isspace(c)) {
      throw MalformedRequestException("No whitespace in header name!");
      }
  }
}