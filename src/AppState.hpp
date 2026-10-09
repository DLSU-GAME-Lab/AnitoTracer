#pragma once

// Runtime play-mode state. Unlike AppConfig (init-time only, set from launch
// args), these flags are checked continuously through the update/render loop
// and can change while the application is running.
struct AppState {
    // True while the engine is simulating gameplay (Update/FixedUpdate,
    // physics, release-style rendering). Toggled by the in-editor Play/Stop
    // control; stopping reloads the current scene file to discard changes.
    inline static bool isPlaying = false;

    // Seeded once from AppConfig::release during initialization. True means
    // this process was launched as a standalone player, so isPlaying is
    // perpetually true and can never be stopped back to editor mode.
    inline static bool isReleaseBuild = false;
};
