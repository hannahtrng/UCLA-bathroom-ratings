#include "gtest/gtest.h"
#include "request_handler_not_found.h"

#include <algorithm>

class NotFoundHandlerTest : public ::testing::Test {
protected:
    NotFoundRequestHandler handler;
};

TEST_F(NotFoundHandlerTest, ReturnsDefaultNotFoundResponse) {
    request req{};
    req.url = "/does-not-exist";

    auto resp = handler.handle_request(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->status_code, "404 Not Found");
    EXPECT_EQ(resp->content_type, "text/plain");
    EXPECT_EQ(resp->body, "404 Not Found");
    EXPECT_EQ(resp->content_length, std::to_string(resp->body.size()));

    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(), "Content-Type: text/plain"),
              resp->headers.end());
    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(),
                        "Content-Length: " + resp->content_length),
              resp->headers.end());
    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(), "Connection: close"),
              resp->headers.end());
}
