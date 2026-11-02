#include "PrefabFileOpener.hpp"

#include "HierarchyFeatures/PrefabFeature.hpp"

namespace Diligent::FileOpeners
{
    Opener CreatePrefabFileOpener(StatusReporter reportStatus)
    {
        return [reportStatus = std::move(reportStatus)](const std::filesystem::path& path) {
            if (!HasExtension(path, ".aprefab"))
                return;

            const HierarchyObject::Ref created =
                PrefabFeature::GetInstance().InstantiatePrefab(NormalizePath(path));
            if (reportStatus)
                reportStatus(created ? "Prefab instantiated." : "Failed to instantiate prefab.");
        };
    }
}
