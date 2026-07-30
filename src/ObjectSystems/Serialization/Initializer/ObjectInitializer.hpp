#pragma once

#include "InstanceInitializer.hpp"
#include "TypeRegistry.hpp"

#include "HierarchyObject.hpp"

/// <summary>
/// Anito-engine implementation of instance initializer
/// </summary>
class ObjectInitializer : public gbe::InstanceInitializer<HierarchyObject> {
	inline virtual HierarchyObject* InitializeImpl(std::unique_ptr<HierarchyObject> newptr) override {
		return newptr.release();
	}
};

GBE_REGISTER_INITIALIZER(ObjectInitializer);