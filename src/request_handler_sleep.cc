#include "request_handler_sleep.h"
#include "response_builder.h"
#include <thread>
#include <chrono>

std::unique_ptr<RequestHandler> SleepHandler::create(const NginxConfig& config, const std::string& root_path) {
    return std::make_unique<SleepHandler>();
}

std::unique_ptr<response> SleepHandler::handle_request(const request &request) {
    // Sleep for 3 seconds to simulate a blocking operation
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return ResponseBuilder::Ok("Slept for 3 seconds.", "text/plain");
}