#include "EventHandler.hpp"
#include "InputMappedBool.hpp"
#include "InputSystem.hpp"

#define INPUTKEY_THRUST_FORWARD "INPUT_KEY_THRUST_FORWARD"
#define INPUTKEY_THRUST_BACKWARD "INPUT_KEY_THRUST_BACKWARD"
#define INPUTKEY_ROLL_LEFT "INPUT_KEY_ROLL_LEFT"
#define INPUTKEY_ROLL_RIGHT "INPUT_KEY_ROLL_RIGHT"
#define INPUTKEY_PITCH_DOWN "INPUT_KEY_PITCH_DOWN"
#define INPUTKEY_PITCH_UP "INPUT_KEY_PITCH_UP"
#define INPUTKEY_YAW_LEFT "INPUT_KEY_YAW_LEFT"
#define INPUTKEY_YAW_RIGHT "INPUT_KEY_YAW_RIGHT"
#define INPUTKEY_PRIMARY "INPUT_KEY_PRIMARY"
#define INPUTKEY_SECONDARY "INPUT_KEY_SECONDARY"

#include <glm/glm.hpp>

class PlayerInput {
public:
    GBE_INPUT_BOOL(isThrustingForward, INPUTKEY_THRUST_FORWARD);
    GBE_INPUT_BOOL(isThrustingBackward, INPUTKEY_THRUST_BACKWARD);
    GBE_INPUT_BOOL(isRollingLeft, INPUTKEY_ROLL_LEFT);
    GBE_INPUT_BOOL(isRollingRight, INPUTKEY_ROLL_RIGHT);
    GBE_INPUT_BOOL(isPitchingDown, INPUTKEY_PITCH_DOWN);
    GBE_INPUT_BOOL(isPitchingUp, INPUTKEY_PITCH_UP);
    GBE_INPUT_BOOL(isYawingLeft, INPUTKEY_YAW_LEFT);
    GBE_INPUT_BOOL(isYawingRight, INPUTKEY_YAW_RIGHT);
    GBE_INPUT_BOOL(isPrimary, INPUTKEY_PRIMARY);
    GBE_INPUT_BOOL(isSecondary, INPUTKEY_SECONDARY);

    static inline void RegisterDefaultKeybinds() {
        gbe::InputSystem::RegisterMapping(INPUTKEY_THRUST_FORWARD, gbe::Key::W, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_THRUST_BACKWARD, gbe::Key::S, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_ROLL_LEFT, gbe::Key::Q, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_ROLL_RIGHT, gbe::Key::E, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_PITCH_DOWN, gbe::Key::Space, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_PITCH_UP, gbe::Key::Shift, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_YAW_LEFT, gbe::Key::D, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_YAW_RIGHT, gbe::Key::A, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_PRIMARY, gbe::Key::MouseLeft, gbe::InputTrigger::All);
        gbe::InputSystem::RegisterMapping(INPUTKEY_SECONDARY, gbe::Key::MouseRight, gbe::InputTrigger::All);
    }
};