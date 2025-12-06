#ifndef SERVER_MAIN_UTILS_H
#define SERVER_MAIN_UTILS_H

#include "config_parser.h"
#include "server_config.h"
#include "request_dispatcher.h"
#include <string>
#include <iostream>
#include <memory>

// Forward declaration
class NginxConfig;

// Structure to hold initialization results
struct ServerInitResult {
    std::shared_ptr<RequestDispatcher> dispatcher;
    short port;
    
    // Returns true if initialization was successful
    bool is_valid() const { return dispatcher != nullptr && port != 0; }
};

// Validate command line arguments
// Returns true if argc is correct (should be 2: program name + config file)
bool validate_arguments(int argc);

// Load and parse config from file
// Returns true if file was successfully parsed, false otherwise
bool load_config_from_file(const char* file_path, NginxConfig* config);

// Initialize server from command line arguments
// Parses config file, extracts server config, and initializes dispatcher
// Returns ServerInitResult with dispatcher and port (check is_valid() for success)
ServerInitResult initialize_from_args(int argc, char* argv[], short default_port = 8080,
                                       std::ostream& err_stream = std::cerr);

// Extract server configuration from parsed NginxConfig
// Returns ServerConfig struct with port and location data
ServerConfig ExtractServerConfig(const NginxConfig& config, short default_port = 8080);

// Initialize request dispatcher from extracted server config
// Returns pointer to dispatcher on success, nullptr on error
std::shared_ptr<RequestDispatcher> initialize_dispatcher(const ServerConfig& server_config);

// Register a handler factory for a location configuration
// Creates the appropriate factory based on handler_name and registers it
// Returns true if registration was successful, false otherwise
bool RegisterHandlerForLocation(const LocationConfig& location);

void writeLogToConsole();
void writeLogToFile(const std::string &name = "logs");

#endif  // SERVER_MAIN_UTILS_H

