#include "EventHandler.hpp"

namespace gbe {

    class InputMappedBool : public EventHandler {
    public:
        explicit MappedBool(const std::string& actionName)
            : isPressed_(false) {

            // Automatically set state to true on Down
            SubscribeTo(actionName + ":Down", [this](const std::unique_ptr<EventArgs>&) {
                isPressed_ = true;
                });

            // Automatically set state to false on Up
            SubscribeTo(actionName + ":Up", [this](const std::unique_ptr<EventArgs>&) {
                isPressed_ = false;
                });
        }

        // Implicit conversion to standard bool for seamless if(isJumping) syntax
        operator bool() const { return isPressed_; }

        // Explicit getter
        [[nodiscard]] bool Get() const { return isPressed_; }

    private:
        bool isPressed_ = false;
    };

} // namespace gbe

// Macro to declare self-initializing mapped bool member variables
#define GBE_INPUT_BOOL(varName, actionName) \
    ::gbe::InputMappedBool varName{ actionName }