#include "imguimanager.hpp"
#include "appbase.hpp"

// interfaces
#include "graphics/renderer/sdlrenderer.hpp"

using namespace PopLib;

bool demoWind = false;

////////////////////////////

static std::vector<ImGuiWindowEntry> gWindowList;

void RegisterImGuiWindow(const char *name, bool *enabled, const ImGuiManager::WindowFunction &func)
{
	gWindowList.push_back({name, enabled, func});
}

void RegisterImGuiWindows()
{
	for (auto &entry : gWindowList)
	{
		if (entry.enabled && *entry.enabled)
		{
			gAppBase->mIGUIManager->AddWindow(entry.func);
		}
	}
}

////////////////////////////

ImGuiManager::ImGuiManager(Renderer *theInterface)
{
	mRenderer = theInterface;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io; // uhhhhhh
	ImGui::StyleColorsDark();

	switch (gAppBase->mRendererAPI)
	{
		default:
		{
			SDLRenderer *aInterface = (SDLRenderer*)mRenderer;
			ImGui_ImplSDL3_InitForSDLRenderer(gAppBase->mWindow, aInterface->mRenderer);
			ImGui_ImplSDLRenderer3_Init(aInterface->mRenderer);
		}
	}
}

void ImGuiManager::RenderAll(void)
{
	// simple yet effective
	for (const auto &entry : gWindowList)
	{
		if (entry.enabled && *entry.enabled)
			entry.func();
	}
}

void ImGuiManager::Frame(void)
{
	switch (gAppBase->mRendererAPI)
	{
		default:
		{
			ImGui_ImplSDLRenderer3_NewFrame();
			ImGui_ImplSDL3_NewFrame();
		}
	}
	ImGui::NewFrame();

	if (demoWind)
		ImGui::ShowDemoWindow();

	RenderAll();

	ImGui::Render();
}

ImGuiManager::~ImGuiManager()
{
	switch (gAppBase->mRendererAPI)
	{
		default:
		{
			ImGui_ImplSDLRenderer3_Shutdown();
			ImGui_ImplSDL3_Shutdown();
		}
	}

	ImGui::DestroyContext();
}