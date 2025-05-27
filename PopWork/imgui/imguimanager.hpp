#ifndef __IMGUIMANAGER_HPP__
#define __IMGUIMANAGER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "core/imgui.h"
#include "core/imgui_impl_sdl3.h"
#include "core/imgui_impl_sdlrenderer3.h"

#include <vector>
#include <functional>

namespace PopWork
{
class SDLInterface;

/**
 * @brief imgui manager for sdlinterface
 */
class ImGuiManager
{
  public:
	/// @brief the function
	using WindowFunction = std::function<void()>;

	/// @brief adds a window to the mWindows list
	/// @param func
	inline void AddWindow(const WindowFunction &func)
	{
		mWindows.push_back(func);
	}

	/// @brief constructor
	ImGuiManager(SDLInterface *theInterface);
	/// @brief destructor
	virtual ~ImGuiManager();

	/// @brief renders all imgui windows
	/// @param none
	virtual void RenderAll(void);

	/// @brief runs each frame
	/// @param none
	virtual void Frame(void);

	/// @brief the sdlinterface
	SDLInterface *mInterface;

  private:
	/// @brief the imgui windows list
	std::vector<WindowFunction> mWindows;
};

} // namespace PopWork

/// @brief the imgui window entry
struct ImGuiWindowEntry
{
	/// @brief the name of imgui window
	const char *name;
	/// @brief pointer to external bool
	bool *enabled;
	/// @brief the function
	PopWork::ImGuiManager::WindowFunction func;
};

/// @brief registers an imgui window
/// @param name
/// @param enabled
/// @param func
void RegisterImGuiWindow(const char *name, bool *enabled, const PopWork::ImGuiManager::WindowFunction &func);
/// @brief registers all imgui windows
void RegisterImGuiWindows();

#endif