#pragma once

#include <functional>
#include <string>
#include <utility>

#include "ISerializable.hpp"
#include "SerializedData.hpp"
#include "PropertyDrawer.hpp"

namespace gbe {
    struct IAutoSerializer {

        std::string m_id;
        std::string m_display_name;

        virtual void Serialize(SerializedData& data) = 0;
        virtual void Deserialize(SerializedData& data) = 0;

        // --- Type Erasure API for Editor ---
        virtual bool DrawInspector() = 0;

        virtual ~IAutoSerializer() = default;
        virtual void RemoveFromInspector(ISerializable* _owner) = 0;
    };

    // =======================================================
    // BASE TEMPLATE: Holds all boilerplate, constructor, UI
    // =======================================================
    template <typename T>
    class AutoSerializerBase : public IAutoSerializer {
    public:
        using InitCallback = std::function<void(T&)>;

    protected:
        T* m_target;
        ISerializable* m_owner;
        InitCallback m_on_init;

    public:
        AutoSerializerBase(ISerializable* owner,
            std::string id,
            std::string display_name,
            T& target,
            InitCallback on_init = nullptr)
            : m_owner(owner), m_target(&target), m_on_init(std::move(on_init))
        {
            m_id = std::move(id);
            m_display_name = std::move(display_name);

            if (m_owner) {
                m_owner->RegisterProperty(this);
            }
        }

        // Enable move operations
        AutoSerializerBase(AutoSerializerBase&&) = default;
        AutoSerializerBase& operator=(AutoSerializerBase&&) = default;
        
        // Strictly delete copy operations
        AutoSerializerBase(const AutoSerializerBase&) = delete;
        AutoSerializerBase& operator=(const AutoSerializerBase&) = delete;


        ~AutoSerializerBase() override {
            if (m_owner) {
                RemoveFromInspector(m_owner);
            }
        }

        void RemoveFromInspector(ISerializable* _owner) override {
            _owner->UnRegisterProperty(this);
        }

        T& Get() { return *m_target; }
        const T& Get() const { return *m_target; }



        // --- Static Template Dispatch for ImGui ---
        bool DrawInspector() override {
            bool changed = PropertyDrawer<T>::Draw(m_display_name, this->Get());

            if (changed && m_on_init) {
                m_on_init(this->Get());
            }
            return changed;
        }
    };
}