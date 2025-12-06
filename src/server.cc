#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

#include "server.h"

using boost::asio::ip::tcp;

server::server(boost::asio::io_service& io_service, short port, std::shared_ptr<RequestDispatcher> dispatcher) 
    : io_service_(io_service), 
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      dispatcher_(dispatcher)
  {
    start_accept();
  }

void server::start_accept()
{
    session* new_session = new session(io_service_, dispatcher_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        BOOST_LOG_TRIVIAL(info) << "Successfully accepted connection from " << new_session->socket().remote_endpoint().address().to_string() << ":" << new_session->socket().remote_endpoint().port();
        new_session->start();
    }
    else
    {
        BOOST_LOG_TRIVIAL(warning) << "Error handling accept: " << error.message();
        delete new_session;
    }

    start_accept();
}
