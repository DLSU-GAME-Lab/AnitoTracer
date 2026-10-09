// Hand-written stand-in for the transpiler output of SpinAndBob.ascript.
#include "AnitoScriptSDK.hpp"

#include <cmath>
#include <cstring>

using namespace anito;

class SpinAndBob final : public ScriptBehaviour {
public:
    explicit SpinAndBob(const ScriptContext& ctx)
        : ScriptBehaviour(ctx),
          spinSpeed(*static_cast<float*>(ctx.fields[0])),
          spinAxis(*static_cast<Vec3*>(ctx.fields[1])),
          bobHeight(*static_cast<float*>(ctx.fields[2])),
          bobFrequency(*static_cast<float*>(ctx.fields[3])),
          enabled(*static_cast<bool*>(ctx.fields[4])),
          tag(*static_cast<std::string*>(ctx.fields[5])),
          spinCount(*static_cast<int*>(ctx.fields[6])) {}

    void OnUpdate(float dt) override {
        if (!enabled) { return; }
        time = time + dt;
        Rotate(spinAxis, spinSpeed * dt);
        Vec3 p = GetPosition();
        p.y = baseY + std::sin(time * bobFrequency) * bobHeight;
        SetPosition(p);
    }

    bool Invoke(const char* method) override {
        if (std::strcmp(method, "resetTime") == 0) { resetTime(); return true; }
        return false;
    }

private:
    void resetTime() {
        time = 0.0f;
        spinCount = spinCount + 1;
    }

    float& spinSpeed;
    Vec3& spinAxis;
    float& bobHeight;
    float& bobFrequency;
    bool& enabled;
    std::string& tag;
    int& spinCount;

    float time = 0.0f;
    float baseY = 0.0f;
};

static const FieldType kSpinAndBobFields[] = {
    FieldType::Float, FieldType::Vec3, FieldType::Float, FieldType::Float,
    FieldType::Bool, FieldType::String, FieldType::Int,
};

static const ScriptEntry kEntries[] = {
    {"SpinAndBob",
     [](const ScriptContext& ctx) -> ScriptBehaviour* { return new SpinAndBob(ctx); },
     [](ScriptBehaviour* instance) { delete instance; },
     7, kSpinAndBobFields},
};

static const ModuleInfo kModule = {ANITO_SCRIPT_ABI_VERSION, 1, kEntries};

ANITO_SCRIPT_EXPORT const ModuleInfo* AnitoScript_GetModule() { return &kModule; }
