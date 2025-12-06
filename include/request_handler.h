#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>
#include <vector>

struct request {
  bool is_valid;
  std::string raw_request;
  std::string method;
  std::string url;
  std::string http_version;
  std::vector<std::string> headers;
  std::string body;
};

struct response {
  std::string http_version;
  std::string status_code;
  std::string content_type;
  std::string content_length;
  std::vector<std::string> headers;
  std::string body;
};

class RequestHandler {
  public:
    virtual std::unique_ptr<response> handle_request(const request& request) = 0;
    virtual std::string GetHandlerName() const = 0;
    virtual ~RequestHandler() = default;
};

#endif