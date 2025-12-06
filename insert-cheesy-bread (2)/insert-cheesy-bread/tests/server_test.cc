#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <boost/asio.hpp>

#include "server.h"
#include "request_dispatcher.h"
#include "server_config.h"
#include "handler_registry.h"
#include "handler_factory.h"

using boost::asio::ip::tcp;
using ::testing::_;

class MockSession: public session {
  public:
    MockSession(boost::asio::io_service& io_service, std::shared_ptr<RequestDispatcher> dispatcher) 
        : session(io_service, dispatcher) {}
    MOCK_METHOD(void, start, (), (override));

};

// make methods accesible (public)
class TestServer : public server {
  public:
    using server::server;
    using server::handle_accept;
};

class ServerTest : public ::testing::Test {
  protected:
    boost::asio::io_service io_service_;
    TestServer* server_ = nullptr;
    std::shared_ptr<RequestDispatcher> dispatcher_;
    const int port_ = 8080;

    void SetUp() override {
      // Handlers are registered automatically when LocationConfigs are processed
      
      dispatcher_ = std::make_shared<RequestDispatcher>();
      
      ServerConfig test_config;
      test_config.port = port_;
      
      LocationConfig echo_location;
      echo_location.path = "/";
      echo_location.handler_name = "EchoHandler";
      test_config.locations.push_back(echo_location);
      
      dispatcher_->InitFromServerConfig(test_config);
      
      server_ = new TestServer(io_service_, port_, dispatcher_);
    }
    void TearDown() override {
      delete server_;
      server_ = nullptr;
    }
};

// TEST_F(ServerTest, ConstructorWorks) {
//   ASSERT_NE(server_, nullptr);
// }

// TEST_F(ServerTest, CallsStartOnce) {
//   auto* mockSession = new MockSession(io_service_, dispatcher_);
//   EXPECT_CALL(*mockSession, start()).Times(1); // check no more no less than 1
//   server_->handle_accept(mockSession, boost::system::error_code{}); // acts if handle accept is successful, so should call start
//   delete mockSession;
// }

// // Test error path in handle_accept (targets line 31)
// // Note: We can't use MockSession here because handle_accept deletes it,
// // which causes issues with GMock expectations. Instead, we use a real session.
// TEST_F(ServerTest, HandleAcceptWithError_DeletesSession) {
//   // Create a real session (not a mock)
//   session* real_session = new session(io_service_, dispatcher_);
  
//   // Create an error code (connection aborted)
//   boost::system::error_code error = boost::asio::error::connection_aborted;
  
//   // Call handle_accept with error - session should be deleted inside
//   // We can't verify much here except that it doesn't crash
//   server_->handle_accept(real_session, error);
  
//   // Note: We don't delete real_session here because handle_accept already did
//   // The test passes if no crash/double-free occurs
// }

// // Test handle_accept with operation_aborted error
// TEST_F(ServerTest, HandleAcceptWithOperationAborted) {
//   session* real_session = new session(io_service_, dispatcher_);
//   boost::system::error_code error = boost::asio::error::operation_aborted;
  
//   server_->handle_accept(real_session, error);
//   // Session deleted by handle_accept - test passes if no crash
// }

// // Test handle_accept with connection_reset error
// TEST_F(ServerTest, HandleAcceptWithConnectionReset) {
//   session* real_session = new session(io_service_, dispatcher_);
//   boost::system::error_code error = boost::asio::error::connection_reset;
  
//   server_->handle_accept(real_session, error);
//   // Session deleted by handle_accept - test passes if no crash
// }

// // Test multiple successful accepts
// TEST_F(ServerTest, MultipleSuccessfulAccepts) {
//   // First accept
//   auto* session1 = new MockSession(io_service_, dispatcher_);
//   EXPECT_CALL(*session1, start()).Times(1);
//   server_->handle_accept(session1, boost::system::error_code{});
//   delete session1;
  
//   // Second accept
//   auto* session2 = new MockSession(io_service_, dispatcher_);
//   EXPECT_CALL(*session2, start()).Times(1);
//   server_->handle_accept(session2, boost::system::error_code{});
//   delete session2;
// }

// // Test alternating success and error accepts
// TEST_F(ServerTest, AlternatingSuccessAndError) {
//   // Successful accept with mock
//   auto* session1 = new MockSession(io_service_, dispatcher_);
//   EXPECT_CALL(*session1, start()).Times(1);
//   server_->handle_accept(session1, boost::system::error_code{});
//   delete session1;
  
//   // Error accept with real session (can't use mock due to deletion)
//   session* session2 = new session(io_service_, dispatcher_);
//   boost::system::error_code error = boost::asio::error::connection_aborted;
//   server_->handle_accept(session2, error);
//   // session2 deleted by handle_accept
  
//   // Another successful accept with mock
//   auto* session3 = new MockSession(io_service_, dispatcher_);
//   EXPECT_CALL(*session3, start()).Times(1);
//   server_->handle_accept(session3, boost::system::error_code{});
//   delete session3;
// }

// // Test multiple consecutive errors
// TEST_F(ServerTest, MultipleConsecutiveErrors) {
//   // First error
//   auto* session1 = new session(io_service_, dispatcher_);
//   boost::system::error_code error1 = boost::asio::error::connection_aborted;
//   server_->handle_accept(session1, error1);
  
//   // Second error
//   auto* session2 = new session(io_service_, dispatcher_);
//   boost::system::error_code error2 = boost::asio::error::network_down;
//   server_->handle_accept(session2, error2);
  
//   // Third error
//   auto* session3 = new session(io_service_, dispatcher_);
//   boost::system::error_code error3 = boost::asio::error::host_unreachable;
//   server_->handle_accept(session3, error3);
// }

// // Test with different port numbers
// TEST(ServerConstructorTest, DifferentPorts) {
//   boost::asio::io_service io1, io2, io3;
//   auto dispatcher = std::make_shared<RequestDispatcher>();
//   
//   ServerConfig test_config;
//   test_config.port = 8080;
//   LocationConfig echo_location;
//   echo_location.path = "/";
//   echo_location.handler_name = "EchoHandler";
//   test_config.locations.push_back(echo_location);
//   dispatcher->InitFromServerConfig(test_config);
  
//   // Test with port 80
//   TestServer* s1 = new TestServer(io1, 80, dispatcher);
//   EXPECT_NE(s1, nullptr);
//   delete s1;
  
//   // Test with port 443
//   TestServer* s2 = new TestServer(io2, 443, dispatcher);
//   EXPECT_NE(s2, nullptr);
//   delete s2;
  
//   // Test with port 8080
//   TestServer* s3 = new TestServer(io3, 8080, dispatcher);
//   EXPECT_NE(s3, nullptr);
//   delete s3;
// }

// // Test error followed by multiple successes
// TEST_F(ServerTest, ErrorFollowedByMultipleSuccess) {
//   // Error first
//   auto* errorSession = new session(io_service_, dispatcher_);
//   boost::system::error_code error = boost::asio::error::connection_refused;
//   server_->handle_accept(errorSession, error);
  
//   // Then successes
//   for (int i = 0; i < 3; i++) {
//     auto* s = new MockSession(io_service_, dispatcher_);
//     EXPECT_CALL(*s, start()).Times(1);
//     server_->handle_accept(s, boost::system::error_code{});
//     delete s;
//   }
// }

