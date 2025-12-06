#include "server_main_utils.h"
#include "handler_registry.h"
#include "handler_factory.h"
#include "entity_store_persistent.h"
#include <cstdlib>  // For atoi

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/support/date_time.hpp> 
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/attributes.hpp>
#include <filesystem>
#include <ctime>  
#include <memory>

using boost::posix_time::ptime;


bool validate_arguments(int argc) {
    return argc == 2;
}

bool load_config_from_file(const char* file_path, NginxConfig* config) {
    if (config == nullptr) {
        return false;
    }
    NginxConfigParser parser;
    return parser.Parse(file_path, config);
}

bool RegisterHandlerForLocation(const LocationConfig& location) {
    if (location.handler_name.empty()) {
        return false;
    }
    
    // Create factory based on handler name
    std::shared_ptr<HandlerFactory> factory;
    if (location.handler_name == "EchoHandler") {
        factory = std::make_shared<EchoHandlerFactory>(location);
    } else if (location.handler_name == "StaticHandler") {
        factory = std::make_shared<StaticHandlerFactory>(location);
    } else if (location.handler_name == "HealthHandler") {
        factory = std::make_shared<HealthHandlerFactory>(location);
    } else if (location.handler_name == "SleepHandler") {
        factory = std::make_shared<SleepHandlerFactory>(location);
    } else if (location.handler_name == "ApiHandler") {
        std::string data_path = ".";
        auto it = location.args.find("data_path");
        if (it != location.args.end()) {
            data_path = it->second;
        }
        std::shared_ptr<EntityStore> entity_store = std::make_shared<EntityStorePersistent>(data_path);
        factory = std::make_shared<ApiHandlerFactory>(location, entity_store);
    } else {
        BOOST_LOG_TRIVIAL(warning) << "Unknown handler name: " << location.handler_name
                                   << " for path: " << location.path;
        return false;
    }
    
    // Register the handler factory using path as the key
    HandlerRegistry::RegisterHandler(location.path, factory);
    return true;
}


ServerConfig ExtractServerConfig(const NginxConfig& config, short default_port) {
    ServerConfig server_config;
    server_config.port = default_port;
    
    // Look for server block
    for (const auto& statement : config.statements_) {
        if (statement->tokens_.size() >= 1 && 
            statement->tokens_[0] == "server" &&
            statement->child_block_) {
            
            // Parse all directives inside server block
            for (const auto& server_stmt : statement->child_block_->statements_) {
                // Extract port from listen directive
                if (server_stmt->tokens_.size() >= 2 && 
                    server_stmt->tokens_[0] == "listen") {
                    server_config.port = atoi(server_stmt->tokens_[1].c_str());
                    BOOST_LOG_TRIVIAL(info) << "Extracted listen port: " << server_config.port;
                }
                // Extract location blocks
                else if (server_stmt->tokens_.size() >= 2 && 
                    server_stmt->tokens_[0] == "location" &&
                    server_stmt->child_block_) {
                    
                    LocationConfig location;
                    location.path = server_stmt->tokens_[1];
                    
                    // Parse directives inside location block
                    for (const auto& location_stmt : server_stmt->child_block_->statements_) {
                        if (location_stmt->tokens_.size() >= 1) {
                            std::string directive = location_stmt->tokens_[0];
                            
                            // Parse handler directive (e.g., "handler EchoHandler" or "handler StaticHandler")
                            if (directive == "handler" && location_stmt->tokens_.size() >= 2) {
                                location.handler_name = location_stmt->tokens_[1];
                            }
                            // Parse root directive (e.g., "root ./files")
                            else if (directive == "root" && location_stmt->tokens_.size() >= 2) {
                                location.args["root"] = location_stmt->tokens_[1];
                            }
                            // Parse data_path directive (e.g., "data_path ./files")
                            else if (directive == "data_path" && location_stmt->tokens_.size() >= 2) {
                                location.args["data_path"] = location_stmt->tokens_[1];
                            }
                            // Store other directives as args
                            else if (location_stmt->tokens_.size() >= 2) {
                                location.args[directive] = location_stmt->tokens_[1];
                            }
                        }
                    }
                    
                    // Add location if handler type was specified
                    if (!location.handler_name.empty()) {
                        // Register the handler factory for this location
                        if (RegisterHandlerForLocation(location)) {
                            server_config.locations.push_back(location);
                            BOOST_LOG_TRIVIAL(info) << "Extracted location: " << location.path 
                                                    << " -> " << location.handler_name;
                        }
                    }
                }
            }
            break;  // Only process first server block
        }
    }
    
    return server_config;
}

ServerInitResult initialize_from_args(int argc, char* argv[], short default_port,
                                      std::ostream& err_stream) {
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    
    // Validate arguments
    if (!validate_arguments(argc)) {
        err_stream << "Usage: async_tcp_echo_server <config_file>\n";
        BOOST_LOG_TRIVIAL(error) << "Error: invalid arguments";
        return ServerInitResult{nullptr, 0};  // Return invalid result
    }

    // Load and parse config file
    NginxConfig config;
    if (!load_config_from_file(argv[1], &config)) {
        err_stream << "Error: Could not parse config file: " << argv[1] << "\n";
        BOOST_LOG_TRIVIAL(error) << "Error: cannot load config file";
        return ServerInitResult{nullptr, 0};  // Return invalid result
    }
    
    // Extract server configuration to get port
    ServerConfig server_config = ExtractServerConfig(config, default_port);
    BOOST_LOG_TRIVIAL(info) << "Extracted server config: port=" << server_config.port 
                            << ", locations=" << server_config.locations.size();
    
    // Initialize dispatcher
    auto dispatcher = initialize_dispatcher(server_config);
    if (!dispatcher) {
        err_stream << "Error: Failed to initialize request dispatcher\n";
        BOOST_LOG_TRIVIAL(error) << "Failed to initialize request dispatcher";
        return ServerInitResult{nullptr, 0};  // Return invalid result
    }
    
    BOOST_LOG_TRIVIAL(info) << "Server initialization complete";
    return ServerInitResult{dispatcher, server_config.port};
}

std::shared_ptr<RequestDispatcher> initialize_dispatcher(const ServerConfig& server_config) {
    auto dispatcher = std::make_shared<RequestDispatcher>();
    
    if (!dispatcher->InitFromServerConfig(server_config)) {
        BOOST_LOG_TRIVIAL(error) << "Failed to initialize request dispatcher";
        return nullptr;
    }
    
    BOOST_LOG_TRIVIAL(info) << "Request dispatcher initialized successfully with " 
                            << server_config.locations.size() << " routes";
    return dispatcher;
}

void writeLogToFile(const std::string& name) {
    boost::log::add_common_attributes();
    std::filesystem::path pathToLog =
    std::filesystem::current_path().parent_path() / "logs"; 
    std::filesystem::create_directories(pathToLog);
    boost::log::add_file_log(
    boost::log::keywords::file_name = (pathToLog/"%Y%m%d.log").string(),
    boost::log::keywords::open_mode = std::ios_base::out | std::ios_base::app,
    boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
    boost::log::keywords::rotation_size = 10 * 1024 * 1024,
    boost::log::keywords::auto_flush = true,
    boost::log::keywords::format =
      (boost::log::expressions::stream
        << "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << "] " // timestamp
        << "[" << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "] " // current thread ID
        << "[" << boost::log::trivial::severity << "] " // severity level
        << boost::log::expressions::smessage)); // message
}

void writeLogToConsole() {
    boost::log::add_common_attributes(); 
    boost::log::add_console_log(
    std::clog,
    boost::log::keywords::format =
        (boost::log::expressions::stream
        << "[" << boost::log::expressions::format_date_time<ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << "] " // timestamp 
        << "[" << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "] " // current thread ID
        << "[" << boost::log::trivial::severity << "] " //severity level
        << boost::log::expressions::smessage)); // message
}
