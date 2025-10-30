#pragma once
#include "GLFW/glfw3.h"
#include "ActionMap.hpp"
#include "HotkeyListener.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

template<class T>
inline void hash_combine(std::size_t& s, const T& v)
{
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

struct EventKey
{
	int key;
	int mod;

	bool operator==(const EventKey& other) const {
		return key == other.key && mod == other.mod;
	}
};

namespace std
{
	template<>
	struct hash<EventKey>
	{
		std::size_t operator()(const EventKey& ek) const
		{
			std::size_t res = 0;

			hash_combine(res, ek.key);
			hash_combine(res, ek.key);

			return res;
		}
	};
};

class HotkeySystem
{
public:
	using Action = Hotkey::Action;

	static class HotkeySystem* getInstance();
	static void initialize();
	static void destroy();

	void addListener(HotkeyListener* listener);
	void removeListener(HotkeyListener* listener);

	void bindHotkey(EventKey event, Action action);
	void bindMouseHotkey(EventKey event, Action action);

	void processInputKeys(int key, int mod, int action);
	void processInputMouseButtons(int key, int mod, int action);

private:
	HotkeySystem();
	~HotkeySystem() = default;
	HotkeySystem(HotkeySystem const&) {};             // copy constructor is private
	HotkeySystem& operator=(HotkeySystem const&) {};  // assignment operator is private*/

	static HotkeySystem* sharedInstance;

	void setupDefaultBindings();

	std::unordered_set<HotkeyListener*> m_hotkeyListeners;
	std::unordered_map<EventKey, std::vector<Action>> m_keyBindings;
	std::unordered_map<EventKey, std::vector<Action>> m_mouseButtonBindings;
};