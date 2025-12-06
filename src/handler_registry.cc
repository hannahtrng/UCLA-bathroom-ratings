#include "handler_registry.h"
#include "handler_factory.h"
#include <boost/log/trivial.hpp>

// Define the static map
std::map<std::string, std::shared_ptr<HandlerFactory>> HandlerRegistry::_factories;

void HandlerRegistry::RegisterHandler(const std::string& path, std::shared_ptr<HandlerFactory> factory) {
    _factories[path] = factory;
}

std::shared_ptr<HandlerFactory> HandlerRegistry::GetFactory(const std::string& path) {
    auto it = _factories.find(path);
    if (it != _factories.end()) {
        // Return a shared copy of the stored factory
        return it->second;
    }
    BOOST_LOG_TRIVIAL(warning) << "HandlerRegistry: No factory found for path '" << path << "'";
    return nullptr;
}

