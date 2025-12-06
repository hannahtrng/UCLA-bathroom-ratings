#include "request_dispatcher.h"
#include "handler_registry.h"
#include "handler_factory.h"
#include "config_parser.h"

#include <algorithm>
#include <cstdlib>  // For atoi
#include <boost/log/trivial.hpp>

RequestDispatcher::RequestDispatcher() {}

RequestDispatcher::~RequestDispatcher() {}

bool RequestDispatcher::InitFromServerConfig(const ServerConfig& server_config) {
    BOOST_LOG_TRIVIAL(info) << "Initializing request dispatcher from server config";
    
    if (server_config.locations.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "No location blocks found in server config";
        return false;
    }
    
    locations_ = server_config.locations;
    
    BOOST_LOG_TRIVIAL(info) << "Dispatcher initialized with " << server_config.locations.size() << " locations";
    
    return true;
}

const LocationConfig* RequestDispatcher::FindBestMatch(const std::string& url_path) const {
    const LocationConfig* best_match = nullptr;
    size_t best_match_length = 0;
    
    // Iterate over locations_ to find the best matching location
    for (const auto& location : locations_) {
        if (url_path.find(location.path) == 0 && location.path.length() > best_match_length) {
            best_match = &location;
            best_match_length = location.path.length();
        }
    }
    
    return best_match;
}

std::unique_ptr<RequestHandler> RequestDispatcher::CreateHandler(const std::string& url_path) {
    const LocationConfig* matched_location = FindBestMatch(url_path);
    
    if (!matched_location) {
        // No location matched, create NotFoundHandler directly
        BOOST_LOG_TRIVIAL(warning) << "No handler found for URL: " << url_path << ", using NotFoundHandler";
        LocationConfig not_found_config;
        not_found_config.path = "";
        not_found_config.handler_name = "NotFoundHandler";
        NotFoundHandlerFactory factory(not_found_config);
        return factory.Create();
    }
    
    // Get factory from registry using path
    auto factory = HandlerRegistry::GetFactory(matched_location->path);
    if (!factory) {
        // Factory not found, fall back to NotFoundHandler
        BOOST_LOG_TRIVIAL(error) << "Factory not found for path: " << matched_location->path << ", using NotFoundHandler";
        LocationConfig not_found_config;
        not_found_config.path = "";
        not_found_config.handler_name = "NotFoundHandler";
        NotFoundHandlerFactory not_found_factory(not_found_config);
        return not_found_factory.Create();
    }
    
    // Use factory to create handler (already returns unique_ptr)
    return factory->Create();
}
