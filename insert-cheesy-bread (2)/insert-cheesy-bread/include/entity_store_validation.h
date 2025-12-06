#ifndef ENTITY_STORE_VALIDATION_H
#define ENTITY_STORE_VALIDATION_H

#include <string>

// This code was partially created with the help of GPT-5.1-CODEX

// Provides helpers for validating entity names and identifiers shared between
// entity store implementations and API handlers.
class EntityStoreValidation {
public:
    static bool IsValidEntityName(const std::string &entity_name);
    static bool ParseId(const std::string &segment, int &out_id);
    static bool IsValidJson(const std::string &data);
};

#endif // ENTITY_STORE_VALIDATION_H
