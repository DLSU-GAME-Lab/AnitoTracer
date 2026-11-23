#include "ModelFileOpener.hpp"

#include "HierarchyManager.hpp"
#include "ObjectFactory.hpp"
#include "Models/ModelManager.hpp"

namespace Diligent::FileOpeners
{
    Opener CreateModelFileOpener(StatusReporter reportStatus)
    {
        return [reportStatus = std::move(reportStatus)](const std::filesystem::path& path) {
            if (!HasExtension(path, ".obj"))
                return;

            const auto modelPath = NormalizePath(path);
            if (!ModelManager::GetAssetByPath(modelPath)) {
                if (reportStatus)
                    reportStatus("Model asset is not loaded.");
                return;
            }

            EditorCamera* editorCamera = HierarchyManager::GetInstance().GetEditorCamera();
            if (!editorCamera) {
                if (reportStatus)
                    reportStatus("Cannot place model without an editor camera.");
                return;
            }

            HierarchyObject::Ref created =
                ObjectFactory::GetInstance().CreateModelObject(path.stem().string(), modelPath.string());
            HierarchyObject* object = created.GetPtr();
            Transform* transform = object ? object->GetTransform() : nullptr;
            const ModelComponent* modelComponent = object ? object->GetComponent<ModelComponent>() : nullptr;
            if (!transform || !modelComponent || !modelComponent->HasModel()) {
                if (reportStatus)
                    reportStatus("Failed to create model object.");
                return;
            }

            transform->SetWorldPosition(editorCamera->GetPivotPosition());
            if (reportStatus)
                reportStatus("Model instantiated.");
        };
    }
}
