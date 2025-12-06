#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include "request_handler.h"
#include "request_handler_file.h"
#include "response_builder.h"

namespace fs = boost::filesystem;

std::unique_ptr<response> FileRequestHandler::handle_request(const request& request) {
    std::string url_path = request.url;
    
    // Strip the URL prefix if it matches
    if (!url_prefix_.empty() && url_path.find(url_prefix_) == 0) {
        url_path = url_path.substr(url_prefix_.length());
        if (url_path.empty()) {
            url_path = "/";
        }
    }
    
    // Remove leading slash for relative path construction
    if (!url_path.empty() && url_path[0] == '/') {
        url_path = url_path.substr(1);
    }
    
    // Handle empty path (directory root)
    if (url_path.empty()) {
        url_path = ".";
    }
    
    fs::path requested_path = fs::path(base_dir_) / url_path;
    
    BOOST_LOG_TRIVIAL(info) << "FileHandler: base_dir=" << base_dir_ 
                            << ", url_path=" << url_path 
                            << ", requested_path=" << requested_path.string();
    
    // Check if file exists first
    if (!fs::exists(requested_path)) {
        BOOST_LOG_TRIVIAL(warning) << "File does not exist: " << requested_path.string();
        return ResponseBuilder::NotFound();
    }
    
    try {
        requested_path = fs::canonical(requested_path);
        fs::path base_canonical = fs::canonical(base_dir_);
        
        BOOST_LOG_TRIVIAL(info) << "FileHandler: canonical requested_path=" << requested_path.string()
                                << ", canonical base_dir=" << base_canonical.string();
        
        std::string requested_str = requested_path.string();
        std::string base_str = base_canonical.string();
        
        if (requested_str.find(base_str) != 0) {
            return ResponseBuilder::Forbidden();
        }
        
        if (!fs::exists(requested_path)) {
            return ResponseBuilder::NotFound();
        }
        
        if (fs::is_directory(requested_path)) {
            requested_path /= "index.html";
            if (!fs::exists(requested_path)) {
                return ResponseBuilder::NotFound();
            }
        }
        
        std::string file_content = read_file(requested_path.string());
        std::string content_type = get_content_type(requested_path.string());
        
        return ResponseBuilder::Ok(file_content, content_type);
        
    } catch (const fs::filesystem_error& e) {
        return ResponseBuilder::NotFound();
    }
}

std::string FileRequestHandler::read_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string FileRequestHandler::get_content_type(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    
    return "application/octet-stream";
}

std::string FileRequestHandler::GetHandlerName() const {
    return "StaticHandler";
}