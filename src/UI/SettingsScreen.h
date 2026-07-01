#pragma once
#include "AUIScreen.h"
#include <glm/gtc/type_ptr.hpp>

class SettingsScreen final :
    public AUIScreen
{
public:
	explicit SettingsScreen();
	virtual ~SettingsScreen();

protected:
	void drawUI() override;
};

