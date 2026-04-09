#pragma once

#include "GuiElement.h"

#include <typeinfo>
#include <string>

#include <imgui.h>

namespace gbe {
	namespace editor {
		class GuiWindow : public GuiElement{
		protected:
			bool is_open = false;
			bool pointer_here = false;

			inline virtual void push_styles() {

			}
			inline virtual void pop_styles() {

			}
		private:
			bool ext_Begin() override;
			void ext_End() override;

		public:
			inline bool Get_pointer_here() {
				return pointer_here;
			}

			inline bool Get_is_open() {
				return is_open;
			}
			inline virtual void Set_is_open(bool newstate) {
				is_open = newstate;
			}
			virtual std::string GetWindowId() = 0;
		};
	}
}