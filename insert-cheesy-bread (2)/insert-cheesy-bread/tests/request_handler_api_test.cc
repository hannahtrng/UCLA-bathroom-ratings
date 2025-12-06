#include "gtest/gtest.h"

#include <memory>
#include <string>

#include "entity_store_in_memory.h"
#include "request_handler_api.h"

// This code was partially created with the help of GPT-5.1-CODEX

namespace {
    request MakeRequest(const std::string &method, const std::string &url,
                        const std::string &body = "") {
        request req;
        req.is_valid = true;
        req.method = method;
        req.url = url;
        req.body = body;
        return req;
    }
} // namespace

TEST(ApiRequestHandlerTest, CreateStoresDataAndReturnsId) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp =
        handler.handle_request(MakeRequest("POST", "/api/Shoes", "{\"size\":10}"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("200 OK", resp->status_code);
    EXPECT_EQ("application/json", resp->content_type);
    EXPECT_EQ("{\"id\":0}", resp->body);

    std::string stored;
    ASSERT_TRUE(entity_store->Read("Shoes", 0, stored));
    EXPECT_EQ("{\"size\":10}", stored);
}

TEST(ApiRequestHandlerTest, GetReturnsStoredEntity) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    int id = entity_store->Create("Shoes", "{\"color\":\"red\"}");
    ASSERT_EQ(0, id);

    auto resp = handler.handle_request(
        MakeRequest("GET", "/api/Shoes/" + std::to_string(id)));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("200 OK", resp->status_code);
    EXPECT_EQ("{\"color\":\"red\"}", resp->body);
}

TEST(ApiRequestHandlerTest, ListReturnsExistingIds) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    int first = entity_store->Create("Books", "{\"title\":\"A\"}");
    int second = entity_store->Create("Books", "{\"title\":\"B\"}");
    ASSERT_EQ(0, first);
    ASSERT_EQ(1, second);

    auto resp = handler.handle_request(MakeRequest("GET", "/api/Books"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("200 OK", resp->status_code);
    EXPECT_EQ("[0,1]", resp->body);
}

TEST(ApiRequestHandlerTest, PutUpdatesExistingEntity) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    int id = entity_store->Create("Shoes", "{\"size\":9}");
    ASSERT_EQ(0, id);
    std::string payload = "{\"size\":11}";

    auto resp = handler.handle_request(
        MakeRequest("PUT", "/api/Shoes/" + std::to_string(id), payload));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("200 OK", resp->status_code);
    EXPECT_EQ("{\"id\":0}", resp->body);

    std::string stored;
    ASSERT_TRUE(entity_store->Read("Shoes", id, stored));
    EXPECT_EQ(payload, stored);
}

TEST(ApiRequestHandlerTest, DeleteRemovesEntity) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    int id = entity_store->Create("Shoes", "{\"size\":8}");
    ASSERT_EQ(0, id);

    auto resp = handler.handle_request(
        MakeRequest("DELETE", "/api/Shoes/" + std::to_string(id)));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("200 OK", resp->status_code);
    EXPECT_EQ("{\"deleted\":true}", resp->body);

    std::string stored;
    EXPECT_FALSE(entity_store->Read("Shoes", id, stored));
}

TEST(ApiRequestHandlerTest, GetMissingEntityReturnsNotFound) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp =
        handler.handle_request(MakeRequest("GET", "/api/Shoes/9999"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("404 Not Found", resp->status_code);
}

TEST(ApiRequestHandlerTest, DeleteMissingEntityReturnsNotFound) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp =
        handler.handle_request(MakeRequest("DELETE", "/api/Shoes/9999"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("404 Not Found", resp->status_code);
}

TEST(ApiRequestHandlerTest, BadMethodAndUrlCombinationsReturn400) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp = handler.handle_request(MakeRequest("GET", "/api"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);

    resp = handler.handle_request(MakeRequest("POST", "/api/Shoes/3"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);

    resp = handler.handle_request(MakeRequest("PUT", "/api/Shoes"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);

    resp = handler.handle_request(MakeRequest("DELETE", "/api/Shoes"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);
}

TEST(ApiRequestHandlerTest, UnsupportedMethodReturns405) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp =
        handler.handle_request(MakeRequest("CHEESE", "/api/Shoes/1"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("405 Method Not Allowed", resp->status_code);
}

TEST(ApiRequestHandlerTest, CreateWithNonJsonReturns400) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    auto resp =
        handler.handle_request(MakeRequest("POST", "/api/Shoes", "this payload is not JSON"));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);
    EXPECT_EQ("Body is not valid JSON", resp->body);

    std::string stored;
    ASSERT_FALSE(entity_store->Read("Shoes", 0, stored));
}

TEST(ApiRequestHandlerTest, PutWithNonJsonReturns400) {
    auto entity_store = std::make_shared<EntityStoreInMemory>();
    ApiRequestHandler handler(entity_store, "/api");

    std::string original_payload = "{\"size\":9}";
    int id = entity_store->Create("Shoes", original_payload);
    ASSERT_EQ(0, id);
    std::string payload = "this payload is not JSON";

    auto resp = handler.handle_request(
        MakeRequest("PUT", "/api/Shoes/" + std::to_string(id), payload));
    ASSERT_NE(nullptr, resp);
    EXPECT_EQ("400 Bad Request", resp->status_code);
    EXPECT_EQ("Body is not valid JSON", resp->body);

    // The rejected PUT should have left the entity unaffected.
    std::string stored;
    ASSERT_TRUE(entity_store->Read("Shoes", id, stored));
    EXPECT_EQ(original_payload, stored);
}
