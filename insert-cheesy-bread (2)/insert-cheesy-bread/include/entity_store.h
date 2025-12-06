#ifndef ENTITY_STORE_H
#define ENTITY_STORE_H

#include <string>
#include <vector>

class EntityStore {
public:
  virtual ~EntityStore() = default;
  // Creates an object, returning a nonnegative integer ID on success and a
  // negative value on failure.
  virtual int Create(const std::string &entity_name,
                     const std::string &data) = 0;
  // If the object with the given ID exists, sets out_data to its data and
  // returns true. Otherwise returns false and does not modify out_data.
  virtual bool Read(const std::string &entity_name, int id,
                    std::string &out_data) = 0;
  // Updates the value of an object OR creates one with the given ID if none
  // already exists. Returns true on success, false on failure.
  virtual bool Update(const std::string &entity_name, int id,
                      const std::string &data) = 0;
  // Deletes the object with the given ID, returning true on success. Returns
  // false if deletion fails (for example, if there is no object with ID = id).
  virtual bool Delete(const std::string &entity_name, int id) = 0;
  // Returns all of the IDs of objects of the given entity type.
  virtual std::vector<int> List(const std::string &entity_name) = 0;
};

#endif // ENTITY_STORE_H
