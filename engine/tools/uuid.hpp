#pragma once
#include <random>
#include "uuid_v4.h"
#include "engine/core/reflection.hpp"

namespace tmt {

using UUID = UUIDv4::UUID;

static inline const UUID NULL_UUID(0, 0);

class UUIDGenerator : public UUIDv4::UUIDGenerator<std::mt19937_64> {
   public:
    UUIDGenerator() : UUIDv4::UUIDGenerator<std::mt19937_64>() {}
    UUIDGenerator(uint64_t seed) : UUIDv4::UUIDGenerator<std::mt19937_64>(seed) {}

    static UUID generate() {
        static UUIDGenerator generator;
        return generator.getUUID();
    }
};

}  // namespace tmt

FMT_LOGGING(tmt::UUID, "{}", obj.str());
TMT_COMPONENT_NAME(tmt::UUID, "UUID");