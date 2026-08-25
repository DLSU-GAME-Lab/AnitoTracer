#pragma once

#include "PhysicsBase.hpp"
#include "../Transform.hpp"
#include "../../HierarchyObject.hpp"

class StaticBody : public PhysicsBase {
public:
	StaticBody(gbe::IInstanceManager<HierarchyObject>::Ref owner = {}) 
		: PhysicsBase("StaticBody", owner) 
	{
		glm::vec3 pos(0.0f);
		glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
		if (HierarchyObject* o = m_owner.GetPtr()) {
			if (Transform* t = o->GetTransform()) {
				pos = t->GetPosition();
				rot = t->GetRotation();
			}
		}
		CreateBody(pos, rot, 0.0f); // Mass is zero for static bodies
	}

	~StaticBody() override = default;

	StaticBody(const StaticBody&) = delete;
	StaticBody& operator=(const StaticBody&) = delete;
	StaticBody(StaticBody&&) = default;
	StaticBody& operator=(StaticBody&&) = default;

	virtual std::string GetLabel() override { return "StaticBody"; }
	
	GBE_GENERATE_SERIALIZER_CONSTRUCTOR(StaticBody, PhysicsBase);
};

GBE_REGISTER_SERIALIZED_TYPE(StaticBody, PhysicsBase);