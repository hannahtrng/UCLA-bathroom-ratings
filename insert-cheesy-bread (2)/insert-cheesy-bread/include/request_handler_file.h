#ifndef FILE_REQUEST_HANDLER_H
#define FILE_REQUEST_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>
#include "request_handler.h"

class FileRequestHandler : public RequestHandler {
  public:
    FileRequestHandler(const std::string& base_dir, const std::string& url_prefix = "")
      : base_dir_(base_dir), url_prefix_(url_prefix) {}
    std::unique_ptr<response> handle_request(const request& request) override;
    std::string GetHandlerName() const override;

  private:
    std::string base_dir_;
    std::string url_prefix_;
    
    std::string read_file(const std::string& filepath);
    std::string get_content_type(const std::string& filepath);
};

#endif // FILE_REQUEST_HANDLER_H