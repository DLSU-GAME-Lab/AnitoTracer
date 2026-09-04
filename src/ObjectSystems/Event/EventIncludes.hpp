#include "EventDefines.hpp"
#include "EventSystem.hpp"

//PER OBJECT TRIGGER TYPES
#include "Types/FixedUpdateTrigger.hpp"
#include "Types/LateUpdateTrigger.hpp"
#include "Types/EditorUpdateTrigger.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Types/OnGUI_Release.hpp"
#include "Types/OnGUI_Editor.hpp"

//ARG TYPES
#include "Args/SceneLoadArgs.hpp"
#include "Args/DefaultEventArgs.hpp"
#include "Args/HierarchyObjectArgs.hpp"

#include "ScopedSubscription.hpp"
#include "TriggerDispatcher.hpp"
