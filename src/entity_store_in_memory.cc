#include "entity_store_in_memory.h"
#include <mutex>

EntityStoreInMemory::EntityStoreInMemory() : next_id_(0) {}

int EntityStoreInMemory::Create(const std::string &entity_name,
                                const std::string &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unordered_map<int, std::string> &objects_by_id =
      objects_by_id_by_entity_[entity_name];
  // Typically this loop runs 0 times, but the Update method can cause objects
  // to exist with IDs that collide with the next_id_ incrementing sequence.
  while (objects_by_id.count(next_id_)) {
    next_id_++;
  }
  int id = next_id_;
  next_id_++;
  objects_by_id[id] = data;
  return id;
}

bool EntityStoreInMemory::Read(const std::string &entity_name, int id,
                               std::string &out_data) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::unordered_map<int, std::string> &objects_by_id =
      objects_by_id_by_entity_[entity_name];
  auto object_it = objects_by_id.find(id);
  if (object_it != objects_by_id.end()) {
    out_data = object_it->second;
    return true;
  }
  return false;
}

bool EntityStoreInMemory::Update(const std::string &entity_name, int id,
                                 const std::string &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  objects_by_id_by_entity_[entity_name][id] = data;
  return true;
}

bool EntityStoreInMemory::Delete(const std::string &entity_name, int id) {
  std::lock_guard<std::mutex> lock(mutex_);
  size_t elements_erased = objects_by_id_by_entity_[entity_name].erase(id);
  return elements_erased != 0;
}

std::vector<int> EntityStoreInMemory::List(const std::string &entity_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::unordered_map<int, std::string> &objects_by_id =
      objects_by_id_by_entity_[entity_name];
  std::vector<int> ids;
  for (const auto &[id, _data] : objects_by_id) {
    ids.push_back(id);
  }
  return ids;
}
