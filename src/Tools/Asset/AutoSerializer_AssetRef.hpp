#pragma once

#include "AutoSerializer.hpp"
#include "AssetRef.hpp" // Ensure this includes your AssetRef<T> implementation

namespace gbe {

    template <typename T>
    class AutoSerializer<AssetRef<T>> : public IAutoSerializer {
    private:
        using InitCallback = std::function<void(AssetRef<T>&)>;
        AssetRef<T>& m_target;
        ISerializable* m_owner;
        InitCallback m_on_init;

    public:
        
        AutoSerializer(ISerializable* owner,
            std::string id,
            std::string display_name,
            AssetRef<T>& target,
            InitCallback on_init = nullptr)
            : m_owner(owner), m_target(target), m_on_init(std::move(on_init))
        {
            m_id = std::move(id);
            m_display_name = std::move(display_name);

            if (m_owner) {
                m_owner->RegisterProperty(this);
            }
        }

        // Strictly delete copy operations
        AutoSerializer(const AutoSerializer&) = delete;
        AutoSerializer& operator=(const AutoSerializer&) = delete;

        ~AutoSerializer() override {
            if (m_owner) {
                RemoveFromInspector(m_owner);
            }
        }

        AssetRef<T>& Get() { return m_target; }
        const AssetRef<T>& Get() const { return m_target; }

        inline void Serialize(SerializedData& data) override {
            // Serializes the underlying GUID as a string
            data.serialized_variables.insert_or_assign(m_id, m_target.GetGUID().ToString());
        }

        inline void Deserialize(SerializedData& data) override {
            auto it = data.serialized_variables.find(m_id);
            if (it != data.serialized_variables.end()) {
                // Reconstructs GUID from string and updates the reference
                m_target.SetGUID(GUID::FromString(it->second));
            }

            if (m_on_init) {
                m_on_init(m_target);
            }
        }
    };

} // namespace gbe