// parser for http requests :)

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "request_handler.h"

class RequestParser {
    public:
        RequestParser() {}
        bool Parse(const std::string& request_buffer, request& request);

    protected:
        // Helper methods for parsing
        bool ParseRequestLine(const std::string& line, request& request);
        bool ParseHeaderLine(const std::string& line, request& request);
};

#endif  // REQUEST_PARSER_H
