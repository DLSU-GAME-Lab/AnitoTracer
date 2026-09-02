#pragma once

#include <type_traits>
#include "IAsset.hpp"
#include "AssetDatabase.hpp"

namespace gbe {

    template <typename T>
    class AssetRef {
        static_assert(
            std::is_base_of_v<IAsset, T>,
            "AssetRef<T> error: Template parameter T must derive from gbe::IAsset!"
            );

    private:
        GUID m_guid = GUID::Empty();

    public:
        // Default Constructor (Empty/Null reference)
        AssetRef() = default;

        // Construct from GUID
        AssetRef(const GUID& guid) : m_guid(guid) {}

        // Construct directly from Raw Asset Pointer
        AssetRef(T* asset)
            : m_guid(asset ? asset->GetGUID() : GUID::Empty()) {}

        // Copy & Move Operators
        AssetRef(const AssetRef& other) = default;
        AssetRef& operator=(const AssetRef& other) = default;

        // Assign from Raw Asset Pointer
        AssetRef& operator=(T* asset) {
            m_guid = asset ? asset->GetGUID() : GUID::Empty();
            return *this;
        }

        // Getters / Setters
        GUID GetGUID() const { return m_guid; }

        void SetGUID(const GUID& guid) {
            m_guid = guid;
        }

        // Check if reference is populated
        bool IsEmpty() const { return m_guid == GUID::Empty(); }

        // Check if the referenced asset exists and is loaded
        bool IsValid() const { return Get() != nullptr; }

        // Pointer Accessors
        T* Get() const {
            if (m_guid == GUID::Empty()) return nullptr;
            return dynamic_cast<T*>(AssetDatabase::GetAssetByGUID(m_guid));
        }

        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }

        explicit operator bool() const { return IsValid(); }

        // Comparison Operators
        bool operator==(const AssetRef<T>& other) const { return m_guid == other.m_guid; }
        bool operator!=(const AssetRef<T>& other) const { return m_guid != other.m_guid; }

        bool operator==(std::nullptr_t) const { return !IsValid(); }
        bool operator!=(std::nullptr_t) const { return IsValid(); }

        // Serialization Hook (Format via JSON, YAML, or Binary Stream)
        template <typename Archive>
        void serialize(Archive& ar) {
            // Serializes only the GUID
            ar(m_guid);
        }
    };

} // namespace gbe