#pragma once
#include <any>
#include <map>
#include <typeindex>
#include "engine/tools/serializer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
    #include <ImReflect.hpp>
    #include "editor/imgui/types/all.hpp"
    #include "editor/imgui/components/all.hpp"
#endif

namespace tmt {

class PlayerData {
    struct Config {
        static inline const IO::FileLocation FILE_LOCATION { IO::Location::USERDATA, "PlayerData.json" };
    };

    struct TypeOps {
        std::type_index type_index = typeid(void);
        std::function<tmt::json(const std::any&)> serialize = nullptr;
        std::function<void(const tmt::json&, std::any&)> deserialize = nullptr;
        std::function<void(const char* label, std::any&)> inspect = nullptr;
    };

    struct Entry {
        std::any value = {};
        TypeOps ops = {};
    };

    std::map<std::string, Entry> entries;
    ResourceRef<Json> data;

   public:
    /* Important! Type needs to be reflected */
    template <typename T>
    T& get(const std::string& key, T default_value = {}) {
        /* Check if it's already loaded */
        const bool contains = entries.contains(key);
        if (contains == false) {
            /* Default emplace */
            auto ops_entry = Entry {
                .value = default_value,
                .ops = create_ops<T>(),
            };
            entries.emplace(key, ops_entry);

            /* Check if json, else use default */
            const bool has_json = data && data->get_parsed_json().contains(key);
            if (has_json) {
                const tmt::json& j = data->get_parsed_json().at(key);
                auto& entry = entries.at(key);
                try {
                    entry.ops.deserialize(j, entry.value);
                } catch (const std::exception& e) {
                    tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to deserialize key '{}': {}. Using default value.", key, e.what());
                }
            }
        }

        auto& entry = entries.at(key);
        if (entry.ops.type_index != std::type_index(typeid(T))) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Type mismatch for key '{}'. Requested type: '{}', actual type: '{}'. Overriding...", key, typeid(T).name(), entry.ops.type_index.name());
            entries.erase(key);
            entries.emplace(
                key, Entry {
                         .value = default_value,
                         .ops = create_ops<T>(),
                     }
            );
            return std::any_cast<T&>(entries.at(key).value);
        }
        return std::any_cast<T&>(entry.value);
    }

    void serialize() {
        /* Update JSON data from entries */
        for (const auto& [key, entry] : entries) {
            try {
                data->get_parsed_json()[key] = entry.ops.serialize(entry.value);
            } catch (const std::exception& e) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to serialize key '{}': {}", key, e.what());
            }
        }

        /* Write to disk */
        const bool success = IO::write_text_file(Config::FILE_LOCATION, data->get_parsed_json().dump(4));
        if (!success) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to write PlayerData to file '{}'", Config::FILE_LOCATION);
        }
    }

    void init() {
        data = engine.resources.load_resource<Json>(Config::FILE_LOCATION);
        if (!data) {
            /* Create file and load */
            IO::write_text_file(Config::FILE_LOCATION, "{}");
            data = engine.resources.load_resource<Json>(Config::FILE_LOCATION);
        }
    }

    void clear() {
        entries.clear();
        if (data) {
            data->get_parsed_json().clear();
            IO::write_text_file(Config::FILE_LOCATION, "{}");
        }
    }

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
    void inspect() {
        for (auto& [key, entry] : entries) {
            if (entry.ops.inspect == nullptr) {
                ImGui::Text("Key '%s' with type '%s' has not been called yet or is engine struct.", key.c_str(), entry.ops.type_index.name());
                continue;
            }
            if (entry.value.has_value() == false) {
                ImGui::Text("Key '%s' with type '%s' has no value.", key.c_str(), entry.ops.type_index.name());
                continue;
            }
            try {
                entry.ops.inspect(key.c_str(), entry.value);
            } catch (const std::exception& e) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to inspect key '{}': {}", key, e.what());
            }
        }
    }
#endif

   private:
    template <typename T>
    TypeOps create_ops() {
        return TypeOps {
            std::type_index(typeid(T)),
            [](const std::any& value) -> tmt::json {
                //
                return tmt::Serializer::serialize(std::any_cast<T>(value));
            },
            [](const tmt::json& j, std::any& value) {
                //
                tmt::Serializer::deserialize(j, std::any_cast<T&>(value));
            },
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
            [](const char* label, std::any& value) {
                //
                ImReflect::Input(label, std::any_cast<T&>(value));
            },
#endif
        };
    }
};

}  // namespace tmt