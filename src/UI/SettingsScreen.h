#pragma once
#include "AUIScreen.h"
class SettingsScreen final :
    public AUIScreen
{
public:
	explicit SettingsScreen();
	virtual ~SettingsScreen();

protected:
	void drawUI() override;
};

