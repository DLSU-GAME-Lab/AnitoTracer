#pragma once

#include <filesystem>
#include <string>
#include "GUID.hpp"
#include "../Organization/DynamicEnum.hpp"

namespace gbe {

    struct AssetTypeTag {};
    using AssetType = DynamicEnum<AssetTypeTag>;

    class IAsset {
    public:
        virtual ~IAsset() = default;

        // --- GUID Accessors ---
        GUID GetGUID() const { return m_guid; } //
        void SetGUID(const GUID& guid) { m_guid = guid; } //[cite: 4]

        // --- Filepath Accessors ---
        const std::filesystem::path& GetPath() const { return m_path; } //[cite: 4]
        void SetPath(const std::filesystem::path& path) { m_path = path; } //[cite: 4]

        const std::filesystem::path& GetMetaPath() const { return m_metaPath; }
        void SetMetaPath(const std::filesystem::path& metaPath) { m_metaPath = metaPath; }

        // --- Asset Type Accessors ---
        AssetType GetAssetType() const { return m_assetType; }
        void SetAssetType(const AssetType& type) { m_assetType = type; }

        // --- Convenience Helpers ---
        // Returns the display/stem name (e.g., "PlayerTexture")
        std::string GetAssetId() const {
            return m_path.empty() ? "" : m_path.stem().string();
        }

    protected:
        GUID m_guid = GUID::Empty(); //[cite: 4]
        std::filesystem::path m_path; //[cite: 4]
        std::filesystem::path m_metaPath;
        AssetType m_assetType;
    };

} // namespace gbe