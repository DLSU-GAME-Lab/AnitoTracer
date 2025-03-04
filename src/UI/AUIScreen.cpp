#include "AUIScreen.h"

String AUIScreen::getName()
{
	return this->name;
}

void AUIScreen::SetEnabled(bool flag)
{
	this->enabled = flag;
}

AUIScreen::AUIScreen(String name)
{
	this->name = name;
}

AUIScreen::~AUIScreen()
{
}
