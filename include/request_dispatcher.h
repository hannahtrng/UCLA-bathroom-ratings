#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include <memory>
#include <string>
#include <vector>

#include "server_config.h"
#include "request_handler.h"

// Forward declaration
class NginxConfig;

class RequestDispatcher {
public:
    RequestDispatcher();
    ~RequestDispatcher();
    
    // Initialize dispatcher from ServerConfig
    bool InitFromServerConfig(const ServerConfig& server_config);
    
    // Create a handler for a given URL path
    // Returns nullptr if no handler matches
    // Caller takes ownership of the returned handler
    std::unique_ptr<RequestHandler> CreateHandler(const std::string& url_path);

private:
    
    // Find best matching location config for a URL (longest prefix match)
    // Returns nullptr if no match found
    const LocationConfig* FindBestMatch(const std::string& url_path) const;
    
    // Location configurations (stored to match URLs and create handlers on demand)
    std::vector<LocationConfig> locations_;
};

#endif // REQUEST_DISPATCHER_H

