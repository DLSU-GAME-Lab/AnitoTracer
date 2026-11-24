#include "../AnimationEditorDocument.hpp"
#include "../../FileOpeners/AnimationFileOpener.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
    void Check(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
}

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("anito-animation-editor-tests-" + gbe::GUID::Generate().ToString());
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{root};
    std::filesystem::create_directories(root);

    const auto path = root / "Example.anim";
    std::ofstream(path) << R"({"version":1,"name":"Example","duration":1.5,"frames_per_second":30,"loop":false,"tracks":[]})";

    Diligent::AnimationEditor::EditorDocument document;
    Check(document.Load(path), "animation document did not load");
    Check(!document.IsDirty(), "new document should be clean");
    document.GetClip().duration = 2.0f;
    document.MarkDirty();
    Check(document.IsDirty(), "animation document was not marked dirty");
    Check(document.Save(), "animation document did not save");
    Check(!document.IsDirty(), "saved animation document remained dirty");

    Diligent::AnimationEditor::EditorDocument reloaded;
    Check(reloaded.Load(path), "saved animation document did not reload");
    Check(reloaded.GetClip().duration == 2.0f, "saved duration did not round-trip");

    bool opened = false;
    std::filesystem::path received;
    auto opener = Diligent::FileOpeners::CreateAnimationFileOpener(
        [&](const std::filesystem::path& value) {
            opened = true;
            received = value;
        });
    opener(root / "clip.ANIM");
    Check(opened && received == root / "clip.ANIM", ".anim opener did not open animation extension");
    opened = false;
    opener(root / "clip.wav.ani");
    Check(!opened, ".anim opener accepted an unrelated metadata extension");
    return 0;
}
