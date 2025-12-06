#ifndef ENTITY_STORE_PERSISTENT_H
#define ENTITY_STORE_PERSISTENT_H

#include <filesystem>
#include <string>
#include <vector>

#include "entity_store.h"
#include "entity_store_validation.h"

// This code was partially created with the help of GPT-5.1-CODEX

// Stores entities persistently on disk.
// Each directory under data_path_ corresponds to an entity type
// Each file within an entity directory is named by the entity ID and contains the entity data
class EntityStorePersistent : public EntityStore {
public:
    explicit EntityStorePersistent(const std::string &data_path);
    int Create(const std::string &entity_name,
               const std::string &data) override;
    bool Read(const std::string &entity_name, int id,
              std::string &out_data) override;
    bool Update(const std::string &entity_name, int id,
                const std::string &data) override;
    bool Delete(const std::string &entity_name, int id) override;
    std::vector<int> List(const std::string &entity_name) override;

private:
    bool EnsureEntityDirectory(const std::string &entity_name) const;
    std::filesystem::path EntityDirectory(const std::string &entity_name) const;
    std::filesystem::path EntityFilePath(const std::string &entity_name,
                                         int id) const;

    std::filesystem::path data_root_;
};

#endif // ENTITY_STORE_PERSISTENT_H
