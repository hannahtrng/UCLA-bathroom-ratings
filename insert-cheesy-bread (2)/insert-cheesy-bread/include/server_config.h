#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <vector>
#include <map>

// Represents a single location block configuration
struct LocationConfig {
    std::string path;           // URL path like "/echo" or "/static"
    std::string handler_name;   // Handler class name like "EchoHandler"
    std::map<std::string, std::string> args;  // Additional arguments (e.g., "root" for file handler)
};

// Represents the parsed server configuration
struct ServerConfig {
    short port;
    std::vector<LocationConfig> locations;
};

#endif // SERVER_CONFIG_H

