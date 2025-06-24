#include "gameapp.hpp"
#include "board.hpp"
#include "PopLib/widget/widgetmanager.hpp"
#include "PopLib/common.hpp"
#include "PopLib/audio/bassmusicinterface.hpp"
#include "PopLib/graphics/sdlimage.hpp"
#include "PopLib/resources/resourcemanager.hpp"
#include "res.hpp"

using namespace PopLib;

GameApp::GameApp()
{
	mProdName = "Tetris";

	mProductVersion = "1.1";

	mTitle = "PopLib: " + mProdName + " - " + mProductVersion;

	mRegKey = "PopCap/PopLib/Tetris";

	mWidth = 800;
	mHeight = 600;

	mBoard = nullptr;
}

GameApp::~GameApp()
{
	mWidgetManager->RemoveWidget(mBoard);
	delete mBoard;
}

void GameApp::Init()
{
	AppBase::Init();

	LoadResourceManifest();

	if (!mResourceManager->LoadResources("Init") || !mResourceManager->LoadResources("Game"))
	{
		mLoadingFailed = true;
		ShowResourceError(true);
		return;
	}

	if (!ExtractInitResources(mResourceManager) || !ExtractGameResources(mResourceManager))
	{
		mLoadingFailed = true;
		ShowResourceError(true);
		return;
	}

	// load the music
	mMusicInterface->LoadMusic(0, "music/tetris.ogg");
	mMusicInterface->FadeIn(0, 0, 0.002, false);
}

void GameApp::LoadingThreadProc()
{
}

void GameApp::LoadingThreadCompleted()
{
	AppBase::LoadingThreadCompleted();

	if (mLoadingFailed)
		return;

	mBoard = new Board(this);

	mBoard->Resize(0, 0, mWidth, mHeight);

	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
}