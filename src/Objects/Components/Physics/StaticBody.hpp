#pragma once

#include "RigidBody.hpp"
#include "../Transform.hpp"
#include "../../HierarchyObject.hpp"

class StaticBody : public RigidBody {
public:
    StaticBody(gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : RigidBody(owner, 0.0f)  // Always mass=0 for static bodies
    {
        std::cout << "[DEBUG] StaticBody (mass=0 RigidBody) created" << std::endl;
    }

    ~StaticBody() override = default;

    StaticBody(const StaticBody&) = delete;
    StaticBody& operator=(const StaticBody&) = delete;
    StaticBody(StaticBody&&) = default;
    StaticBody& operator=(StaticBody&&) = default;

    virtual void Deserialize(gbe::SerializedData& data) override {
        RigidBody::Deserialize(data);
        // Force mass to always be 0 for static bodies
        mMass = 0.0f;
        if (mBody) mBody->SetMass(0.0f);
    } 

    virtual void SetMass(float mass) override {
        if (mBody) {
            mBody->SetMass(0.0f);
        }
        mMass = 0.0f;
    }

    virtual std::vector<std::string> GetHiddenProperties() const override {
        return { "mMass" };
    }

    virtual std::string GetLabel() override { return "StaticBody"; }

    bool WasAutoCreated() const { return mAutoCreated; }
    void MarkAutoCreated() { mAutoCreated = true;  }

protected:
    bool mAutoCreated = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(StaticBody, RigidBody);
};

GBE_REGISTER_SERIALIZED_TYPE(StaticBody, RigidBody);