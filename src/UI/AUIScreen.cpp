#include "AUIScreen.h"

String AUIScreen::getName()
{
	return this->name;
}

void AUIScreen::setEnabled(const bool flag)
{
	this->enabled = flag;
}

void AUIScreen::toggleEnabled()
{
	this->enabled = !this->enabled;
}

AUIScreen::AUIScreen(const String& name)
{
	this->name = name;
}

AUIScreen::~AUIScreen()
= default;
