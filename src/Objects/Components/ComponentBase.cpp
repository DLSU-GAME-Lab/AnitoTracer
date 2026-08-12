#include "ComponentBase.hpp"

#include "HierarchyObject.hpp"

std::string ComponentBase::GetLabel() {
    return m_name + (m_owner ? (" (" + m_owner.GetPtr()->GetName() + ")") : "");
}