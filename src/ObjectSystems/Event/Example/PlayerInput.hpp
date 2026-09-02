#include "EventHandler.hpp"
#include "InputMappedBool.hpp"
#include "InputSystem.hpp"

#define INPUTKEY_DOWN "INPUT_KEY_DOWN"
#define INPUTKEY_UP "INPUT_KEY_UP"
#define INPUTKEY_RIGHT "INPUT_KEY_RIGHT"
#define INPUTKEY_LEFT "INPUT_KEY_LEFT"
#define INPUTKEY_JUMP "INPUT_KEY_JUMP"
#define INPUTKEY_SPRINT "INPUT_KEY_SPRINT"
#define INPUTKEY_CROUCH "INPUT_KEY_CROUCH"
#define INPUTKEY_PRIMARY "INPUT_KEY_PRIMARY"
#define INPUTKEY_SECONDARY "INPUT_KEY_SECONDARY"

#include <glm/glm.hpp>

class PlayerInput {
public:
    // Declare member bools bound directly to action triggers
    GBE_INPUT_BOOL(isDown, INPUTKEY_DOWN);
    GBE_INPUT_BOOL(isUp, INPUTKEY_UP);
    GBE_INPUT_BOOL(isRight, INPUTKEY_RIGHT);
    GBE_INPUT_BOOL(isLeft, INPUTKEY_LEFT);
    GBE_INPUT_BOOL(isJumping, INPUTKEY_JUMP);
    GBE_INPUT_BOOL(isSprinting, INPUTKEY_SPRINT);
    GBE_INPUT_BOOL(isCrouching, INPUTKEY_CROUCH);
    GBE_INPUT_BOOL(isPrimary, INPUTKEY_PRIMARY);
    GBE_INPUT_BOOL(isSecondary, INPUTKEY_SECONDARY);

	glm::vec2 GetMovementVector() const {
		float x = static_cast<float>(isRight) - static_cast<float>(isLeft);
		float y = static_cast<float>(isUp) - static_cast<float>(isDown);
        return glm::vec2(x, y);
	}

    static inline void RegisterDefaultKeybinds() {
        gbe::InputSystem::RegisterMapping(INPUTKEY_DOWN, gbe::Key::S, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_UP, gbe::Key::W, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_RIGHT, gbe::Key::D, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_LEFT, gbe::Key::A, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_JUMP, gbe::Key::Space, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_PRIMARY, gbe::Key::MouseLeft, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_SECONDARY, gbe::Key::MouseRight, gbe::InputTrigger::All);
    }
};