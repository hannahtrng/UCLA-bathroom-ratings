#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <boost/log/trivial.hpp>

#include "request_parser.h"

bool RequestParser::Parse(const std::string& request_buffer, request& request) {
    BOOST_LOG_TRIVIAL(info) << "Parsing request buffer of size " << request_buffer.size();
    size_t header_end = request_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        BOOST_LOG_TRIVIAL(info) << "Incomplete request, waiting for more data.";
        return false;
    }
    
    request.is_valid = true;
    request.raw_request = request_buffer;
    
    std::istringstream stream(request_buffer);
    std::string line;
    
    if (!std::getline(stream, line)) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to get request line from buffer.";
        request.is_valid = false;
        return true;
    }
    
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    if (!ParseRequestLine(line, request)) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to parse request line: " << line;
        return true;
    }
    
    while (std::getline(stream, line) && line != "\r") {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            break;
        }
        
        ParseHeaderLine(line, request);
    }
    
    size_t body_start = header_end + 4;
    if (body_start < request_buffer.size()) {
        request.body = request_buffer.substr(body_start);
        BOOST_LOG_TRIVIAL(info) << "Request has body of size " << request.body.size();
    }
    
    BOOST_LOG_TRIVIAL(info) << "Successfully parsed a complete request.";
    return true;
}

namespace {
    const std::unordered_set<std::string> kAllowedMethods = {"GET", "POST", "PUT",
                                                             "DELETE"};
} // namespace

bool RequestParser::ParseRequestLine(const std::string& line, request& request) {
    std::istringstream stream(line);
    std::string method, url, http_version;
    
    if (!(stream >> method >> url >> http_version)) {
        request.is_valid = false;
        return false;
    }
    
    if (!kAllowedMethods.count(method)) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid request method: " << method;
        request.is_valid = false;
        return false;
    }
    
    if (url.empty() || url[0] != '/') {
        BOOST_LOG_TRIVIAL(warning) << "Invalid URL: " << url;
        request.is_valid = false;
        return false;
    }
    
    if (http_version != "HTTP/1.1") {
        BOOST_LOG_TRIVIAL(warning) << "Invalid HTTP version: " << http_version;
        request.is_valid = false;
        return false;
    }
    
    request.method = method;
    request.url = url;
    request.http_version = "1.1";
    
    return true;
}

bool RequestParser::ParseHeaderLine(const std::string& line, request& request) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid header line: " << line;
        return false;
    }
    
    std::string header_name = line.substr(0, colon_pos);
    std::string header_value = line.substr(colon_pos + 1);
    
    while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t')) {
        header_value.erase(0, 1);
    }
    
    while (!header_value.empty() && (header_value.back() == ' ' || header_value.back() == '\t')) {
        header_value.pop_back();
    }
    
    request.headers.push_back(header_name + ": " + header_value);
    return true;
}
