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
		m_isFocused = m_hWnd && ::GetForegroundWindow() == m_hWnd;
	}

	// Lock or unlock the cursor
	void SetCursorLock(bool locked) {
		if (m_isLocked == locked) {
			if (locked && m_isTemporarilyUnlocked) {
				Relock();
			}
			return;
		}
		m_isLocked = locked;
		m_isTemporarilyUnlocked = false;
		if (!m_isFocused) return;

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

	void OnFocusChanged(bool focused) {
		if (m_isFocused == focused) return;
		m_isFocused = focused;

#if defined(_WIN32) || defined(PLATFORM_WIN32)
		if (!focused) {
			::ClipCursor(NULL);
			while (::ShowCursor(TRUE) < 0);
			m_cursorWarped = false;
			return;
		}
#endif

		if (m_isLocked) {
			Relock();
		}
	}

	void TemporarilyUnlock() {
		if (!m_isLocked || m_isTemporarilyUnlocked) return;
		m_isTemporarilyUnlocked = true;
		m_cursorWarped = false;

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

#if defined(_WIN32) || defined(PLATFORM_WIN32)
		if (m_hWnd) {
			::ClipCursor(NULL);
			while (::ShowCursor(TRUE) < 0);
		}
#endif
	}

	void Relock() {
		if (!m_isLocked || !m_isFocused) return;
		m_isTemporarilyUnlocked = false;
		m_isLocked = false;
		SetCursorLock(true);
	}

	void OnMouseButtonDown() {
		if (m_isTemporarilyUnlocked) {
			Relock();
		}
	}

	void ToggleLock() {
		SetCursorLock(!m_isLocked);
	}

	bool IsLocked() const {
		return m_isLocked;
	}

	bool IsTemporarilyUnlocked() const {
		return m_isTemporarilyUnlocked;
	}

	ImVec2 ProcessMouseDelta(const ImVec2& mouseDelta) {
		if (!m_cursorWarped) return mouseDelta;
		m_cursorWarped = false;
		return ImVec2(0.0f, 0.0f);
	}

	void MaintainLock() {
#if defined(_WIN32) || defined(PLATFORM_WIN32)
		if (m_isLocked && !m_isTemporarilyUnlocked && m_isFocused && m_hWnd && ::GetForegroundWindow() == m_hWnd) {
			UpdateClipRect(false);

			POINT cursorPosition;
			if (::GetCursorPos(&cursorPosition)) {
				RECT clientRect;
				::GetClientRect(m_hWnd, &clientRect);

				POINT topLeft{ clientRect.left, clientRect.top };
				POINT bottomRight{ clientRect.right, clientRect.bottom };
				::ClientToScreen(m_hWnd, &topLeft);
				::ClientToScreen(m_hWnd, &bottomRight);

				constexpr LONG recenterMargin = 32;
				if (cursorPosition.x <= topLeft.x + recenterMargin ||
					cursorPosition.x >= bottomRight.x - recenterMargin ||
					cursorPosition.y <= topLeft.y + recenterMargin ||
					cursorPosition.y >= bottomRight.y - recenterMargin) {
					UpdateClipRect(true);
				}
			}
		}
#endif
	}

	// Recalculates screen bounds (call if window is resized/moved while locked)
	void UpdateClipRect(bool recenter = true) {
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

			if (recenter) {
				int centerX = topLeft.x + (rect.right - rect.left) / 2;
				int centerY = topLeft.y + (rect.bottom - rect.top) / 2;
				::SetCursorPos(centerX, centerY);
				m_cursorWarped = true;
			}
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
	bool m_isFocused = true;
	bool m_isTemporarilyUnlocked = false;
	bool m_cursorWarped = false;
};