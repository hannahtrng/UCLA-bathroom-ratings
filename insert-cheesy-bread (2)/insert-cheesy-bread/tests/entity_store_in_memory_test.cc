#include "gtest/gtest.h"

#include "entity_store_in_memory.h"

TEST(EntityStoreInMemoryTest, BasicFunctionality) {
    EntityStoreInMemory entity_store;
    std::string data = "123";
    int id = entity_store.Create("Shoes", data);
    ASSERT_GE(id, 0);

    std::vector<int> list = entity_store.List("Shoes");
    EXPECT_EQ(list, std::vector<int>{ id });

    std::string stored;
    ASSERT_TRUE(entity_store.Read("Shoes", id, stored));
    EXPECT_EQ(data, stored);

    EXPECT_TRUE(entity_store.Delete("Shoes", id));
    EXPECT_FALSE(entity_store.Delete("Shoes", id));
    EXPECT_FALSE(entity_store.Read("Shoes", id, stored));
}

TEST(EntityStoreInMemoryTest, CreateSkipsIDsCreatedByUpdate) {
    EntityStoreInMemory entity_store;
    ASSERT_TRUE(entity_store.Update("Shoes", 1, "\"one\""));
    int id = entity_store.Create("Shoes", "\"zero\"");
    ASSERT_EQ(id, 0);
    // ID 1 should be skipped
    int id2 = entity_store.Create("Shoes", "\"two\"");
    ASSERT_EQ(id2, 2);

    std::vector<int> list = entity_store.List("Shoes");
    std::sort(list.begin(), list.end());
    EXPECT_EQ(list, (std::vector<int>{0, 1, 2}));
}

TEST(EntityStoreInMemoryTest, EntityIDSpacesAreSeparate) {
    EntityStoreInMemory entity_store;
    int id_shoes = entity_store.Create("Shoes", "123");
    int id_shoes_2 = entity_store.Create("Shoes", "456");
    int id_people = entity_store.Create("People", "789");

    ASSERT_EQ(id_shoes, 0);
    ASSERT_EQ(id_shoes_2, 1);
    // This being 2 rather than 0 is a sign that the ID spaces are not as separate as
    // they could be. It stems from the implementation having a shared next_id_ rather
    // than one per entity. However, it should be OK by the spec because the example
    // it gives:
    // > (GET /api/Shoes/1 and GET /api/Books/1 can return separate objects, even though they both share ID 1)
    // is true, it's just that the user would have to create one or both of those with Update.
    ASSERT_EQ(id_people, 2);

    ASSERT_TRUE(entity_store.Update("People", 1, "0"));

    std::vector<int> list_shoes = entity_store.List("Shoes");
    std::sort(list_shoes.begin(), list_shoes.end());
    EXPECT_EQ(list_shoes, (std::vector<int>{0, 1}));

    std::vector<int> list_people = entity_store.List("People");
    std::sort(list_people.begin(), list_people.end());
    EXPECT_EQ(entity_store.List("People"), (std::vector<int>{1,2}));
}
