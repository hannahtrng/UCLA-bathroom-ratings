#include <cstdlib>
#include <iostream>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "session.h"
#include "request_dispatcher.h"

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_service& io_service, short port, std::shared_ptr<RequestDispatcher> dispatcher);

protected:
  void start_accept();
  void handle_accept(session* new_session,const boost::system::error_code& error);

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  std::shared_ptr<RequestDispatcher> dispatcher_;
};