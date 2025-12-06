#include <cstdlib>
#include <iostream>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "request_dispatcher.h"

using boost::asio::ip::tcp;

class session
{
public:
  session(boost::asio::io_service& io_service, std::shared_ptr<RequestDispatcher> dispatcher);

  virtual tcp::socket& socket();

  virtual void start();

  std::string ip = "";
  int port = -1;
protected:
  void handle_read(const boost::system::error_code& error,size_t bytes_transferred);
  void handle_write(const boost::system::error_code& error);

  // DEPRECATED Testable helper methods 
  // static bool IsCompleteHttpRequest(const std::string& buffer);
  // static std::string BuildHttpResponse(const std::string& request_body);
  // static std::string BuildHttpHeader(const std::string& status, 
  //                                    const std::string& content_type,
  //                                    size_t content_length);
  
  // Internal helpers to reduce duplication
  void StartAsyncRead();
  void HandleError();
  void SendResponse();
  void LogResponseMetrics(const std::unique_ptr<response>& resp_obj, 
                          const std::string& request_path,
                          const std::string& handler_name);
  std::string SerializeResponse(const std::unique_ptr<response>& response_obj);

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  std::string request_buffer_;
  std::string response_;
  std::shared_ptr<RequestDispatcher> dispatcher_;
};