#include "entity_store_validation.h"

// Including boost/json.hpp is more consistent with our conventional usage of
// Boost, but the dev environment Docker image
// (https://code.cs130.org/plugins/gitiles/tools/+/refs/heads/main/env/base/Dockerfile)
// does not install libboost-json-dev and working around that would create toil.
#include <boost/json/src.hpp>
#include <cctype>

// This code was partially created with the help of GPT-5.1-CODEX

bool EntityStoreValidation::IsValidEntityName(const std::string &entity_name) {
    if (entity_name.empty()) {
        return false;
    }
    for (char c : entity_name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
              c == '-')) {
            return false;
        }
    }
    return true;
}

bool EntityStoreValidation::ParseId(const std::string &segment, int &out_id) {
    if (segment.empty()) {
        return false;
    }
    int result = 0;
    for (char c : segment) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
        result = result * 10 + (c - '0');
    }
    out_id = result;
    return true;
}

bool EntityStoreValidation::IsValidJson(const std::string &data) {
    boost::system::error_code ec;
    auto _discard = boost::json::parse(data, ec);
    return !ec;
}
