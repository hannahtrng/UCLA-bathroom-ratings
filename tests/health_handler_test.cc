#include "gtest/gtest.h"
#include "request_handler_health.h"

#include <algorithm>

class HealthHandlerTest : public ::testing::Test {
protected:
    HealthRequestHandler handler;
};

TEST_F(HealthHandlerTest, ReturnsOkResponse) {
    request req{};
    req.url = "/health";

    auto resp = handler.handle_request(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->status_code, "200 OK");
    EXPECT_EQ(resp->content_type, "text/plain");
    EXPECT_EQ(resp->body, "OK");
    EXPECT_EQ(resp->content_length, std::to_string(resp->body.size()));

    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(), "Content-Type: text/plain"),
              resp->headers.end());
    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(),
                        "Content-Length: " + resp->content_length),
              resp->headers.end());
    EXPECT_NE(std::find(resp->headers.begin(), resp->headers.end(), "Connection: close"),
              resp->headers.end());
}

TEST_F(HealthHandlerTest, AlwaysReturnsOkRegardlessOfRequest) {
    request req1{};
    req1.url = "/health";
    req1.method = "GET";

    request req2{};
    req2.url = "/health";
    req2.method = "POST";
    req2.body = "some body";

    auto resp1 = handler.handle_request(req1);
    auto resp2 = handler.handle_request(req2);

    ASSERT_NE(resp1, nullptr);
    ASSERT_NE(resp2, nullptr);
    EXPECT_EQ(resp1->status_code, "200 OK");
    EXPECT_EQ(resp2->status_code, "200 OK");
    EXPECT_EQ(resp1->body, "OK");
    EXPECT_EQ(resp2->body, "OK");
}

