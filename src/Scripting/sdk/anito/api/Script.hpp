#pragma once

#include "Common.hpp"

namespace anito {
namespace detail {
inline Variant ToVariant(float v) { return Variant::Of(v); }
inline Variant ToVariant(double v) { return Variant::Of(static_cast<float>(v)); }
inline Variant ToVariant(int v) { return Variant::Of(v); }
inline Variant ToVariant(bool v) { return Variant::Of(v); }
inline Variant ToVariant(Vec3 v) { return Variant::Of(v); }
inline Variant ToVariant(const std::string& v) { return Variant::Of(v.c_str()); }
inline Variant ToVariant(const char* v) { return Variant::Of(v); }
inline Variant ToVariant(const Variant& v) { return v; }
inline Variant ToVariant(const GameObject& v);
} // namespace detail

class Script {
public:
    ObjectId id = 0;
    std::string name;

    bool valid() const { return !name.empty() && detail::H().script.Has(id, name.c_str()); }
    inline GameObject gameObject() const;

    // Invokes a script method on the target; returns the Variant result (Kind::None when the call fails).
    template <class... Args>
    Variant callRaw(const std::string& method, const Args&... args) const {
        const Variant packed[sizeof...(Args) + 1] = {detail::ToVariant(args)..., Variant{}};
        Variant ret;
        detail::H().script.Call(id, name.c_str(), method.c_str(), packed, static_cast<uint32_t>(sizeof...(Args)), &ret);
        return ret;
    }
    template <class... Args> void call(const std::string& method, const Args&... args) const { callRaw(method, args...); }
    template <class... Args> float callFloat(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsFloat(); }
    template <class... Args> int callInt(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsInt(); }
    template <class... Args> bool callBool(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsBool(); }
    template <class... Args> Vec3 callVec3(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsVec3(); }
    template <class... Args> GameObject callObject(const std::string& method, const Args&... args) const;

    bool operator==(const Script& o) const { return id == o.id && name == o.name; }
    bool operator!=(const Script& o) const { return !(*this == o); }
};
} // namespace anito
