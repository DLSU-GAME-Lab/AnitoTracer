#include "../EventDefines.hpp"
#include "EventHandler.hpp"

// 3. Create an EventHandler that listens for scene load events
class Print_OnSceneLoad : public gbe::EventHandler {
public:
    Print_OnSceneLoad() {
        // Subscribe using the defined macro
        this->SubscribeTo(EVENT_ONSCENELOAD, [](const std::unique_ptr<gbe::EventArgs>& args) {
            // Cast base pointer to our specific event payload
            auto sceneArgs = dynamic_cast<const SceneLoadArgs*>(args.get());
            if (sceneArgs) {
                std::cout << "[SceneManager] Scene loaded: " << sceneArgs->name << std::endl;
            }
            });
    }
};