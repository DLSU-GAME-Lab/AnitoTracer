#pragma once

#include <typeinfo>
#include <string>

namespace gbe {
	namespace editor {
		class GuiElement {
		public:
			void Draw();
		protected:
			virtual void DrawSelf() = 0;
		private:
			virtual bool ext_Begin() = 0;
			virtual void ext_End() = 0;
		};
	}
}