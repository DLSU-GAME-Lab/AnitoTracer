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
        mutable T* m_cachedPtr = nullptr; // Transient runtime cache

    public:
        // Default Constructor (Empty/Null reference)
        AssetRef() = default;

        // Construct from GUID
        AssetRef(const GUID& guid) : m_guid(guid) {}

        // Construct directly from Raw Asset Pointer
        AssetRef(T* asset)
            : m_guid(asset ? asset->GetGUID() : GUID::Empty()),
            m_cachedPtr(asset) {}

        // Copy & Move Operators
        AssetRef(const AssetRef& other) = default;
        AssetRef& operator=(const AssetRef& other) = default;

        AssetRef(AssetRef&& other) noexcept
            : m_guid(std::move(other.m_guid)), m_cachedPtr(other.m_cachedPtr) {
            other.m_cachedPtr = nullptr;
        }

        AssetRef& operator=(AssetRef&& other) noexcept {
            if (this != &other) {
                m_guid = std::move(other.m_guid);
                m_cachedPtr = other.m_cachedPtr;
                other.m_cachedPtr = nullptr;
            }
            return *this;
        }

        // Assign from Raw Asset Pointer
        AssetRef& operator=(T* asset) {
            m_guid = asset ? asset->GetGUID() : GUID::Empty();
            m_cachedPtr = asset;
            return *this;
        }

        // Getters / Setters
        GUID GetGUID() const { return m_guid; }

        void SetGUID(const GUID& guid) {
            if (m_guid != guid) {
                m_guid = guid;
                m_cachedPtr = nullptr; // Invalidate cache
            }
        }

        // Check if reference is populated
        bool IsEmpty() const { return m_guid == GUID::Empty(); }

        // Check if the referenced asset exists and is loaded
        bool IsValid() const { return Get() != nullptr; }

        // Pointer Accessors
        T* Get() const {
            if (m_guid == GUID::Empty()) return nullptr;

            // Resolve cached pointer or fetch from AssetDatabase
            if (!m_cachedPtr) {
                IAsset* baseAsset = AssetDatabase::GetAssetByGUID(m_guid);
                m_cachedPtr = dynamic_cast<T*>(baseAsset);
            }
            return m_cachedPtr;
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