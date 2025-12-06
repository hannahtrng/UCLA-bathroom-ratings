//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>

#include "server.h"
#include "config_parser.h"
#include "server_main_utils.h"

using boost::asio::ip::tcp;
using namespace std; // For atoi.

int main(int argc, char* argv[])
{
  try
  {
    writeLogToConsole();
    writeLogToFile("../logs");
    
    // Initialize everything from command line arguments
    auto init_result = initialize_from_args(argc, argv);
    if (!init_result.is_valid()) {
      // Error already logged by initialize_from_args
      return 1;
    }

    auto dispatcher = init_result.dispatcher;
    short port = init_result.port;

    boost::asio::io_service io_service;

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code& errorCode, int signalReceived){
      if (!errorCode) {
        BOOST_LOG_TRIVIAL(info) << "Terminating since received Ctrl + C:" ;
        io_service.stop();
      }
    });

    server s(io_service, port, dispatcher);
    
    std::cout << "Listening on port: " << port << std::endl;
    BOOST_LOG_TRIVIAL(info) << "Server is listening on port: " << port;

    // create a thread pool and run io_service on each thread
    std::vector<std::thread> threads;
    // Use hardware_concurrency to determine the number of threads
    unsigned int thread_count = std::thread::hardware_concurrency();
    BOOST_LOG_TRIVIAL(info) << "Creating a thread pool with " << thread_count << " threads.";
    for (unsigned int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&io_service](){
            io_service.run();
        });
    }

    // Wait for all threads in the pool to exit.
    for (std::thread& t : threads) {
        t.join();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    BOOST_LOG_TRIVIAL(error) << "Caught error: " << e.what();
  }

  return 0;
}
