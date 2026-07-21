#pragma once

class ComponentBase;

class IComponentUI {
public:
    virtual ~IComponentUI() = default;

    // Takes the base component, which the derived UI class will cast to the specific type.
    virtual void Draw(ComponentBase* component) = 0;
};