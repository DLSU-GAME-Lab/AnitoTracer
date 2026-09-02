#pragma once

#include <string>
#include <array>

#include "InputEvent.hpp"

namespace gbe {
    class InputSystem {
    public:
        struct Vec2 {
            float x = 0.0f;
            float y = 0.0f;
        };

        // Tier 3: Register mappings with default InputTrigger::All (Down | While | Up)
        static void RegisterMapping(const std::string& actionName, Key key, InputTrigger triggers = InputTrigger::All, KeyModifier modifiers = KeyModifier::None) {
            Instance().RegisterMappingInternal(actionName, key, triggers, modifiers);
        }

        static void ClearMappings() {
            Instance().ClearMappingsInternal();
        }

        // Tier 2: Ingestion from raw inputs
        static void SetRawKeyState(Key key, bool isDown) {
            Instance().SetRawKeyStateInternal(key, isDown);
        }

        static void SetRawModifierState(KeyModifier modifier, bool active) {
            Instance().SetRawModifierStateInternal(modifier, active);
        }

        // Tier 4: Process frame transitions and dispatch events
        static void Update() {
            Instance().UpdateInternal();
        }

        // Setters using native types
        static void SetMouseDelta(float x, float y) { s_mouseDelta = { x, y }; }
        static void SetMouseDelta(Vec2 delta) { s_mouseDelta = delta; }

        static void SetMousePosition(float x, float y) { s_mousePosition = { x, y }; }
        static void SetMousePosition(Vec2 pos) { s_mousePosition = pos; }

        // Native getters
        static Vec2 GetMouseDelta() { return s_mouseDelta; }
        static Vec2 GetMousePosition() { return s_mousePosition; }

    private:
        InputSystem() {
            currentKeyStates_.fill(false);
            previousKeyStates_.fill(false);
        }

        static InputSystem& Instance() {
            static InputSystem instance;
            return instance;
        }

        struct Mapping {
            std::string actionName;
            Key key;
            InputTrigger mask;
            KeyModifier modifiers;
        };

        void RegisterMappingInternal(const std::string& actionName, Key key, InputTrigger mask, KeyModifier modifiers) {
            mappings_.push_back({ actionName, key, mask, modifiers });
        }

        void ClearMappingsInternal() {
            mappings_.clear();
        }

        void SetRawKeyStateInternal(Key key, bool isDown) {
            size_t index = static_cast<size_t>(key);
            if (index < currentKeyStates_.size()) {
                currentKeyStates_[index] = isDown;
            }
        }

        void SetRawModifierStateInternal(KeyModifier modifier, bool active) {
            uint8_t current = static_cast<uint8_t>(currentModifiers_);
            uint8_t modBit = static_cast<uint8_t>(modifier);

            if (active) {
                currentModifiers_ = static_cast<KeyModifier>(current | modBit);
            }
            else {
                currentModifiers_ = static_cast<KeyModifier>(current & ~modBit);
            }
        }

        void UpdateInternal() {
            for (const auto& binding : mappings_) {
                size_t keyIndex = static_cast<size_t>(binding.key);
                bool isCurrDown = currentKeyStates_[keyIndex];
                bool isPrevDown = previousKeyStates_[keyIndex];

                // Modifier Check
                bool modifiersMatch = (static_cast<uint8_t>(currentModifiers_) & static_cast<uint8_t>(binding.modifiers))
                    == static_cast<uint8_t>(binding.modifiers);

                if (!modifiersMatch) continue;

                // Frame State Conditions
                bool isDownState = isCurrDown && !isPrevDown;
                bool isWhileState = isCurrDown;
                bool isUpState = !isCurrDown && isPrevDown;

                auto dispatchPhase = [&](InputTrigger phase, const std::string& phaseSuffix) {
                    bool enabledInMask = static_cast<uint8_t>(binding.mask & phase) != 0;
                    if (enabledInMask) {
                        // 1. Dispatch to global channel (e.g., "ChargeAttack")
                        EventSystem::DispatchTo(
                            binding.actionName,
                            std::make_unique<InputEventArgs>(binding.actionName, binding.key, phase, binding.modifiers)
                        );

                        // 2. Dispatch to phase-specific sub-channel (e.g., "ChargeAttack:Down")
                        EventSystem::DispatchTo(
                            binding.actionName + ":" + phaseSuffix,
                            std::make_unique<InputEventArgs>(binding.actionName, binding.key, phase, binding.modifiers)
                        );
                    }
                    };

                if (isDownState)  dispatchPhase(InputTrigger::Down, "Down");
                if (isWhileState) dispatchPhase(InputTrigger::While, "While");
                if (isUpState)    dispatchPhase(InputTrigger::Up, "Up");
            }

            previousKeyStates_ = currentKeyStates_;
        }

        static inline Vec2 s_mouseDelta{ 0.0f, 0.0f };
        static inline Vec2 s_mousePosition{ 0.0f, 0.0f };

        std::array<bool, static_cast<size_t>(Key::COUNT)> currentKeyStates_;
        std::array<bool, static_cast<size_t>(Key::COUNT)> previousKeyStates_;
        KeyModifier currentModifiers_ = KeyModifier::None;
        std::vector<Mapping> mappings_;
    };
}