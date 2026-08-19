#pragma once

#include "IPhysicsEngine.hpp"
#include "JoltPhysicsEngine.hpp"

class PhysicsEngine {
public:
	static PhysicsEngine& GetInstance() {
		static PhysicsEngine instance;
		return instance;
	}

	PhysicsEngine(const PhysicsEngine&) = delete;
	PhysicsEngine& operator=(const PhysicsEngine&) = delete;
	PhysicsEngine(PhysicsEngine&&) = delete;
	PhysicsEngine& operator=(PhysicsEngine&&) = delete;

	IPhysicsEngine& Get() { return *mEngine; }

private:
	PhysicsEngine() : mEngine(std::make_unique<JoltPhysicsEngine>()) {}
	~PhysicsEngine() = default;

	std::unique_ptr<IPhysicsEngine> mEngine;
};