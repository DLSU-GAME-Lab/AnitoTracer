#pragma once

#if defined(_WIN32) || defined(PLATFORM_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <imgui.h>

class CursorManager {
public:
	static CursorManager& GetInstance() {
		static CursorManager instance;
		return instance;
	}

	// Initialize with the HWND from your app setup
	void Initialize(HWND hWnd) {
		m_hWnd = hWnd;
	}

	// Lock or unlock the cursor
	void SetCursorLock(bool locked) {
		if (m_isLocked == locked) return;
		m_isLocked = locked;

		ImGuiIO& io = ImGui::GetIO();

		if (m_isLocked) {
			// 1. Prevent ImGui from overriding system cursor state
			io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);

#if defined(_WIN32) || defined(PLATFORM_WIN32)
			if (m_hWnd) {
				// 2. Hide cursor (loop ensures counter goes negative)
				while (::ShowCursor(FALSE) >= 0);

				// 3. Lock cursor within window client area
				UpdateClipRect();
			}
#endif
		}
		else {
			// Restore ImGui mouse control
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
			ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

#if defined(_WIN32) || defined(PLATFORM_WIN32)
			if (m_hWnd) {
				// Release cursor bounds
				::ClipCursor(NULL);

				// Show cursor (loop ensures counter goes non-negative)
				while (::ShowCursor(TRUE) < 0);
			}
#endif
		}
	}

	void ToggleLock() {
		SetCursorLock(!m_isLocked);
	}

	bool IsLocked() const {
		return m_isLocked;
	}

	// Recalculates screen bounds (call if window is resized/moved while locked)
	void UpdateClipRect() {
#if defined(_WIN32) || defined(PLATFORM_WIN32)
		if (m_isLocked && m_hWnd) {
			RECT rect;
			::GetClientRect(m_hWnd, &rect);

			POINT topLeft{ rect.left, rect.top };
			POINT bottomRight{ rect.right, rect.bottom };

			::ClientToScreen(m_hWnd, &topLeft);
			::ClientToScreen(m_hWnd, &bottomRight);

			RECT screenRect{ topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
			::ClipCursor(&screenRect);

			// Position cursor at center of client window
			int centerX = topLeft.x + (rect.right - rect.left) / 2;
			int centerY = topLeft.y + (rect.bottom - rect.top) / 2;
			::SetCursorPos(centerX, centerY);
		}
#endif
	}

private:
	CursorManager() = default;
	~CursorManager() = default;

	CursorManager(const CursorManager&) = delete;
	CursorManager& operator=(const CursorManager&) = delete;

	HWND m_hWnd = nullptr;
	bool m_isLocked = false;
};