#include "entity_store_persistent.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <unordered_set>

// This code was partially created with the help of GPT-5.1-CODEX

EntityStorePersistent::EntityStorePersistent(const std::string &data_path)
    : data_root_(data_path) {
    std::error_code ec;
    std::filesystem::create_directories(data_root_, ec);
}

int EntityStorePersistent::Create(const std::string &entity_name,
                                  const std::string &data) {
    if (!EntityStoreValidation::IsValidEntityName(entity_name) ||
        !EnsureEntityDirectory(entity_name)) {
        return -1;
    }

    // Gathers all ids in use
    std::filesystem::path entity_dir = EntityDirectory(entity_name);
    std::error_code ec;
    std::unordered_set<int> ids;
    for (const auto &entry :
         std::filesystem::directory_iterator(entity_dir, ec)) {
        if (entry.is_regular_file()) {
            int parsed_id;
            if (EntityStoreValidation::ParseId(
                    entry.path().filename().string(), parsed_id)) {
                ids.insert(parsed_id);
            }
        }
    }
    if (ec) {
        return -1;
    }

    // Finds the lowest unused id
    int next_id = 0;
    while (ids.count(next_id)) {
        next_id++;
    }

    // Creates a new file with the lowest unused id
    std::filesystem::path file_path =
        entity_dir / std::to_string(next_id);
    std::ofstream output(file_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return -1;
    }

    // Writes the data to the file
    output << data;

    output.close();
    if (!output.good()) {
        return -1;
    }

    return next_id;
}

bool EntityStorePersistent::Read(const std::string &entity_name, int id,
                                 std::string &out_data) {
    if (id < 0 || !EntityStoreValidation::IsValidEntityName(entity_name)) {
        return false;
    }
    std::filesystem::path file_path = EntityFilePath(entity_name, id);
    std::ifstream input(file_path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
    out_data = std::move(data);
    return true;
}

bool EntityStorePersistent::Update(const std::string &entity_name, int id,
                                   const std::string &data) {
    if (id < 0 || !EntityStoreValidation::IsValidEntityName(entity_name) ||
        !EnsureEntityDirectory(entity_name)) {
        return false;
    }

    // Overwrites or creates the file with the given id
    std::filesystem::path file_path = EntityFilePath(entity_name, id);
    std::ofstream output(file_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << data;

    output.close();
    if (!output.good()) {
        return false;
    }

    return true;
}

bool EntityStorePersistent::Delete(const std::string &entity_name, int id) {
    if (id < 0 || !EntityStoreValidation::IsValidEntityName(entity_name)) {
        return false;
    }
    std::filesystem::path file_path = EntityFilePath(entity_name, id);
    std::error_code ec;
    bool removed = std::filesystem::remove(file_path, ec);
    return !ec && removed;
}

// Gets all IDs for the given entity type
std::vector<int> EntityStorePersistent::List(
    const std::string &entity_name) {
    std::vector<int> ids;
    if (!EntityStoreValidation::IsValidEntityName(entity_name)) {
        return ids;
    }
    std::filesystem::path entity_dir = EntityDirectory(entity_name);
    std::error_code ec;
    if (!std::filesystem::exists(entity_dir, ec) || ec) {
        return ids;
    }
    for (const auto &entry :
         std::filesystem::directory_iterator(entity_dir, ec)) {
        if (entry.is_regular_file()) {
            int parsed_id;
            if (EntityStoreValidation::ParseId(
                    entry.path().filename().string(), parsed_id)) {
                ids.push_back(parsed_id);
            }
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool EntityStorePersistent::EnsureEntityDirectory(
    const std::string &entity_name) const {
    std::filesystem::path entity_dir = EntityDirectory(entity_name);
    std::error_code ec;
    std::filesystem::create_directories(entity_dir, ec);
    return !ec;
}

std::filesystem::path
EntityStorePersistent::EntityDirectory(
    const std::string &entity_name) const {
    return data_root_ / entity_name;
}

std::filesystem::path
EntityStorePersistent::EntityFilePath(const std::string &entity_name,
                                      int id) const {
    return EntityDirectory(entity_name) / std::to_string(id);
}
