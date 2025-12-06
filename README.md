# Insert-Cheesy-Bread Repository
## Introduction
**Authors:** Mikey Choi, Teresa Lee, Travis Nguyen, and Hannah Truong 
Welcome to our repository! 'insert-cheesy-bread' is a simple HTTP/1.1 server built for CS130 that emphasizes clarity, modularity, and testability. Routes are defined in an nginx-style config and dispatched via longest-prefix matching to short-lived request handlers created by factories. The codebase follows the class Common API, so contributors can drop in new handlers (e.g., 'EchoHandler', 'StaticHandler', 'NotFoundHandler') without touching the core server. Everything is wired through CMake, covered by unit/integration tests, and organized for quick experimentation and review. 

## Prerequisites
Before building, ensure you have:
- **CMake** (version 3.10 or higher)
- **C++ compiler** with C++17 support (GCC 7+, Clang 5+, or MSVC 2017+)
- **Boost libraries** (tested with 1.65+):
  - Boost.Asio
  - Boost.Filesystem
  - Boost.Log
  - Boost.System

## How To Build, Test, and Run Code

### Building the Server
```bash
# From the project root directory
mkdir build
cd build
cmake ..
make
```

This will generate the server executable in the `build/` directory.

### Running the Server
To run the server, run the following command from the root directory
You can also use your own configuration file as the argument for the program.

```bash
# From the build directory
./build/bin/server_main <path_to_config_file>

# Example:
./build/bin/server_main ./server_config_files/example.conf
```

**Sample Configuration File** (`config/server.conf`):
```nginx
server {
    listen 8080;
    
    location /echo {
        handler EchoHandler;
    }
    
    location /static {
        handler StaticHandler;
        root ./files;
    }
}
```

### Running Tests
```bash
# From the build directory
make test

# Or run specific test binaries directly:
./request_parser_test
./config_parser_test
./request_dispatcher_test
```

### Server Logs
The server writes logs to:
- **Console**: Real-time log output during execution
- **Log files**: `../logs/YYYYMMDD.log` (rotated daily)

## General Source Code Layout

Our source code is organized into the following sections:

### 1. **Core Server Infrastructure** (`server.cc`, `session.cc`, `server_main.cc`)
- **`server.cc`**: Manages the TCP acceptor and spawns new sessions for each connection
- **`session.cc`**: Handles individual client connections, reads requests, and writes responses
- **`server_main.cc`**: Entry point that initializes logging, parses config, and starts the server

### 2. **Request Processing Pipeline** (`request_parser.cc`, `request_dispatcher.cc`)
- **`request_parser.cc`**: Parses raw HTTP/1.1 requests into structured `Request` objects
- **`request_dispatcher.cc`**: Routes incoming requests to appropriate handlers using longest-prefix matching

### 3. **Request Handlers** (`request_handler_*.cc`, `handler_factory.cc`)
- **`request_handler.h`**: Base interface that all handlers implement
- **`request_handler_echo.cc`**: Example handler that echoes back the raw request
- **`request_handler_file.cc`**: Serves static files from a configured root directory
- **`handler_factory.cc`**: Factory classes that instantiate handlers with proper configuration

### 4. **Configuration & Utilities** (`config_parser.cc`, `server_main_utils.cc`, `response_builder.cc`)
- **`config_parser.cc`**: Parses nginx-style configuration files
- **`server_main_utils.cc`**: Initialization utilities for server setup
- **`response_builder.cc`**: Helper functions for constructing HTTP responses (200 OK, 404 Not Found, etc.)

### Directory Structure
```
project_root/
├── include/           # Header files
├── src/              # Source files
├── tests/            # Unit and integration tests
├── config/           # Sample configuration files
├── logs/             # Server log output (auto-created)
└── build/            # Build artifacts (created by CMake)
```

## Adding Request Handlers

Adding a new request handler is straightforward and requires no changes to the core server code. Follow these steps:

### Step 1: Create Your Handler Class

Create two files: a header (`request_handler_<name>.h`) and implementation (`request_handler_<name>.cc`).

**Example: Creating a "Time" handler that returns the current server time**

**`include/request_handler_time.h`:**
```cpp
#ifndef REQUEST_HANDLER_TIME_H
#define REQUEST_HANDLER_TIME_H

#include "request_handler.h"

class TimeRequestHandler : public RequestHandler {
public:
    std::unique_ptr<response> handle_request(const Request& request) override;
};

#endif
```

**`src/request_handler_time.cc`:**
```cpp
#include "request_handler_time.h"
#include "response_builder.h"
#include <ctime>
#include <sstream>

std::unique_ptr<response> TimeRequestHandler::handle_request(const Request& request) {
    // Get current time
    std::time_t now = std::time(nullptr);
    std::string time_str = std::ctime(&now);
    
    // Build and return response
    return ResponseBuilder::Ok(time_str, "text/plain");
}
```

### Step 2: Create a Factory for Your Handler

**`include/handler_factory.h`** (add to existing file):
```cpp
class TimeHandlerFactory : public HandlerFactory {
public:
    TimeHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    RequestHandler* Create() override;
};
```

**`src/handler_factory.cc`** (add to existing file):
```cpp
#include "request_handler_time.h"

RequestHandler* TimeHandlerFactory::Create() {
    return new TimeRequestHandler();
}
```

### Step 3: Register Your Handler

**In `src/request_dispatcher.cc`**, update the `CreateHandler` method:
```cpp
RequestHandler* RequestDispatcher::CreateHandler(const LocationConfig& config) {
    if (config.handler_type == "echo") {
        return new EchoRequestHandler();
    } 
    else if (config.handler_type == "file") {
        std::string root = ".";
        auto it = config.args.find("root");
        if (it != config.args.end()) {
            root = it->second;
        }
        return new FileRequestHandler(root, config.path);
    }
    else if (config.handler_type == "time") {  // ADD THIS
        return new TimeRequestHandler();
    }
    else {
        BOOST_LOG_TRIVIAL(error) << "Unknown handler type: " << config.handler_type;
        return nullptr;
    }
}
```

**In `src/server_main_utils.cc`**, update the config parsing to recognize your handler:
```cpp
// Inside ExtractServerConfig function, in the handler parsing section
if (handler_name == "EchoHandler") {
    location.handler_type = "echo";
} else if (handler_name == "StaticHandler") {
    location.handler_type = "file";
} else if (handler_name == "TimeHandler") {  // ADD THIS
    location.handler_type = "time";
}
```

### Step 4: Configure Your Handler

Add your handler to the server configuration file:
```nginx
server {
    listen 8080;
    
    location /time {
        handler TimeHandler;
    }
}
```

### Step 5: Rebuild and Test
```bash
cd build
make
./build/bin/server_main ./server_config_files/example.conf

# In another terminal:
curl http://localhost:8080/time
```

## Important Header Files

Understanding these key headers will help you work with the codebase:

### `request_handler.h` - Base Handler Interface
All request handlers must inherit from this interface:
```cpp
class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    
    // Process an HTTP request and return an HTTP response string
    virtual std::unique_ptr<response> handle_request(const Request& requestr) = 0;
};
```

### `request_handler.h` - Request Structure
The parsed HTTP request is represented as:
```cpp
struct Request {
    std::string method;        // "GET", "POST", etc.
    std::string url;           // Request URL path
    std::string http_version;  // "1.1"
    std::vector<std::string> headers;  // "Host: example.com", etc.
    std::string body;          // Request body (for POST, etc.)
    bool is_valid;             // Whether parsing succeeded
};
```

### `response_builder.h` - Response Helpers
Use these static methods to build responses:
```cpp
class ResponseBuilder {
public:
    static std::unique_ptr<response> Ok(const std::string& body, 
                         const std::string& content_type = "text/plain");
    static std::unique_ptr<response> NotFound(const std::string& message = "Not Found");
    static std::unique_ptr<response> Forbidden(const std::string& message = "Forbidden");
    static std::unique_ptr<response> BadRequest(const std::string& message = "Bad Request");
    static std::unique_ptr<response> InternalServerError(const std::string& message = "Internal Server Error");
    static std::unique_ptr<response> MethodNotAllowed(const std::string& message = "Method Not Allowed");
};
```

### `config_parser.h` - Configuration Structures
Configuration is parsed into these structures:
```cpp
struct LocationConfig {
    std::string path;                        // "/static", "/echo", etc.
    std::string handler_type;                // "file", "echo", "time"
    std::map<std::string, std::string> args; // Handler-specific arguments
};

struct ServerConfig {
    short port;
    std::vector<LocationConfig> locations;
};
```

## Code Examples: Existing Request Handlers

### Example 1: EchoRequestHandler (Simple Handler)
The echo handler is the simplest possible handler - it returns the raw request:

**`src/request_handler_echo.cc`:**
```cpp
#include "request_handler_echo.h"
#include "response_builder.h"

std::unique_ptr<response> EchoRequestHandler::handle_request(const Request& request) {
    // Simply return the entire raw request as the response body
    return ResponseBuilder::Ok(request_buffer, "text/plain");
}
```

**Usage in config:**
```nginx
location /echo {
    handler EchoHandler;
}
```

**Testing:**
```bash
curl http://localhost:8080/echo -d "Hello World"
# Returns the entire HTTP request including headers
```

### Example 2: FileRequestHandler (Stateful Handler)
The file handler demonstrates a more complex handler with configuration:

**`src/request_handler_file.cc`** (simplified):
```cpp
class FileRequestHandler : public RequestHandler {
private:
    std::string base_dir_;    // Root directory to serve from
    std::string url_prefix_;  // URL prefix to strip
    
public:
    FileRequestHandler(const std::string& base_dir, const std::string& url_prefix)
        : base_dir_(base_dir), url_prefix_(url_prefix) {}
    
    std::unique_ptr<response> handle_request(const Request& request, ) override {
        std::unique_ptr<response> url_path = request.url;
        
        // Strip URL prefix (/static/file.txt -> /file.txt)
        if (!url_prefix_.empty() && url_path.find(url_prefix_) == 0) {
            url_path = url_path.substr(url_prefix_.length());
        }
        
        // Build file path
        fs::path requested_path = fs::path(base_dir_) / url_path;
        
        // Security check: prevent directory traversal
        fs::path canonical = fs::canonical(requested_path);
        fs::path base_canonical = fs::canonical(base_dir_);
        if (canonical.string().find(base_canonical.string()) != 0) {
            return ResponseBuilder::Forbidden();
        }
        
        // Check if file exists
        if (!fs::exists(canonical)) {
            return ResponseBuilder::NotFound();
        }
        
        // Read and return file
        std::string content = read_file(canonical.string());
        std::string content_type = get_content_type(canonical.string());
        return ResponseBuilder::Ok(content, content_type);
    }
};
```

**Usage in config:**
```nginx
location /static {
    handler StaticHandler;
    root ./files;  # Serve files from ./files directory
}
```

**Testing:**
```bash
# Assuming ./files/index.html exists
curl http://localhost:8080/static/index.html
# Returns the content of ./files/index.html with correct Content-Type
```

## Additional Resources

- **CS130 Common API Documentation**: [https://www.cs130.org/assignments/](https://www.cs130.org/assignments/)
- **Boost.Asio Documentation**: [https://www.boost.org/doc/libs/release/doc/html/boost_asio.html](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- **HTTP/1.1 RFC**: [https://www.rfc-editor.org/rfc/rfc2616](https://www.rfc-editor.org/rfc/rfc2616)