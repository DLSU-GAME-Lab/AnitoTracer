#include "EventHandler.hpp"
#include "InputMappedBool.hpp"

#define INPUTKEY_DOWN "INPUTKEY_DOWN"
#define INPUTKEY_UP "INPUTKEY_UP"
#define INPUTKEY_RIGHT "INPUTKEY_RIGHT"
#define INPUTKEY_LEFT "INPUTKEY_LEFT"
#define INPUTKEY_JUMP "INPUTKEY_JUMP"
#define INPUTKEY_SPRINT "INPUTKEY_SPRINT"
#define INPUTKEY_CROUCH "INPUTKEY_CROUCH"
#define INPUTKEY_PRIMARY "INPUTKEY_PRIMARY"
#define INPUTKEY_SECONDARY "INPUTKEY_SECONDARY"

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
		return glm::normalize(glm::vec2(x, y));
	}
};