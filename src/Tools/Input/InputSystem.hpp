#pragma once

#include <string>
#include <array>

#include "InputEvent.hpp"

namespace gbe {
    class InputSystem {
    public:
        // ---------------------------------------------------------------------
        // TIER 3: DEFINING MAPPINGS (Static Access for UI / Game Logic)
        // ---------------------------------------------------------------------
        static void RegisterMapping(const std::string& actionName, Key key, InputTrigger trigger, KeyModifier modifiers = KeyModifier::None) {
            Instance().RegisterMappingInternal(actionName, key, trigger, modifiers);
        }

        static void ClearMappings() {
            Instance().ClearMappingsInternal();
        }

        // ---------------------------------------------------------------------
        // TIER 2: ROUTING INPUTS (Raw Ingestion from GLFW, ImGui, Win32, etc.)
        // ---------------------------------------------------------------------
        static void SetRawKeyState(Key key, bool isDown) {
            Instance().SetRawKeyStateInternal(key, isDown);
        }

        static void SetRawModifierState(KeyModifier modifier, bool active) {
            Instance().SetRawModifierStateInternal(modifier, active);
        }

        // ---------------------------------------------------------------------
        // TIER 4: CALLING FUNCTIONS (Tick Evaluation & Dispatch)
        // ---------------------------------------------------------------------
        static void Update() {
            Instance().UpdateInternal();
        }

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
            InputTrigger trigger;
            KeyModifier modifiers;
        };

        void RegisterMappingInternal(const std::string& actionName, Key key, InputTrigger trigger, KeyModifier modifiers) {
            mappings_.push_back({ actionName, key, trigger, modifiers });
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

                // 1. Check if required modifiers match active modifiers
                bool modifiersMatch = (static_cast<uint8_t>(currentModifiers_) & static_cast<uint8_t>(binding.modifiers))
                    == static_cast<uint8_t>(binding.modifiers);

                if (!modifiersMatch) continue;

                // 2. Evaluate Trigger Type
                bool shouldFire = false;
                switch (binding.trigger) {
                case InputTrigger::Down:
                    shouldFire = isCurrDown && !isPrevDown;
                    break;
                case InputTrigger::Up:
                    shouldFire = !isCurrDown && isPrevDown;
                    break;
                case InputTrigger::While:
                    shouldFire = isCurrDown;
                    break;
                }

                // 3. Dispatch to EventSystem
                if (shouldFire) {
                    EventSystem::DispatchTo(
                        binding.actionName,
                        std::make_unique<InputEventArgs>(binding.actionName, binding.key, binding.trigger, binding.modifiers)
                    );
                }
            }

            // Copy state to previous frame state at the end of tick
            previousKeyStates_ = currentKeyStates_;
        }

        std::array<bool, static_cast<size_t>(Key::COUNT)> currentKeyStates_;
        std::array<bool, static_cast<size_t>(Key::COUNT)> previousKeyStates_;
        KeyModifier currentModifiers_ = KeyModifier::None;
        std::vector<Mapping> mappings_;
    };
}