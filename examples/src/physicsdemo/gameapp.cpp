#include "gameapp.hpp"
#include "board.hpp"
#include "PopLib/widget/widgetmanager.hpp"

// The SexyAppFramework resides in the "Sexy" namespace. As a convenience,
// you'll see in all the .cpp files "using namespace Sexy" to avoid
// having to prefix everything with Sexy::
using namespace PopLib;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
GameApp::GameApp()
{
	// mProdName is used for internal purposes to indicate the game that we're working on
	mProdName = "PhysicsDemo";

	// For internal uses, indicates the current product version
	mProductVersion = "1.0";

	// This is the text that appears in the title bar of the application window
	mTitle = "PopLib: " + mProdName + " - " + mProductVersion;

	// Indicates the registry location where all registry keys will be read from
	// and written to. This is stored under the HKEY_CURRENT_USER tree on
	// Windows systems.
	mRegKey = "PopCap/PopLib/Physicsdemo1";

	// Set the application width/height in terms of pixels here. Let's
	// use a different resolution from Demo 1 just for fun.
	mWidth = 640;
	mHeight = 480;

	// By setting this to true, the framework will automatically check to see
	// if hardware acceleration can be turned on. This doesn't guarantee that it
	// WILL be turned on, however. Some cards just aren't compatible or have
	// known issues. Also, cards with less than 8MB of video RAM aren't supported.
	// There are ways to override the 3D enabled settings, which we will discuss
	// in a later demo. As a side note, if you want to see if you app is
	// running with 3D acceleration, first enable debug keys by pressing
	// CTRL-ALT-D and then press F9. To toggle 3D on/off, press F8. That is just
	// for testing purposes.
	//
	// When 3D mode is on, the standard drawing routines will automatically use
	// their hardware rendering versions, which in truns makes the game run faster.
	// You do not need to do anything different when drawing in 2D or 3D mode.
	// Although if 3D mode is disabled, you will most likely want to do less
	// drawing intensive operations like additive drawing, colorization,
	// real-time flipping/mirroring, etc.
	// mAutoEnable3D = true;

	mBoard = NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
GameApp::~GameApp()
{
	// Remove our "Board" class which was, in this particular demo,
	// responsible for all our game drawing and updating.
	// All widgets MUST be removed from the widget manager before deletion.
	// More information on the basics of widgets can be found in the Board
	// class file. If you tried to delete the Board widget before removing
	// it, you will get an assert.
	mWidgetManager->RemoveWidget(mBoard);

	delete mBoard;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void GameApp::Init()
{
	// Let the parent class perform any needed initializations first.
	// This should always be done.
	AppBase::Init();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void GameApp::LoadingThreadProc()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void GameApp::LoadingThreadCompleted()
{
	// Let the base app class also know that we have completed
	AppBase::LoadingThreadCompleted();

	// When we're actually loading resources, we'll set the
	// mLoadingFailed variable to "true" if there were any problems
	// encountered along the way. If that is the case, just return
	// because we won't want the user to get to the main menu or any
	// other part of the game. We will want them to exit out.
	if (mLoadingFailed)
		return;

	// Now that we're done loading everything we need (which wasn't
	// anything in this particular demo), we need to get the main
	// game screen up and running: That is our "Board" class, and
	// it will handle all the drawing, updating, and input processing
	// for most of the game.
	mBoard = new Board(this);

	// This is a very important step: Because the Board class is a widget
	// (see Board.h/.cpp for more details) we need to tell it what
	// dimensions it has and where to place it.
	// By default a widget is invisible because its
	// width/height are 0, 0. Since the Board class is our main
	// drawing area and game logic class, we want to make it the
	// same size as the application. For this particular demo, that means
	// 800x600. We will use mWidth and mHeight though, as those were
	// already set to the proper resolution in GameApp::Init().
	mBoard->Resize(0, 0, mWidth, mHeight);

	// Also an important step is to add the newly created Board widget to
	// the widget manager so that it will automatically have its update, draw,
	// and input processing methods called.
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
}
