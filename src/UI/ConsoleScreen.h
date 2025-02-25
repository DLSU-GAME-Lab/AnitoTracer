#pragma once
#include "AUIScreen.h"
#include "imgui.h"
class ConsoleScreen :    public AUIScreen
{
public:
	ConsoleScreen();
	~ConsoleScreen();
	void appendText(String text);
private:
	virtual void drawUI() override;
	friend class UIManager;

	ImGuiTextBuffer* textLog;
	int lineCount = 0;
};

