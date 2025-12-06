#include "request_parser.h"
#include "gtest/gtest.h"

class RequestParserTest : public ::testing::Test {
protected:
    RequestParser parser;
    request request_;
};

// Test valid GET request
TEST_F(RequestParserTest, ValidGetRequest) {
    std::string request_buffer = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.method, "GET");
    EXPECT_EQ(request_.url, "/index.html");
    EXPECT_EQ(request_.http_version, "1.1");
    EXPECT_EQ(request_.headers.size(), 1);
    EXPECT_EQ(request_.headers[0], "Host: example.com");
}

// Test GET request with HTTP/1.0 (should be rejected since only 1.1 supported)
TEST_F(RequestParserTest, HTTP10NotSupported) {
    std::string request_buffer = "GET / HTTP/1.0\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test request with multiple headers
TEST_F(RequestParserTest, MultipleHeaders) {
    std::string request_buffer =
        "GET /test HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.headers.size(), 3);
    EXPECT_EQ(request_.headers[0], "Host: example.com");
    EXPECT_EQ(request_.headers[1], "User-Agent: TestAgent/1.0");
    EXPECT_EQ(request_.headers[2], "Accept: text/html");
}

// Test request with body
TEST_F(RequestParserTest, RequestWithBody) {
    std::string request_buffer =
        "GET /data HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.body, "Hello World");
}

// Test request with whitespace in header values
TEST_F(RequestParserTest, HeadersWithWhitespace) {
    std::string request_buffer =
        "GET / HTTP/1.1\r\n"
        "Host:   example.com   \r\n"
        "\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.headers[0], "Host: example.com");
}

// Test missing \r\n\r\n (incomplete request)
TEST_F(RequestParserTest, IncompleteRequest) {
    std::string request_buffer = "GET / HTTP/1.1\r\nHost: example.com\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_FALSE(result);
}

// Test empty request
TEST_F(RequestParserTest, EmptyRequest) {
    std::string request_buffer = "\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test non-GET method (should fail per TODO comment)
TEST_F(RequestParserTest, FakeMethodNotSupported) {
    std::string request_buffer = "CHEESE /data HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test invalid URL (not starting with /)
TEST_F(RequestParserTest, InvalidURL) {
    std::string request_buffer = "GET invalid HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test invalid HTTP version
TEST_F(RequestParserTest, InvalidHTTPVersion) {
    std::string request_buffer = "GET / HTTP/2.0\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test malformed request line
TEST_F(RequestParserTest, MalformedRequestLine) {
    std::string request_buffer = "INVALID\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test request with empty body
TEST_F(RequestParserTest, EmptyBody) {
    std::string request_buffer = "GET / HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_TRUE(request_.body.empty());
}

// Test URL with query parameters
TEST_F(RequestParserTest, URLWithQueryParams) {
    std::string request_buffer = "GET /search?q=test&page=1 HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.url, "/search?q=test&page=1");
}

// Test complex URL path
TEST_F(RequestParserTest, ComplexURLPath) {
    std::string request_buffer = "GET /path/to/resource.html HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    EXPECT_EQ(request_.url, "/path/to/resource.html");
}

// Test header without colon (malformed)
TEST_F(RequestParserTest, HeaderWithoutColon) {
    std::string request_buffer =
        "GET / HTTP/1.1\r\n"
        "InvalidHeader\r\n"
        "\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_TRUE(request_.is_valid);
    // Malformed header should be silently ignored
    EXPECT_EQ(request_.headers.size(), 0);
}

// Test URL without leading slash to hit SetError line
TEST_F(RequestParserTest, URLMissingLeadingSlash) {
    std::string request_buffer = "GET test.html HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test completely invalid URL to hit SetError line
TEST_F(RequestParserTest, CompletelyInvalidURL) {
    std::string request_buffer = "GET example.com HTTP/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test HTTP/2.0 to hit SetError line
TEST_F(RequestParserTest, HTTP20Version) {
    std::string request_buffer = "GET / HTTP/2.0\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test HTTP/0.9 to hit SetError line
TEST_F(RequestParserTest, HTTP09Version) {
    std::string request_buffer = "GET / HTTP/0.9\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test invalid HTTP version format to hit SetError line
TEST_F(RequestParserTest, InvalidHTTPFormat) {
    std::string request_buffer = "GET / HTTPS/1.1\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test request line with only method (missing URL and version) to hit SetError line 56-58
TEST_F(RequestParserTest, RequestLineOnlyMethod) {
    std::string request_buffer = "GET\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}

// Test request line with only method and URL (missing version) to hit SetError line 56-58
TEST_F(RequestParserTest, RequestLineMissingVersion) {
    std::string request_buffer = "GET /index.html\r\n\r\n";

    bool result = parser.Parse(request_buffer, request_);

    EXPECT_TRUE(result);
    EXPECT_FALSE(request_.is_valid);
}
