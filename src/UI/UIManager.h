#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"
#include "AUIScreen.h"
#include "UserSettings.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

typedef std::string String;

class Viewport;
class UIManager
{
public: 
	typedef std::string String;
	typedef std::vector<std::shared_ptr<AUIScreen>> UIList;
	typedef std::unordered_map<String, std::shared_ptr<AUIScreen>> UITable;

	static UIManager* getInstance();
	static void initialize(UserSettings* userSettings);
	static void destroy();

	void drawAllUI() const;
	bool getEnabled(const std::string& name);
	void setEnabled(const String& uiName, bool flag);
	void toggleEnabled(const String& uiName);
	std::shared_ptr<AUIScreen> findUIByName(const String& uiName);

	UserSettings* settings() const { return userSettings; }
	void toggleAllUI() const;

	// fucky test code below vvv
	//std::vector<VkImage>* images = nullptr;
	const Vulkan::Device* device = nullptr;
	Vulkan::Sampler* sampler = nullptr;
	Vulkan::ImageView* imageView = nullptr;
	VkDescriptorSet m_Dset;
	//Vulkan::Image* image = nullptr;
	// fucky test code above ^^^
private:
	UIManager();
	~UIManager();
	UIManager(UIManager const&) {};             // copy constructor is private
	UIManager& operator=(UIManager const&) {};  // assignment operator is private*/
	static UIManager* sharedInstance;

	UserSettings* userSettings = nullptr;

	UIList uiList;
	UITable uiTable;
};

