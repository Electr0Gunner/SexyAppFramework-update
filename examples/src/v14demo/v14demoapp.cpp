#include "v14demoapp.hpp"
#include "PopLib/widget/widgetmanager.hpp"
#include "PopLib/imagelib/imagelib.hpp"
#include "PopLib/resources/resourcemanager.hpp"
#include "PopLib/widget/dialog.hpp"
#include "PopLib/graphics/SWTri/SWTri.hpp"
#include "board.hpp"
#include "res.hpp"
#include "PopLib/misc/httptransfer.hpp"

using namespace PopLib;

V14DemoApp::V14DemoApp()
{
	mProdName = "V14Demo";
	mProductVersion = "1.0";
	mTitle = "PopLib: " + mProdName + " - " + mProductVersion;
	mRegKey = "PopCap/PopLib/V14Demo";

	//mNoSoundNeeded = true;
	mBoard = NULL;

	// Important: You now need to make this call to enable 
	// DrawImageTransform and DrawImageMatrix to work!
	// This requirement was added so that people who don't use these functions
	// can avoid the extra code required to support the functions 
	// (about 400K in exe size, 150K zipped).
	SWTri_AddAllDrawTriFuncs();
}

V14DemoApp::~V14DemoApp()
{
	delete mBoard;
}

void V14DemoApp::ShutdownHook()
{
	// Notice that we don't have to check for mShutdown and return immediately if it's true.
	// Also note that we don't have to call our parent class' Shutdown() method. This
	// ShutdownHook() function is only called once, and only after the parent gets Shutdown
	// called. It just saves us a few lines of extra checking, and saves some headaches that can
	// arise when you forget to call a parent class method.

	if (mBoard != NULL)
		mWidgetManager->RemoveWidget(mBoard);
}

void V14DemoApp::InitHook()
{
	// We call the PopApp Init hook so we can propely set up systems like DiscordRPC.
	PopApp::InitHook();
	// Like the ShutdownHook idea, we no longer need to call the parent class' Init method.
	// This function only gets called once, and after the parent has finished its Init code.
	// It just saves us a few lines of extra checking, and saves some headaches that can
	// arise when you forget to call a parent class method.
	LoadResourceManifest();

	// We're just going to wind up using the default font from this resource group.
	// Since there's very little to load, we'll dispense with the whole
	// title screen loading stuff.
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

	// This is new. Widgets now contain flags that alter the behavior of them.
	// Please read the document included with the 1.2 release for a full description,
	// but here's the general rundown:
	// The WidgetManager's mDefaultBelowModalFlagsMod contains flags that are applied
	// anytime a dialog box pops up on screen. If we modify these, then we modify the
	// default behavior for ALL dialog boxes, and don't have to mess around with other
	// function calls and setting of misc. variables. All flags variables contain 2
	// sub-variables: mAddFlags and mRemoveFlags. By ORing stuff with mAddFlags, when
	// a dialog box pops up, the flags in mAddFlags are applied in addition to any
	// other flags that exist. Similarly, mRemoveFlags contains flags that are removed
	// from a widget when the dialog pops up. Below, what we're saying is that when
	// a dialog box pops up, we DO NOT want any widgets beneath it to be able to update.
	// What'll happen is that any widget below the dialog gets the WIDGETFLAGS_UPDATE
	// flag removed. This will of course be restored when the dialog box closes.
	// Again, please see the 1.2 doc for a list of all the flags and a much more
	// thorough explanation.
	mWidgetManager->mDefaultBelowModalFlagsMod.mRemoveFlags |= WIDGETFLAGS_UPDATE;

	mBoard = new Board(this);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);

	PopLib::HTTPTransfer transfer;

#ifdef _DEBUG
	// simple GET test,
	// we use HTTP because curl is an asshole
	// that depends on 20gb of slop
	std::string testURL = "http://httpbin.org/get";
	transfer.Get(testURL);

	SDL_Log("GET result: %d", transfer.GetResultCode());
	SDL_Log("GET content length: %zu", transfer.GetContent().size());
	SDL_Log("GET content:\n%s", transfer.GetContent().c_str());
#endif

	// H521
	SetTaskBarIcon("v14icon.png");
}

void V14DemoApp::LostFocus()
{
	if (mBoard)
		mBoard->SetFocusLost(true);
}

void V14DemoApp::GotFocus()
{
	if (mBoard)
		mBoard->SetFocusLost(false);
}

void V14DemoApp::DialogButtonDepress(int dialog_id, int button_id)
{
	// See how easy it is now? We only create a dialog with 1 ID in this
	// demo: ID of 100. 
	// See, all we have to do is check if the dialog id is of the dialog
	// we're interested in, and if the button id is of the button that
	// we want to act on.
	if ((dialog_id == 100) && (button_id == Dialog::ID_OK))
		KillDialog(100);
}