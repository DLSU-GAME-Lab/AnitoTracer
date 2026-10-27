#pragma once

#include "Objects.hpp"

namespace anito {
// ================================================================================================
// ScriptBehaviour
// ================================================================================================
class ScriptBehaviour {
public:
    explicit ScriptBehaviour(const ScriptContext& ctx) : m_ctx(ctx), gameObject{ctx.object} { detail::HostPtr() = ctx.host; }
    virtual ~ScriptBehaviour() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float /*dt*/) {}
    virtual void OnFixedUpdate(float /*dt*/) {}
    virtual void OnGUI(float /*dt*/) {}
    virtual void OnCollisionEnter(GameObject /*other*/) {}
    virtual bool Invoke(const char* /*method*/) { return false; }
    virtual bool Call(const char* /*method*/, const Variant* /*args*/, uint32_t /*argc*/, Variant* /*ret*/) { return false; }

protected:
    ScriptContext m_ctx;

public:
    GameObject gameObject;
};

} // namespace anito
