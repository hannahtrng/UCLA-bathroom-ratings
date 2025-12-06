#ifndef ENTITY_STORE_IN_MEMORY_H
#define ENTITY_STORE_IN_MEMORY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "entity_store.h"

class EntityStoreInMemory : public EntityStore {
public:
  EntityStoreInMemory();
  // Creates an object of the given entity type, returning a nonnegative integer
  // ID on success and a negative value on failure.
  virtual int Create(const std::string &entity_name,
                     const std::string &data) override;
  // If the object with the given ID exists, sets out_data to its data and
  // returns true. Otherwise returns false and does not modify out_data.
  virtual bool Read(const std::string &entity_name, int id,
                    std::string &out_data) override;
  // Updates the value of an object OR creates one with the given ID if none
  // exists. Returns true on success, false on failure.
  virtual bool Update(const std::string &entity_name, int id,
                      const std::string &data) override;
  // Deletes the object with the given ID, returning true on success. Returns
  // false if deletion fails (for example, if there is no object with ID = id).
  virtual bool Delete(const std::string &entity_name, int id) override;
  // Returns all of the IDs of objects of the given entity type.
  virtual std::vector<int> List(const std::string &entity_name) override;

private:
  std::unordered_map<std::string, std::unordered_map<int, std::string>>
      objects_by_id_by_entity_;
  std::mutex mutex_;
  int next_id_;
};

#endif // ENTITY_STORE_IN_MEMORY_H
