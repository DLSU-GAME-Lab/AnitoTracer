#include "EventDefines.hpp"
#include "EventSystem.hpp"

//PER OBJECT TRIGGER TYPES
#include "Types/FixedUpdateTrigger.hpp"
#include "Types/LateUpdateTrigger.hpp"
#include "Types/EditorUpdateTrigger.hpp"
#include "Types/UpdateTrigger.hpp"

//ARG TYPES
#include "Args/SceneLoadArgs.hpp"
#include "Args/DefaultEventArgs.hpp"

#include "ScopedSubscription.hpp"
#include "TriggerDispatcher.hpp"
