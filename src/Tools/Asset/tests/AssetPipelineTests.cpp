#include "Meta/GUID_meta.hpp"
#include "Meta/DynamicEnum_meta.hpp"
#include "Meta/IAsset_meta.hpp"
#include "AssetLoading/BatchLoader.hpp"
#include "AssetRef.hpp"

#include <fstream>
#include <memory>
#include <stdexcept>
#include <unordered_map>

class TestAsset : public gbe::IAsset {};

namespace glz {
    template <> struct meta<TestAsset> {
        static constexpr auto value = meta<gbe::IAsset>::value;
    };
}

namespace {
    void Check(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    void Write(const fs::path& path, const std::string& value) {
        std::ofstream out(path);
        out << value;
        if (!out) throw std::runtime_error("Could not create fixture");
    }

    std::string Read(const fs::path& path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), {});
    }
}

int main() {
    const auto root = fs::temp_directory_path() / ("anito-asset-tests-" + gbe::GUID::Generate().ToString());
    struct Cleanup {
        fs::path root;
        ~Cleanup() {
            std::error_code error;
            fs::remove_all(root, error);
            if (error) std::cerr << "Failed to clean test directory: " << error.message() << '\n';
        }
    } cleanup{root};
    try {
        const auto shared = root / "shared";
        const auto projectA = root / "projectA";
        const auto projectB = root / "projectB";
        fs::create_directories(shared);
        fs::create_directories(projectA / "models");
        fs::create_directories(projectB / "models");
        Write(shared / "shared.foo", "shared");
        Write(projectA / "models" / "asset.foo", "asset");

        std::unordered_map<std::string, std::unique_ptr<TestAsset>> cache;
        gbe::BatchLoader::RegisterCategory<TestAsset>("test", {".foo"}, ".ani",
            [&](const fs::path& path) {
                auto& asset = cache[path.string()];
                if (!asset) asset = std::make_unique<TestAsset>();
                return asset.get();
            });
        gbe::AssetDatabase::RegisterDirectory(shared);
        gbe::BatchLoader::ReloadDirectory(shared);
        auto* sharedAsset = gbe::AssetDatabase::GetAssetByPath(shared / "shared.foo");
        Check(sharedAsset != nullptr, "shared asset did not load");

        auto loadProject = [](const fs::path& project) {
            gbe::AssetDatabase::BeginProjectLoad(project);
            gbe::BatchLoader::ReloadDirectory(project);
            gbe::AssetDatabase::EndProjectLoad();
        };
        loadProject(projectA);
        const auto sourceA = projectA / "models" / "asset.foo";
        const auto metaA = projectA / "models" / "asset.foo.ani";
        auto* assetA = gbe::AssetDatabase::GetAssetByPath(sourceA);
        Check(assetA != nullptr, "project asset did not load");
        const auto guid = assetA->GetGUID();
        gbe::AssetRef<TestAsset> reference(guid);
        Check(reference.Get() == assetA, "GUID reference did not resolve");
        const auto metadata = Read(metaA);
        Check(metadata.find(root.string()) == std::string::npos, "new metadata contains an absolute path");
        Check(gbe::AssetDatabase::GetAssetByPath("models/asset.foo") == assetA, "relative lookup failed");
        Check(gbe::AssetDatabase::GetAssetByPath(root / "other" / "models" / "asset.foo") == nullptr,
            "unrelated absolute path matched by suffix");

        gbe::BatchLoader::ReloadDirectory(projectA);
        Check(reference.Get() == assetA, "reload changed the GUID");
        Check(Read(metaA) == metadata, "reload changed the metadata");

        fs::remove(metaA);
        gbe::BatchLoader::ReloadDirectory(projectA);
        Check(reference.Get() == assetA, "missing metadata changed a loaded GUID");
        Check(Read(metaA) == metadata, "missing metadata did not preserve the loaded GUID");

        Write(projectB / "models" / "asset.foo", "copy");
        Write(projectB / "models" / "asset.foo.ani", metadata);
        loadProject(projectB);
        auto* assetB = gbe::AssetDatabase::GetAssetByPath("models/asset.foo");
        Check(assetB && assetB != assetA && reference.Get() == assetB, "copied project did not retain GUID");
        Check(gbe::AssetDatabase::GetAssetByPath(sourceA) == nullptr, "old project remained registered");
        Check(gbe::AssetDatabase::GetAssetByPath(shared / "shared.foo") == sharedAsset, "shared asset was removed");
        Check(Read(projectB / "models" / "asset.foo.ani") == metadata, "copied metadata changed");

        const auto collision = projectB / "models" / "collision.foo";
        Write(collision, "collision");
        Write(collision.string() + ".ani", metadata);
        gbe::BatchLoader::ReloadDirectory(projectB);
        Check(reference.Get() == assetB, "collision replaced the original asset");
        Check(Read(collision.string() + ".ani") == metadata, "collision rewrote the GUID");
        Check(gbe::AssetDatabase::GetAssetByPath(collision) == nullptr, "collision was registered");

        const auto invalid = projectB / "bad.foo";
        Write(invalid, "bad");
        Write(invalid.string() + ".ani", "{broken");
        const auto orphan = projectB / "missing.foo.ani";
        Write(orphan, metadata);
        gbe::BatchLoader::ReloadDirectory(projectB);
        Check(Read(invalid.string() + ".ani") == "{broken", "invalid metadata was overwritten");
        Check(Read(orphan) == metadata, "orphan metadata was deleted");
        Check(gbe::AssetDatabase::GetAssetByPath(invalid) == nullptr, "invalid asset was registered");

        const auto moved = projectB / "models" / "renamed.foo";
        gbe::AssetDatabase::OnAssetMoved(projectB / "models" / "asset.foo", moved);
        Check(gbe::AssetDatabase::GetAssetByPath(moved) == assetB, "moved asset lookup failed");
        Check(gbe::AssetDatabase::GetAssetByPath(projectB / "models" / "asset.foo") == nullptr,
            "old moved path remained registered");
        gbe::AssetDatabase::Clear();
    }
    catch (const std::exception& e) {
        std::cerr << "Asset pipeline test failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
