#pragma once
#include <string>

typedef std::string String;
class UIManager;

class AUIScreen
{
protected:
	typedef std::string String;

	AUIScreen(String name);
	~AUIScreen();

	String getName();
	virtual void drawUI() = 0;
	void SetEnabled(bool flag);

	String name;
	bool enabled = true;
	friend class UIManager;
};

