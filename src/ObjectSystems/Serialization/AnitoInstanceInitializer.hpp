#pragma once

#include "Internal/InstanceInitializer.hpp"
#include "Internal/TypeRegistry.hpp"

#include "HierarchyObject.hpp"

/// <summary>
/// Anito-engine implementation of instance initializer
/// </summary>
class AnitoInstanceInitializer : public gbe::InstanceInitializer<HierarchyObject> {
	virtual HierarchyObject* InitializeImpl(std::unique_ptr<HierarchyObject>) override;
};

GBE_REGISTER_INITIALIZER(AnitoInstanceInitializer);