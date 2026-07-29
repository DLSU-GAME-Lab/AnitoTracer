#pragma once

#include <memory>
#include <string>
#include <typeinfo>

namespace gbe {
	class IInstanceInitializer {
	public:
		virtual std::string GetTypeHash() = 0;
	};

	/// <summary>
	/// Engine-side interface to execute additional routing and set-up for newly created engine objects.
	/// </summary>
	template<typename T>
	class InstanceInitializer : public IInstanceInitializer {
	private:
		inline static InstanceInitializer* instance = nullptr;
		
	protected:

		void AssignSelfAsInitializer() {
			instance = this;
		}


	public:
		inline InstanceInitializer() {
			AssignSelfAsInitializer();
		}
		virtual ~InstanceInitializer() = default;

		virtual T* InitializeImpl(std::unique_ptr<T>) = 0;
		virtual std::string GetTypeHash() override{
			return typeid(T).name();
		}

		inline static T* Initialize(std::unique_ptr<T> newobj) {
			return instance->InitializeImpl(std::move(newobj));
		}
	};
}