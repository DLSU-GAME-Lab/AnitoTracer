#pragma once

#include "AUIScreen.h"

namespace gdeng03 {
	class PlaybackScreen : public AUIScreen
	{
	public:
		PlaybackScreen();
		~PlaybackScreen();

	private:
		friend class UIManager;

	private:
		virtual void drawUI() override;
	};
}