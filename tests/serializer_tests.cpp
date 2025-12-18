#include <gtest/gtest.h>

#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/all.hpp"
#include "engine/core/components/transform.hpp"

TEST(SerializerTest, BasicSerialization) {
    tmt::Transform transform;
    transform.set_local_position(glm::vec3(1.0f, 2.0f, 3.0f));
    transform.set_local_rotation(glm::vec3(glm::radians(45.0f), glm::radians(90.0f), glm::radians(180.0f)));
    transform.set_local_scale(glm::vec3(2.0f, 2.0f, 2.0f));

    auto json = tmt::Serializer::serialize(transform);
    auto string = json.dump(4);

    tmt::Transform deserialized_transform;
    tmt::Serializer::deserialize(json, deserialized_transform);

    EXPECT_TRUE(deserialized_transform.get_local_position() == transform.get_local_position());
    EXPECT_TRUE(deserialized_transform.get_local_rotation() == transform.get_local_rotation());
    EXPECT_TRUE(deserialized_transform.get_local_scale() == transform.get_local_scale());
    EXPECT_TRUE(deserialized_transform.get_world_matrix() == transform.get_world_matrix());
}
