#ifndef REQUEST_HANDLER_API_H
#define REQUEST_HANDLER_API_H

#include <memory>
#include <string>
#include <vector>

#include "entity_store.h"
#include "request_handler.h"

class ApiRequestHandler : public RequestHandler {
public:
    ApiRequestHandler(std::shared_ptr<EntityStore> entity_store,
                      const std::string &url_prefix = "");
    std::unique_ptr<response> handle_request(const request &request) override;
    std::string GetHandlerName() const override;

private:
    std::unique_ptr<response> HandlePost(const std::string &entity_name,
                                         const std::string &body);
    std::unique_ptr<response> HandleGet(const std::string &entity_name,
                                        const std::vector<std::string> &segments);
    std::unique_ptr<response> HandlePut(const std::string &entity_name, int id,
                                        const std::string &body);
    std::unique_ptr<response> HandleDelete(const std::string &entity_name, int id);
    std::vector<std::string> ExtractSegments(const std::string &raw_url) const;

    std::shared_ptr<EntityStore> entity_store_;
    std::string url_prefix_;
};

#endif // REQUEST_HANDLER_API_H
