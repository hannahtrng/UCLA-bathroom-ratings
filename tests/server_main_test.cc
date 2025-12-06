#include "gtest/gtest.h"
#include "config_parser.h"
#include "server_main_utils.h"
#include "server_config.h"
#include "request_dispatcher.h"
#include "handler_registry.h"
#include "handler_factory.h"
#include <memory>
#include <fstream>
#include <sstream>

class ServerMainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Handlers are registered automatically when LocationConfigs are processed
    }
    
    NginxConfig config;
    
    // Helper to add a location block with echo handler
    void AddEchoLocation(NginxConfig* server_block, const std::string& path) {
        auto location_stmt = std::make_unique<NginxConfigStatement>();
        location_stmt->tokens_.push_back("location");
        location_stmt->tokens_.push_back(path);
        location_stmt->child_block_ = std::make_unique<NginxConfig>();
        
        auto handler_stmt = std::make_unique<NginxConfigStatement>();
        handler_stmt->tokens_.push_back("handler");
        handler_stmt->tokens_.push_back("EchoHandler");
        location_stmt->child_block_->statements_.emplace_back(std::move(handler_stmt));
        
        server_block->statements_.emplace_back(std::move(location_stmt));
    }
};

// Test extract_server_config returns default port when no listen directive
TEST_F(ServerMainTest, NoListenDirective_ReturnsDefaultPort) {
    auto server_stmt = std::make_unique<NginxConfigStatement>();
    server_stmt->tokens_.push_back("server");
    server_stmt->child_block_ = std::make_unique<NginxConfig>();
    
    AddEchoLocation(server_stmt->child_block_.get(), "/");
    config.statements_.emplace_back(std::move(server_stmt));
    
    ServerConfig server_config = ExtractServerConfig(config, 8080);
    EXPECT_EQ(server_config.port, 8080);
    EXPECT_EQ(server_config.locations.size(), 1);
}

// Test extract_server_config returns custom default port
TEST_F(ServerMainTest, NoListenDirective_ReturnsCustomDefaultPort) {
    auto server_stmt = std::make_unique<NginxConfigStatement>();
    server_stmt->tokens_.push_back("server");
    server_stmt->child_block_ = std::make_unique<NginxConfig>();
    
    AddEchoLocation(server_stmt->child_block_.get(), "/");
    config.statements_.emplace_back(std::move(server_stmt));
    
    ServerConfig server_config = ExtractServerConfig(config, 9000);
    EXPECT_EQ(server_config.port, 9000);
    EXPECT_EQ(server_config.locations.size(), 1);
}

// Test extract_server_config extracts port from listen directive
TEST_F(ServerMainTest, WithListenDirective_ExtractsPort) {
    auto server_stmt = std::make_unique<NginxConfigStatement>();
    server_stmt->tokens_.push_back("server");
    server_stmt->child_block_ = std::make_unique<NginxConfig>();
    
    auto listen_stmt = std::make_unique<NginxConfigStatement>();
    listen_stmt->tokens_.push_back("listen");
    listen_stmt->tokens_.push_back("3000");
    server_stmt->child_block_->statements_.emplace_back(std::move(listen_stmt));
    
    AddEchoLocation(server_stmt->child_block_.get(), "/");
    config.statements_.emplace_back(std::move(server_stmt));
    
    ServerConfig server_config = ExtractServerConfig(config, 8080);
    EXPECT_EQ(server_config.port, 3000);
    EXPECT_EQ(server_config.locations.size(), 1);
}

// Test extract_server_config uses first server block when multiple exist
TEST_F(ServerMainTest, MultipleServerBlocks_UsesFirstOne) {
    // First server block with port 5000
    auto server_stmt1 = std::make_unique<NginxConfigStatement>();
    server_stmt1->tokens_.push_back("server");
    server_stmt1->child_block_ = std::make_unique<NginxConfig>();
    
    auto listen_stmt1 = std::make_unique<NginxConfigStatement>();
    listen_stmt1->tokens_.push_back("listen");
    listen_stmt1->tokens_.push_back("5000");
    server_stmt1->child_block_->statements_.emplace_back(std::move(listen_stmt1));
    AddEchoLocation(server_stmt1->child_block_.get(), "/");
    
    // Second server block with port 6000
    auto server_stmt2 = std::make_unique<NginxConfigStatement>();
    server_stmt2->tokens_.push_back("server");
    server_stmt2->child_block_ = std::make_unique<NginxConfig>();
    
    auto listen_stmt2 = std::make_unique<NginxConfigStatement>();
    listen_stmt2->tokens_.push_back("listen");
    listen_stmt2->tokens_.push_back("6000");
    server_stmt2->child_block_->statements_.emplace_back(std::move(listen_stmt2));
    AddEchoLocation(server_stmt2->child_block_.get(), "/");
    
    config.statements_.emplace_back(std::move(server_stmt1));
    config.statements_.emplace_back(std::move(server_stmt2));
    
    ServerConfig server_config = ExtractServerConfig(config, 8080);
    EXPECT_EQ(server_config.port, 5000);
}

// Test extract_server_config extracts standard ports like 80
TEST_F(ServerMainTest, ExtractsPort80) {
    auto server_stmt = std::make_unique<NginxConfigStatement>();
    server_stmt->tokens_.push_back("server");
    server_stmt->child_block_ = std::make_unique<NginxConfig>();
    
    auto listen_stmt = std::make_unique<NginxConfigStatement>();
    listen_stmt->tokens_.push_back("listen");
    listen_stmt->tokens_.push_back("80");
    server_stmt->child_block_->statements_.emplace_back(std::move(listen_stmt));
    
    AddEchoLocation(server_stmt->child_block_.get(), "/");
    config.statements_.emplace_back(std::move(server_stmt));
    
    ServerConfig server_config = ExtractServerConfig(config, 8080);
    EXPECT_EQ(server_config.port, 80);
}

// Test extract_server_config with location blocks
TEST_F(ServerMainTest, ParsesLocationBlocks) {
    auto server_stmt = std::make_unique<NginxConfigStatement>();
    server_stmt->tokens_.push_back("server");
    server_stmt->child_block_ = std::make_unique<NginxConfig>();
    
    auto listen_stmt = std::make_unique<NginxConfigStatement>();
    listen_stmt->tokens_.push_back("listen");
    listen_stmt->tokens_.push_back("8080");
    server_stmt->child_block_->statements_.emplace_back(std::move(listen_stmt));
    
    AddEchoLocation(server_stmt->child_block_.get(), "/echo");
    AddEchoLocation(server_stmt->child_block_.get(), "/test");
    
    config.statements_.emplace_back(std::move(server_stmt));
    
    ServerConfig server_config = ExtractServerConfig(config, 8080);
    EXPECT_EQ(server_config.port, 8080);
    EXPECT_EQ(server_config.locations.size(), 2);
    
    // Test dispatcher can use the server config
    auto dispatcher = initialize_dispatcher(server_config);
    ASSERT_NE(dispatcher, nullptr);
    
    // Verify both handlers are accessible
    EXPECT_NE(dispatcher->CreateHandler("/echo"), nullptr);
    EXPECT_NE(dispatcher->CreateHandler("/test"), nullptr);
}

// Test initialize_from_args with valid config file
TEST_F(ServerMainTest, InitializeFromArgs_ValidFile) {
    char* argv[] = {(char*)"program", (char*)"server_config"};
    std::ostringstream err_stream;
    
    auto result = initialize_from_args(2, argv, 8080, err_stream);
    EXPECT_TRUE(result.is_valid());  // Success
    EXPECT_NE(result.dispatcher, nullptr);
    EXPECT_NE(result.port, 0);
    EXPECT_TRUE(err_stream.str().empty());
}

// Test initialize_from_args with too few arguments
TEST_F(ServerMainTest, InitializeFromArgs_TooFewArguments) {
    char* argv[] = {(char*)"program"};
    std::ostringstream err_stream;
    
    auto result = initialize_from_args(1, argv, 8080, err_stream);
    EXPECT_FALSE(result.is_valid());  // Error
    EXPECT_EQ(result.dispatcher, nullptr);
    EXPECT_EQ(result.port, 0);
    EXPECT_FALSE(err_stream.str().empty());
}

// Test initialize_from_args with too many arguments
TEST_F(ServerMainTest, InitializeFromArgs_TooManyArguments) {
    char* argv[] = {(char*)"program", (char*)"file1", (char*)"file2"};
    std::ostringstream err_stream;
    
    auto result = initialize_from_args(3, argv, 8080, err_stream);
    EXPECT_FALSE(result.is_valid());  // Error
    EXPECT_EQ(result.dispatcher, nullptr);
    EXPECT_EQ(result.port, 0);
    EXPECT_FALSE(err_stream.str().empty());
}

// Test initialize_from_args with nonexistent file
TEST_F(ServerMainTest, InitializeFromArgs_NonexistentFile) {
    char* argv[] = {(char*)"program", (char*)"nonexistent_file.conf"};
    std::ostringstream err_stream;
    
    auto result = initialize_from_args(2, argv, 8080, err_stream);
    EXPECT_FALSE(result.is_valid());  // Error
    EXPECT_EQ(result.dispatcher, nullptr);
    EXPECT_EQ(result.port, 0);
    EXPECT_FALSE(err_stream.str().empty());
}

// Test initialize_from_args with invalid config file
TEST_F(ServerMainTest, InitializeFromArgs_InvalidConfigFile) {
    // Create a temporary invalid config file
    std::ofstream temp_file("test_invalid.conf");
    temp_file << "server { listen 80; {}}"; // Invalid syntax
    temp_file.close();
    
    char* argv[] = {(char*)"program", (char*)"test_invalid.conf"};
    std::ostringstream err_stream;
    
    auto result = initialize_from_args(2, argv, 8080, err_stream);
    EXPECT_FALSE(result.is_valid());  // Error
    EXPECT_EQ(result.dispatcher, nullptr);
    EXPECT_EQ(result.port, 0);
    
    // Clean up
    std::remove("test_invalid.conf");
}

// Test validate_arguments
TEST_F(ServerMainTest, ValidateArguments_Correct) {
    EXPECT_TRUE(validate_arguments(2));
}

TEST_F(ServerMainTest, ValidateArguments_TooFew) {
    EXPECT_FALSE(validate_arguments(1));
}

TEST_F(ServerMainTest, ValidateArguments_TooMany) {
    EXPECT_FALSE(validate_arguments(3));
}

// Test load_config_from_file
TEST_F(ServerMainTest, LoadConfigFromFile_ValidFile) {
    EXPECT_TRUE(load_config_from_file("server_config", &config));
}

TEST_F(ServerMainTest, LoadConfigFromFile_InvalidFile) {
    EXPECT_FALSE(load_config_from_file("nonexistent.conf", &config));
}

TEST_F(ServerMainTest, LoadConfigFromFile_NullConfig) {
    EXPECT_FALSE(load_config_from_file("server_config", nullptr));
}
