#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace gbe {

    template <typename Tag>
    class DynamicEnum {
    public:
        // Custom exception for unregistered lookup attempts
        class UnregisteredValueError : public std::invalid_argument {
        public:
            explicit UnregisteredValueError(const std::string& name)
                : std::invalid_argument("DynamicEnum error: Value '" + name + "' is not registered.") {}
        };

        // Registers a string value for this specific dynamic enum type.
        // Safe to call multiple times with the same string.
        static DynamicEnum register_value(const std::string& name) {
            auto& registry = get_registry();
            auto it = registry.string_to_id.find(name);
            if (it == registry.string_to_id.end()) {
                size_t id = registry.id_to_string.size();
                registry.string_to_id[name] = id;
                registry.id_to_string.push_back(name);
                return DynamicEnum(id);
            }
            return DynamicEnum(it->second);
        }

        // Helper to check registration state without throwing
        static bool is_registered(const std::string& name) {
            const auto& registry = get_registry();
            return registry.string_to_id.find(name) != registry.string_to_id.end();
        }

        // Constructs from string. Throws UnregisteredValueError if invalid.
        explicit DynamicEnum(const std::string& name) {
            const auto& registry = get_registry();
            auto it = registry.string_to_id.find(name);
            if (it == registry.string_to_id.end()) {
                throw UnregisteredValueError(name);
            }
            m_id = it->second;
        }

        // String representation recovery
        const std::string& str() const {
            return get_registry().id_to_string[m_id];
        }

        // Fast integer ID comparison
        size_t id() const { return m_id; }

        bool operator==(const DynamicEnum& other) const { return m_id == other.m_id; }
        bool operator!=(const DynamicEnum& other) const { return m_id != other.m_id; }
        bool operator<(const DynamicEnum& other) const { return m_id < other.m_id; }

        // Stream operator for convenient printing
        friend std::ostream& operator<<(std::ostream& os, const DynamicEnum& e) {
            return os << e.str();
        }

    private:
        explicit DynamicEnum(size_t id) : m_id(id) {}

        struct Registry {
            std::unordered_map<std::string, size_t> string_to_id;
            std::vector<std::string> id_to_string;

            //Always add a null tag
            Registry() {
                DynamicEnum::register_value("");
            }
        };
        inline static Registry instance;

        // Static registry isolated per unique Tag
        static Registry& get_registry() {
            return instance;
        }

        size_t m_id;
    };
}

#include <functional> // required for std::hash

namespace std {
    template <typename Tag>
    struct hash<gbe::DynamicEnum<Tag>> {
        std::size_t operator()(const gbe::DynamicEnum<Tag>& e) const noexcept {
            // e.id() returns a unique size_t for each registered string in the Tag registry
            return std::hash<std::size_t>{}(e.id());
        }
    };
}