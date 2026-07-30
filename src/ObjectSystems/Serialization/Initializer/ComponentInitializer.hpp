#pragma once

#include "InstanceInitializer.hpp"
#include "TypeRegistry.hpp"

#include "Components/ComponentBase.hpp"

/// <summary>
/// Anito-engine implementation of instance initializer
/// </summary>
class ComponentInitializer : public gbe::InstanceInitializer<ComponentBase> {
	inline virtual ComponentBase* InitializeImpl(std::unique_ptr<ComponentBase> newcomp) override {
		return newcomp.release();
	}
};

GBE_REGISTER_INITIALIZER(ComponentInitializer);