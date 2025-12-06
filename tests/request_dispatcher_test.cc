#include "gtest/gtest.h"
#include "request_dispatcher.h"
#include "config_parser.h"
#include "request_handler_echo.h"
#include "request_handler_file.h"
#include "server_main_utils.h"
#include "handler_registry.h"
#include "handler_factory.h"

class RequestDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    NginxConfigParser parser;
    NginxConfig config;
    RequestDispatcher dispatcher;
};


TEST_F(RequestDispatcherTest, ExtractServerConfigLocationWithNoHandler) {
    const char* config_string = "server { listen 8080; location / { root /data; } }";
    std::istringstream in(config_string);
    ASSERT_TRUE(parser.Parse(&in, &config)); // Use the member parser
    ServerConfig server_config = ExtractServerConfig(config, 80);
    EXPECT_EQ(server_config.port, 8080);
    EXPECT_TRUE(server_config.locations.empty());
}

TEST_F(RequestDispatcherTest, ExtractServerConfigLocationWithOtherDirectives) {
    const char* config_string = "server { listen 8080; location /echo { handler EchoHandler; other_directive value; } }";
    std::istringstream in(config_string);
    ASSERT_TRUE(parser.Parse(&in, &config));
    ServerConfig server_config = ExtractServerConfig(config, 80);
    EXPECT_EQ(server_config.port, 8080);
    ASSERT_EQ(server_config.locations.size(), 1);
    EXPECT_EQ(server_config.locations[0].path, "/echo");
    EXPECT_EQ(server_config.locations[0].handler_name, "EchoHandler");
    EXPECT_EQ(server_config.locations[0].args["other_directive"], "value");
}

TEST_F(RequestDispatcherTest, InitWithEmptyConfig) {
    ServerConfig server_config;
    server_config.port = 8080;
    EXPECT_FALSE(dispatcher.InitFromServerConfig(server_config));
}

TEST_F(RequestDispatcherTest, InitWithUnknownHandler) {
    ServerConfig server_config;
    server_config.port = 8080;
    LocationConfig location;
    location.path = "/foo";
    location.handler_name = "UnknownHandler";
    server_config.locations.push_back(location);
    // Dispatcher no longer validates handlers, so this should succeed
    EXPECT_TRUE(dispatcher.InitFromServerConfig(server_config));
}

TEST_F(RequestDispatcherTest, InitWithFileHandlerWithRoot) {
    ServerConfig server_config;
    server_config.port = 8080;
    LocationConfig location;
    location.path = "/static";
    location.handler_name = "StaticHandler";
    location.args["root"] = "/www";
    server_config.locations.push_back(location);
    
    // Register the handler before initializing dispatcher
    RegisterHandlerForLocation(location);
    EXPECT_TRUE(dispatcher.InitFromServerConfig(server_config));
}

TEST_F(RequestDispatcherTest, FindBestMatchNoMatch) {
    ServerConfig server_config;
    server_config.port = 8080;
    LocationConfig loc1;
    loc1.path = "/echo";
    loc1.handler_name = "EchoHandler";
    server_config.locations.push_back(loc1);
    
    // Register the handler before initializing dispatcher
    RegisterHandlerForLocation(loc1);
    ASSERT_TRUE(dispatcher.InitFromServerConfig(server_config));
    
    // Should return NotFoundHandler, not nullptr
    auto handler = dispatcher.CreateHandler("/unmatched/path");
    ASSERT_NE(handler, nullptr);
}

TEST_F(RequestDispatcherTest, FindBestMatchLongestPrefix) {
    ServerConfig server_config;
    server_config.port = 8080;
    
    LocationConfig loc1, loc2, loc3;
    loc1.path = "/";
    loc1.handler_name = "EchoHandler";
    loc2.path = "/foo";
    loc2.handler_name = "EchoHandler";
    loc3.path = "/foo/bar";
    loc3.handler_name = "EchoHandler";

    server_config.locations.push_back(loc1);
    server_config.locations.push_back(loc2);
    server_config.locations.push_back(loc3);
    
    // Register handlers before initializing dispatcher
    RegisterHandlerForLocation(loc1);
    RegisterHandlerForLocation(loc2);
    RegisterHandlerForLocation(loc3);
    ASSERT_TRUE(dispatcher.InitFromServerConfig(server_config));

    EXPECT_NE(dispatcher.CreateHandler("/"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/a"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/foo"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/foo/"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/foo/bar"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/foo/bar/baz"), nullptr);
    EXPECT_NE(dispatcher.CreateHandler("/foobar"), nullptr);
}

TEST_F(RequestDispatcherTest, GetHandlerNoMatch) {
    ServerConfig server_config;
    server_config.port = 8080;
    LocationConfig loc1;
    loc1.path = "/echo";
    loc1.handler_name = "EchoHandler";
    server_config.locations.push_back(loc1);
    
    // Register the handler before initializing dispatcher
    RegisterHandlerForLocation(loc1);
    ASSERT_TRUE(dispatcher.InitFromServerConfig(server_config));

    // Should return NotFoundHandler, not nullptr
    auto handler = dispatcher.CreateHandler("/unmatched/path");
    ASSERT_NE(handler, nullptr);
}

TEST_F(RequestDispatcherTest, GetHandlerSuccess) {
    ServerConfig server_config;
    server_config.port = 8080;
    
    LocationConfig loc1;
    loc1.path = "/echo";
    loc1.handler_name = "EchoHandler";
    server_config.locations.push_back(loc1);

    LocationConfig loc2;
    loc2.path = "/static";
    loc2.handler_name = "StaticHandler";
    loc2.args["root"] = "public";
    server_config.locations.push_back(loc2);

    // Register handlers before initializing dispatcher
    RegisterHandlerForLocation(loc1);
    RegisterHandlerForLocation(loc2);
    ASSERT_TRUE(dispatcher.InitFromServerConfig(server_config));

    auto echo_handler = dispatcher.CreateHandler("/echo/test");
    ASSERT_NE(echo_handler, nullptr);
    // Dynamic cast to check if it's the correct type of handler
    EchoRequestHandler* casted_echo = dynamic_cast<EchoRequestHandler*>(echo_handler.get());
    EXPECT_NE(casted_echo, nullptr);

    auto file_handler = dispatcher.CreateHandler("/static/index.html");
    ASSERT_NE(file_handler, nullptr);
    FileRequestHandler* casted_file = dynamic_cast<FileRequestHandler*>(file_handler.get());
    EXPECT_NE(casted_file, nullptr);
}