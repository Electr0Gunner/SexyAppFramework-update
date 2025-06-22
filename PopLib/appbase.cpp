#include "appbase.hpp"
#include "widget/widgetmanager.hpp"
#include "widget/widget.hpp"
#include "misc/keycodes.hpp"
#include "graphics/sdlinterface.hpp"
#include "graphics/sdlimage.hpp"
#include "graphics/memoryimage.hpp"
#include "widget/dialog.hpp"
#include "imagelib/imagelib.hpp"
#include "audio/openalsoundmanager.hpp"
#include "audio/openalsoundinstance.hpp"
#include "math/rect.hpp"
#include "resources/propertiesparser.hpp"
#include "debug/perftimer.hpp"
#include "math/mtrand.hpp"
#include "readwrite/modval.hpp"
#include "debug/sehcatcher.hpp"
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>

#define _strtime strftime
#endif
#include <fstream>
#include "graphics/sysfont.hpp"
#include "resources/resourcemanager.hpp"
#include "audio/bassmusicinterface.hpp"
#include <bass.h>
#include "misc/autocrit.hpp"
#include "debug/debug.hpp"
#include "debug/errorhandler.hpp"
#include "paklib/pakinterface.hpp"
#include "imgui/imguimanager.hpp"

#include <string>
#include <ctime>
#include <cmath>

#include <filesystem>
#include <set>
#include <regex>

#include "debug/memmgr.hpp"

// H521
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include "stb_image_write.h"

// H522
#include <json.hpp>

using namespace PopLib;

namespace fs = std::filesystem;

AppBase *PopLib::gAppBase = nullptr;

SEHCatcher PopLib::gSEHCatcher;

static bool gScreenSaverActive = false;

#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif

static SDLImage *gFPSImage = nullptr;

AppBase::AppBase()
{
	if (!SDL_Init(SDL_INIT_VIDEO) || !TTF_Init())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Initialization Failed", SDL_GetError(), nullptr);
		return;
	}

	gAppBase = this;

	mMutex = nullptr;

#ifdef _DEBUG
	mOnlyAllowOneCopyToRun = false;
#else
	mOnlyAllowOneCopyToRun = true;
#endif

	// Extract product version
	// char aPath[_MAX_PATH];
	// GetModuleFileNameA(nullptr, aPath, 256);
	// mProductVersion = GetProductVersion(aPath);
	// mChangeDirTo = GetFileDir(aPath);

	mNoDefer = false;
	mFullScreenPageFlip = true; // should we page flip in fullscreen?
	mTimeLoaded = SDL_GetTicks();
	mSEHOccured = false;
	mProdName = "Product";
	mTitle = "PopLibApp";
	mShutdown = false;
	mExitToTop = false;
	mWidth = 640;
	mHeight = 480;
	mFullscreenBits = 16;
	mIsWindowed = true;
	mIsPhysWindowed = true;
	mFullScreenWindow = false;
	mPreferredX = -1;
	mPreferredY = -1;
	mIsScreenSaver = false;
	mAllowMonitorPowersave = true;
	mSDLInterface = nullptr;
	mMusicInterface = nullptr;
	mErrorHandler = nullptr;
	mIGUIManager = nullptr;
	mFrameTime = 10;
	mNonDrawCount = 0;
	mDrawCount = 0;
	mSleepCount = 0;
	mUpdateCount = 0;
	mUpdateAppState = 0;
	mUpdateAppDepth = 0;
	mPendingUpdatesAcc = 0.0;
	mUpdateFTimeAcc = 0.0;
	mHasPendingDraw = true;
	mIsDrawing = false;
	mLastDrawWasEmpty = false;
	mLastTimeCheck = 0;
	mUpdateMultiplier = 1;
	mPaused = false;
	mFastForwardToUpdateNum = 0;
	mFastForwardToMarker = false;
	mFastForwardStep = false;
	mSoundManager = nullptr;
	mCursorNum = CURSOR_POINTER;
	mMouseIn = false;
	mRunning = false;
	mActive = true;
	mProcessInTimer = false;
	mMinimized = false;
	mPhysMinimized = false;
	mIsDisabled = false;
	mLoaded = false;
	mYieldMainThread = false;
	mLoadingFailed = false;
	mLoadingThreadStarted = false;
	mAutoStartLoadingThread = true;
	mLoadingThreadCompleted = false;
	mCursorThreadRunning = false;
	mNumLoadingThreadTasks = 0;
	mCompletedLoadingThreadTasks = 0;
	mLastDrawTick = SDL_GetTicks();
	mNextDrawTick = SDL_GetTicks();
	mSysCursor = true;
	mForceFullscreen = false;
	mForceWindowed = false;
	mHasFocus = true;
	mCustomCursorsEnabled = false;
	mCustomCursorDirty = false;
	mOverrideCursor = nullptr;
	mTitleBarIcon = nullptr;
	mIsOpeningURL = false;
	mInitialized = false;
	mLastShutdownWasGraceful = true;
	mReadFromRegistry = false;
	mCmdLineParsed = false;
	mSkipSignatureChecks = false;
	mCtrlDown = false;
	mAltDown = false;
	mStepMode = 0;
	mCleanupSharedImages = false;
	mStandardWordWrap = true;
	mbAllowExtendedChars = true;
	mEnableMaximizeButton = false;

	mMusicVolume = 0.85;
	mSfxVolume = 0.85;
	mMuteCount = 0;
	mAutoMuteCount = 0;
	mMuteOnLostFocus = true;
	mFPSTime = 0;
	mFPSStartTick = SDL_GetTicks();
	mFPSFlipCount = 0;
	mFPSCount = 0;
	mFPSDirtyCount = 0;
	mShowFPS = false;
	mShowFPSMode = FPS_ShowFPS;
	mDrawTime = 0;
	mScreenBltTime = 0;
	mAlphaDisabled = false;
	mDebugKeysEnabled = false;
	mNoSoundNeeded = false;

	mSyncRefreshRate = 100;
	mVSyncUpdates = false;
	mVSyncBroken = false;
	mVSyncBrokenCount = 0;
	mVSyncBrokenTestStartTick = 0;
	mVSyncBrokenTestUpdates = 0;
	mWaitForVSync = false;
	mSoftVSyncWait = true;
	mUserChanged3DSetting = false;
	mAutoEnable3D = false;
	mTest3D = false;
	mMinVidMemory3D = 6;
	mRecommendedVidMemory3D = 14;
	mRelaxUpdateBacklogCount = 0;
	mWidescreenAware = false;
	mEnableWindowAspect = false;
	mWindowAspect.Set(4, 3);
	mIsWideWindow = false;

	int i;

	for (i = 0; i < NUM_CURSORS; i++)
		mCursorImages[i] = nullptr;

	for (i = 0; i < 256; i++)
		mAdd8BitMaxTable[i] = i;

	for (i = 256; i < 512; i++)
		mAdd8BitMaxTable[i] = 255;

	// Set default strings.  Init could read in overrides from partner.xml
	SetString("DIALOG_BUTTON_OK", "OK");
	SetString("DIALOG_BUTTON_CANCEL", "CANCEL");

	SetString("UPDATE_CHECK_TITLE", "Update Check");
	SetString("UPDATE_CHECK_BODY", "Checking if there are any updates available for this product ...");

	SetString("UP_TO_DATE_TITLE", "Up to Date");
	SetString("UP_TO_DATE_BODY", "There are no updates available for this product at this time.");
	SetString("NEW_VERSION_TITLE", "New Version");
	SetString("NEW_VERSION_BODY",
			  "There is an update available for this product.  Would you like to visit the web site to download it?");

	mWidgetManager = new WidgetManager(this);
	mResourceManager = new ResourceManager(this);

	mPrimaryThreadId = (SDL_ThreadID)0;

	/*
	if (GetSystemMetrics(86)) // check for tablet pc
	{
		mTabletPC = true;
		mFullScreenPageFlip = false; // so that tablet keyboard can show up
	}
	else*/
	mTabletPC = false;

	gSEHCatcher.mApp = this;

	// std::wifstream stringsFile(_wfopen(L".\\properties\\fstrings", L"rb"));
	//
	// if(!stringsFile)
	//{
	//	MessageBox(nullptr, "file missing: 'install-folder\\properties\\fstrings' Please re-install", "FATAL ERROR",
	//MB_OK); 	DoExit(1);
	// }
	// std::getline(stringsFile, mString_HardwareAccelSwitchedOn);
	// std::getline(stringsFile, mString_HardwareAccelConfirm);
	// std::getline(stringsFile, mString_HardwareAccelNotWorking);
	// std::getline(stringsFile, mString_SetColorDepth);
	// std::getline(stringsFile, mString_FailedInitDirectDrawColon);
	// std::getline(stringsFile, mString_UnableOpenProperties);
	// std::getline(stringsFile, mString_SigCheckFailed);
	// std::getline(stringsFile, mString_InvalidCommandLineParam);
	// std::getline(stringsFile, mString_RequiresDirectX);
	// std::getline(stringsFile, mString_YouNeedDirectX);
	// std::getline(stringsFile, mString_FailedInitDirectDraw);
	// std::getline(stringsFile, mString_FatalError);
	// std::getline(stringsFile, mString_UnexpectedErrorOccured);
	// std::getline(stringsFile, mString_PleaseHelpBy);
	// std::getline(stringsFile, mString_FailedConnectPopcap);
	// stringsFile.close();
}

namespace PopLib {
    extern bool gSDLInterfacePreDrawError;
}

AppBase::~AppBase()
{
	Shutdown();

	// Check if we should write the current 3d setting
	bool showedMsgBox = false;
	if (mUserChanged3DSetting)
	{
		bool writeToRegistry = true;
		bool is3D = false;
		bool is3DOptionSet = RegistryReadBoolean("Is3D", &is3D);
		if (!is3DOptionSet) // should we write the option?
		{
			if (!Is3DAccelerationRecommended()) // may need to prompt user if he wants to keep 3d acceleration on
			{
				if (Is3DAccelerated())
				{
					showedMsgBox = true;
					SDL_MessageBoxButtonData buttons[] = {{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
														  {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No"}};

					std::string aTitle = mCompanyName + " " + GetString("HARDWARE_ACCEL_CONFIRMATION", "Hardware Acceleration Confirmation");

					std::string aMessage = GetString("HARDWARE_ACCEL_SWITCHED_ON",
								  "Hardware Acceleration was switched on during this session.\n"
									  "If this resulted in slower performance, it should be switched off.\n"
										  "Would you like to keep Hardware Acceleration switched on?");

					SDL_MessageBoxData aMessageboxData = {};
					aMessageboxData.flags = SDL_MESSAGEBOX_INFORMATION;
					aMessageboxData.title = aTitle.c_str();
					aMessageboxData.message = aMessage.c_str();
					aMessageboxData.numbuttons = SDL_arraysize(buttons);
					aMessageboxData.buttons = buttons;
					aMessageboxData.window = mSDLInterface->mWindow;

					int aResult;
					if (!SDL_ShowMessageBox(&aMessageboxData, &aResult))
					{
						SDL_Log("Error displaying message box: %s", SDL_GetError());
					}
					mSDLInterface->mIs3D = aResult == 1 ? true : false;

					if (aResult != 1)
						writeToRegistry = false;
				}
				else
					writeToRegistry = false;
			}
		}

		if (writeToRegistry)
			RegistryWriteBoolean("Is3D", mSDLInterface->mIs3D);
	}

	if (!showedMsgBox && PopLib::gSDLInterfacePreDrawError && !IsScreenSaver())
	{
		const SDL_MessageBoxButtonData buttons[] = {
			{0, 0, "No"},
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
		};

		std::string aMessage = GetString("HARDWARE_ACCEL_NOT_WORKING",
					  "Hardware Acceleration may not have been working correctly during this session.\r\n"
						  "If you noticed graphics problems, you may want to turn off Hardware Acceleration.\r\n"
							  "Would you like to keep Hardware Acceleration switched on?");

		std::string aTitle = mCompanyName + " " +
			GetString("HARDWARE_ACCEL_CONFIRMATION", "Hardware Acceleration Confirmation");

		SDL_MessageBoxData messageBoxData = {
			SDL_MESSAGEBOX_INFORMATION, nullptr, aTitle.c_str(), aMessage.c_str(), SDL_arraysize(buttons), buttons, nullptr};

		int aResult = mSDLInterface->MakeResultMessageBox(messageBoxData);
		if (aResult == 0)
			RegistryWriteBoolean("Is3D", false);
	}

	DialogMap::iterator aDialogItr = mDialogMap.begin();
	while (aDialogItr != mDialogMap.end())
	{
		mWidgetManager->RemoveWidget(aDialogItr->second);
		delete aDialogItr->second;
		++aDialogItr;
	}
	mDialogMap.clear();
	mDialogList.clear();

	delete mWidgetManager;
	delete mResourceManager;
	delete gFPSImage;
	gFPSImage = nullptr;

	SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
	while (aSharedImageItr != mSharedImageMap.end())
	{
		SharedImage *aSharedImage = &aSharedImageItr->second;
		DBG_ASSERTE(aSharedImage->mRefCount == 0);
		delete aSharedImage->mImage;
		mSharedImageMap.erase(aSharedImageItr++);
	}

	delete mSDLInterface;
	delete mMusicInterface;
	delete mSoundManager;
	delete mErrorHandler;
	delete mIGUIManager;

	BASS_Stop();

	WaitForLoadingThread();

	SDL_DestroyCursor(mHandCursor);
	SDL_DestroyCursor(mDraggingCursor);

	gAppBase = nullptr;
}

void AppBase::ClearUpdateBacklog(bool relaxForASecond)
{
	mLastTimeCheck = SDL_GetTicks();
	mUpdateFTimeAcc = 0.0;

	if (relaxForASecond)
		mRelaxUpdateBacklogCount = 1000;
}

bool AppBase::IsScreenSaver()
{
	return mIsScreenSaver;
}

bool AppBase::AppCanRestore()
{
	return !mIsDisabled;
}

Dialog *AppBase::NewDialog(int theDialogId, bool isModal, const PopString &theDialogHeader,
						   const PopString &theDialogLines, const PopString &theDialogFooter, int theButtonMode)
{
	Dialog *aDialog =
		new Dialog(nullptr, nullptr, theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	return aDialog;
}

Dialog *AppBase::DoDialog(int theDialogId, bool isModal, const PopString &theDialogHeader,
						  const PopString &theDialogLines, const PopString &theDialogFooter, int theButtonMode)
{
	KillDialog(theDialogId);

	Dialog *aDialog = NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);

	AddDialog(theDialogId, aDialog);

	return aDialog;
}

Dialog *AppBase::GetDialog(int theDialogId)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
		return anItr->second;

	return nullptr;
}

bool AppBase::KillDialog(int theDialogId, bool removeWidget, bool deleteWidget)
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
	{
		Dialog *aDialog = anItr->second;

		// set the result to something else so DoMainLoop knows that the dialog is gone
		// in case nobody else sets mResult
		if (aDialog->mResult == -1)
			aDialog->mResult = 0;

		DialogList::iterator aListItr = std::find(mDialogList.begin(), mDialogList.end(), aDialog);
		if (aListItr != mDialogList.end())
			mDialogList.erase(aListItr);

		mDialogMap.erase(anItr);

		if (removeWidget || deleteWidget)
			mWidgetManager->RemoveWidget(aDialog);

		if (aDialog->IsModal())
		{
			ModalClose();
			mWidgetManager->RemoveBaseModal(aDialog);
		}

		if (deleteWidget)
			SafeDeleteWidget(aDialog);

		return true;
	}

	return false;
}

bool AppBase::KillDialog(int theDialogId)
{
	return KillDialog(theDialogId, true, true);
}

bool AppBase::KillDialog(Dialog *theDialog)
{
	return KillDialog(theDialog->mId);
}

int AppBase::GetDialogCount()
{
	return mDialogMap.size();
}

void AppBase::AddDialog(int theDialogId, Dialog *theDialog)
{
	KillDialog(theDialogId);

	if (theDialog->mWidth == 0)
	{
		// Set the dialog position ourselves
		int aWidth = mWidth / 2;
		theDialog->Resize((mWidth - aWidth) / 2, mHeight / 5, aWidth, theDialog->GetPreferredHeight(aWidth));
	}

	mDialogMap.insert(DialogMap::value_type(theDialogId, theDialog));
	mDialogList.push_back(theDialog);

	mWidgetManager->AddWidget(theDialog);
	if (theDialog->IsModal())
	{
		mWidgetManager->AddBaseModal(theDialog);
		ModalOpen();
	}
}

void AppBase::AddDialog(Dialog *theDialog)
{
	AddDialog(theDialog->mId, theDialog);
}

void AppBase::ModalOpen()
{
}

void AppBase::ModalClose()
{
}

void AppBase::DialogButtonPress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonPress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonPress(3000 + theDialogId);
}

void AppBase::DialogButtonDepress(int theDialogId, int theButtonId)
{
	if (theButtonId == Dialog::ID_YES)
		ButtonDepress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonDepress(3000 + theDialogId);
}

void AppBase::GotFocus()
{
}

void AppBase::LostFocus()
{
}

void AppBase::URLOpenFailed(const std::string &theURL)
{
	mIsOpeningURL = false;
}

void AppBase::URLOpenSucceeded(const std::string &theURL)
{
	mIsOpeningURL = false;

	if (mShutdownOnURLOpen)
		Shutdown();
}

bool AppBase::OpenURL(const std::string &theURL, bool shutdownOnOpen)
{
	if ((!mIsOpeningURL) || (theURL != mOpeningURL))
	{
		mShutdownOnURLOpen = shutdownOnOpen;
		mIsOpeningURL = true;
		mOpeningURL = theURL;
		mOpeningURLTime = SDL_GetTicks();

		if (SDL_OpenURL(theURL.c_str()))
		{
			return true;
		}
		else
		{
			URLOpenFailed(theURL);
			return false;
		}
	}

	return true;
}

std::string AppBase::GetProductVersion(const std::string &thePath)
{
	/*
	// Dynamically Load Version.dll
	typedef uint32_t (APIENTRY *GetFileVersionInfoSizeFunc)(LPSTR lptstrFilename, LPuint32_t lpdwHandle);
	typedef BOOL (APIENTRY *GetFileVersionInfoFunc)(LPSTR lptstrFilename, uint32_t dwHandle, uint32_t dwLen, LPVOID lpData);
	typedef BOOL (APIENTRY *VerQueryValueFunc)(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen);

	static GetFileVersionInfoSizeFunc aGetFileVersionInfoSizeFunc = nullptr;
	static GetFileVersionInfoFunc aGetFileVersionInfoFunc = nullptr;
	static VerQueryValueFunc aVerQueryValueFunc = nullptr;

	if (aGetFileVersionInfoSizeFunc==nullptr)
	{
		aGetFileVersionInfoSizeFunc = (GetFileVersionInfoSizeFunc)GetProcAddress(gVersionDLL,"GetFileVersionInfoSizeA");
		aGetFileVersionInfoFunc = (GetFileVersionInfoFunc)GetProcAddress(gVersionDLL,"GetFileVersionInfoA");
		aVerQueryValueFunc = (VerQueryValueFunc)GetProcAddress(gVersionDLL,"VerQueryValueA");
	}

	// Get Product Version
	std::string aProductVersion;

	uint aSize = aGetFileVersionInfoSizeFunc((char*) thePath.c_str(), 0);
	if (aSize > 0)
	{
		uchar* aVersionBuffer = new uchar[aSize];
		aGetFileVersionInfoFunc((char*) thePath.c_str(), 0, aSize, aVersionBuffer);
		char* aBuffer;
		if (aVerQueryValueFunc(aVersionBuffer,
				  "\\StringFileInfo\\040904B0\\ProductVersion",
				  (void**) &aBuffer,
				  &aSize))
		{
			aProductVersion = aBuffer;
		}
		else if (aVerQueryValueFunc(aVersionBuffer,
				  "\\StringFileInfo\\040904E4\\ProductVersion",
				  (void**) &aBuffer,
				  &aSize))
		{
			aProductVersion = aBuffer;
		}

		delete aVersionBuffer;
	}
	*/
	return "0";
}

void AppBase::WaitForLoadingThread()
{
	while ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		SDL_Delay(20);
}

void AppBase::SetCursorImage(int theCursorNum, Image *theImage)
{
	if ((theCursorNum >= 0) && (theCursorNum < NUM_CURSORS))
	{
		mCursorImages[theCursorNum] = theImage;
		EnforceCursor();
	}
}

int WriteScreenShotThread(void *theArg)
{
	//////////////////////////////////////////////////////////////////////////
	// Validate the passed parameter
	SDL_Surface *theSurface = (SDL_Surface *)theArg;
	if (!theSurface)
		return 1;

	//////////////////////////////////////////////////////////////////////////
	// Get free image name
	std::string anImageDir = GetAppDataFolder() + "_screenshots";
	MkDir(anImageDir);
	anImageDir += "/";

	int aMaxId = 0;
	std::string anImagePrefix = "image";
	// add the automatic id later.
	std::string anImageName = anImageDir + anImagePrefix + StrFormat("%d.png", aMaxId + 1);

	//////////////////////////////////////////////////////////////////////////
	// Write image
	ImageLib::Image aSaveImage;
	aSaveImage.mBits = (ulong *)theSurface->pixels;
	aSaveImage.mWidth = theSurface->w;
	aSaveImage.mHeight = theSurface->h;
	ImageLib::WriteImage(anImageName, ".png", &aSaveImage);
	aSaveImage.mBits = nullptr;

	//////////////////////////////////////////////////////////////////////////
	// delete the image in this thread
	SDL_DestroySurface(theSurface);

	return 0;
}

void AppBase::TakeScreenshot()
{
	std::filesystem::path screenshotDir = std::filesystem::current_path() / "screenshots";
	std::filesystem::create_directory(screenshotDir); // create silently

	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	std::ostringstream filenameStream;
	filenameStream << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".png";
	std::filesystem::path filePath = screenshotDir / filenameStream.str();

	SDL_Surface *surface = SDL_RenderReadPixels(mSDLInterface->mRenderer, nullptr);
	if (!surface)
		return;

	uint8_t *bgra = static_cast<uint8_t *>(surface->pixels);
	int size = surface->w * surface->h * 4;
	std::vector<uint8_t> rgba(size);

	for (int i = 0; i < size; i += 4)
	{
		rgba[i + 0] = bgra[i + 2]; // R
		rgba[i + 1] = bgra[i + 1]; // G
		rgba[i + 2] = bgra[i + 0]; // B
		rgba[i + 3] = bgra[i + 3]; // A
	}

	stbi_write_png(filePath.string().c_str(), surface->w, surface->h, 4, rgba.data(), surface->w * 4);
	SDL_DestroySurface(surface);
}

void AppBase::DumpProgramInfo()
{
	Deltree(GetAppDataFolder() + "_dump");

	for (;;)
	{
		if (std::filesystem::create_directory(GetAppDataFolder() + "_dump"))
			break;
		SDL_Delay(100);
	}

	std::fstream aDumpStream("_dump/imagelist.html", std::ios::out);

	time_t aTime;
	time(&aTime);
	tm *aTM = localtime(&aTime);

	aDumpStream << "<HTML><BODY BGCOLOR=EEEEFF><CENTER><FONT SIZE=+2><B>" << asctime(aTM) << "</B></FONT><BR>"
				<< std::endl;

	int anImgNum = 0;

	int aThumbWidth = 64;
	int aThumbHeight = 64;

	ImageLib::Image anImageLibImage;
	anImageLibImage.mWidth = aThumbWidth;
	anImageLibImage.mHeight = aThumbHeight;
	anImageLibImage.mBits = new ulong[aThumbWidth * aThumbHeight];

	typedef std::multimap<int, MemoryImage *, std::greater<int>> SortedImageMap;

	int aTotalMemory = 0;

	SortedImageMap aSortedImageMap;
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage *aMemoryImage = *anItr;

		int aNumPixels = aMemoryImage->mWidth * aMemoryImage->mHeight;

		SDLImage *aSDLImage = dynamic_cast<SDLImage *>(aMemoryImage);

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;

		int aMemorySize = 0;
		if (aMemoryImage->mBits != nullptr)
			aBitsMemory = aNumPixels * 4;
		if ((aSDLImage != nullptr) && (aSDLImage->mD3DData != nullptr))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != nullptr)
			aPalletizedMemory = aNumPixels + 256 * 4;
		if (aMemoryImage->mNativeAlphaData != nullptr)
		{
			if (aMemoryImage->mColorTable != nullptr)
				aNativeAlphaMemory = 256 * 4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != nullptr)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != nullptr)
			aRLAdditiveMemory = aNumPixels;
		if (aMemoryImage->mD3DData != nullptr)
			aTextureMemory += ((SDLTextureData *)aMemoryImage->mD3DData)->GetMemSize();

		aMemorySize = aBitsMemory + aSurfaceMemory + aPalletizedMemory + aNativeAlphaMemory + aRLAlphaMemory +
					  aRLAdditiveMemory + aTextureMemory;
		aTotalMemory += aMemorySize;

		aSortedImageMap.insert(SortedImageMap::value_type(aMemorySize, aMemoryImage));

		++anItr;
	}

	aDumpStream << "Total Image Allocation: " << CommaSeperate(aTotalMemory).c_str()
				<< " bytes<BR>";
	aDumpStream << "<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=4>";

	int aTotalMemorySize = 0;
	int aTotalBitsMemory = 0;
	int aTotalSurfaceMemory = 0;
	int aTotalPalletizedMemory = 0;
	int aTotalNativeAlphaMemory = 0;
	int aTotalRLAlphaMemory = 0;
	int aTotalRLAdditiveMemory = 0;
	int aTotalTextureMemory = 0;

	SortedImageMap::iterator aSortedItr = aSortedImageMap.begin();
	while (aSortedItr != aSortedImageMap.end())
	{
		MemoryImage *aMemoryImage = aSortedItr->second;

		char anImageName[256];
		sprintf(anImageName, "img%04d", anImgNum);

		char aThumbName[256];
		sprintf(aThumbName, "thumb%04d", anImgNum);

		aDumpStream << "<TR>" << std::endl;

		aDumpStream << "<TD><A HREF=" << anImageName << ".png><IMG SRC=" << aThumbName << ".jpeg WIDTH=" << aThumbWidth
					<< " HEIGHT=" << aThumbHeight << "></A></TD>" << std::endl;

		int aNumPixels = aMemoryImage->mWidth * aMemoryImage->mHeight;

		SDLImage *aSDLImage = dynamic_cast<SDLImage *>(aMemoryImage);

		int aMemorySize = aSortedItr->first;

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;
		std::string aTextureFormatName;

		if (aMemoryImage->mBits != nullptr)
			aBitsMemory = aNumPixels * 4;
		if ((aSDLImage != nullptr) && (aSDLImage->mD3DData != nullptr))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != nullptr)
			aPalletizedMemory = aNumPixels + 256 * 4;
		if (aMemoryImage->mNativeAlphaData != nullptr)
		{
			if (aMemoryImage->mColorTable != nullptr)
				aNativeAlphaMemory = 256 * 4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != nullptr)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != nullptr)
			aRLAdditiveMemory = aNumPixels;
		if (aMemoryImage->mD3DData != nullptr)
		{
			aTextureMemory += ((SDLTextureData *)aMemoryImage->mD3DData)->GetMemSize();

			aTextureFormatName = "ARGB8888"; // They are always like this
		}

		aTotalMemorySize += aMemorySize;
		aTotalBitsMemory += aBitsMemory;
		aTotalTextureMemory += aTextureMemory;
		aTotalSurfaceMemory += aSurfaceMemory;
		aTotalPalletizedMemory += aPalletizedMemory;
		aTotalNativeAlphaMemory += aNativeAlphaMemory;
		aTotalRLAlphaMemory += aRLAlphaMemory;
		aTotalRLAdditiveMemory += aRLAdditiveMemory;

		char aStr[256];
		sprintf(aStr, "%d x %d<BR>%s bytes", aMemoryImage->mWidth, aMemoryImage->mHeight,
				CommaSeperate(aMemorySize).c_str());
		aDumpStream << "<TD ALIGN=RIGHT>" << aStr << "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< ((aBitsMemory != 0) ? "mBits<BR>" + CommaSeperate(aBitsMemory) : "&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>"
					<< ((aPalletizedMemory != 0)
							? "Palletized<BR>" + CommaSeperate(aPalletizedMemory)
							: "&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>"
					<< ((aSurfaceMemory != 0)
							? "DDSurface<BR>" + CommaSeperate(aSurfaceMemory)
							: "&nbsp;")
					<< "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< ((aMemoryImage->mD3DData != nullptr)
							? "Texture<BR>" + aTextureFormatName + "<BR>" + CommaSeperate(aTextureMemory)
							: "&nbsp;")
					<< "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< (aMemoryImage->mIsVolatile ? "Volatile" : "&nbsp;")
					<< "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< (aMemoryImage->mForcedMode ? "Forced" : "&nbsp;")
					<< "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< (aMemoryImage->mHasAlpha ? "HasAlpha" : "&nbsp;")
					<< "</TD>" << std::endl;

		aDumpStream << "<TD>"
					<< (aMemoryImage->mHasTrans ? "HasTrans" : "&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>"
					<< ((aNativeAlphaMemory != 0) ? "NativeAlpha<BR>" + CommaSeperate(aNativeAlphaMemory) :"&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>"
					<<((aRLAlphaMemory != 0) ? "RLAlpha<BR>" + CommaSeperate(aRLAlphaMemory) : "&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>"
					<< ((aRLAdditiveMemory != 0)
											  ? "RLAdditive<BR>" + CommaSeperate(aRLAdditiveMemory)
											  : "&nbsp;")
					<< "</TD>" << std::endl;
		aDumpStream << "<TD>" << (aMemoryImage->mFilePath.empty() ? "&nbsp;" : aMemoryImage->mFilePath) << "</TD>"
					<< std::endl;

		aDumpStream << "</TR>" << std::endl;

		// Write thumb

		MemoryImage aCopiedImage(*aMemoryImage);

		ulong *aBits = aCopiedImage.GetBits();

		uint32_t *aThumbBitsPtr = (uint32_t *)anImageLibImage.mBits;

		for (int aThumbY = 0; aThumbY < aThumbHeight; aThumbY++)
			for (int aThumbX = 0; aThumbX < aThumbWidth; aThumbX++)
			{
				int aSrcX = (int)(aCopiedImage.mWidth * (aThumbX + 0.5)) / aThumbWidth;
				int aSrcY = (int)(aCopiedImage.mHeight * (aThumbY + 0.5)) / aThumbHeight;

				*(aThumbBitsPtr++) = aBits[aSrcX + (aSrcY * aCopiedImage.mWidth)];
			}

		ImageLib::WriteImage((GetAppDataFolder() + std::string("_dump/") + aThumbName).c_str(), ".jpeg",
							 &anImageLibImage);

		// Write high resolution image

		ImageLib::Image anFullImage;
		anFullImage.mBits = aCopiedImage.GetBits();
		anFullImage.mWidth = aCopiedImage.GetWidth();
		anFullImage.mHeight = aCopiedImage.GetHeight();

		ImageLib::WriteImage((GetAppDataFolder() + std::string("_dump/") + anImageName).c_str(), ".png", &anFullImage);

		anFullImage.mBits = nullptr;

		anImgNum++;

		aSortedItr++;
	}

	aDumpStream << "<TD>Totals</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalMemorySize) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalBitsMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalPalletizedMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalSurfaceMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalTextureMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalNativeAlphaMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalRLAlphaMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << CommaSeperate(aTotalRLAdditiveMemory) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;

	aDumpStream << "</TABLE></CENTER></BODY></HTML>" << std::endl;
}

double AppBase::GetLoadingThreadProgress()
{
	if (mLoaded)
		return 1.0;
	if (!mLoadingThreadStarted)
		return 0.0;
	if (mNumLoadingThreadTasks == 0)
		return 0.0;
	return std::min(mCompletedLoadingThreadTasks / (double)mNumLoadingThreadTasks, 1.0);
}

bool AppBase::RegistryWrite(const std::string &theValueName, JSON_RTYPE theType, const uchar *theValue, ulong theLength)
{
	// H522
	std::filesystem::path config = GetAppDataFolder() + mRegKey + "/registry.json"; // always registry.json
	std::filesystem::create_directories(config.parent_path());
	//SDL_Log("%s", config.string().c_str());

	nlohmann::json j;
	std::ifstream inFile(config);
	if (inFile)
	{
		try
		{
			inFile >> j;
		}
		catch (...)
		{
		}
	}

	switch (theType)
	{
	case JSON_STRING:
		j[theValueName] = std::string(reinterpret_cast<const char *>(theValue), theLength);
		break;
	case JSON_INTEGER:
		if (theLength == sizeof(int))
			j[theValueName] = *reinterpret_cast<const int *>(theValue);
		break;
	case JSON_BOOLEAN:
		if (theLength == sizeof(int))
			j[theValueName] = (*reinterpret_cast<const int *>(theValue)) != 0;
		break;
	case JSON_DATA: {
		std::vector<uchar> bin(theValue, theValue + theLength);
		j[theValueName] = bin;
		break;
	}
	default:
		return false;
	}

	std::ofstream outFile(config);
	if (outFile)
		outFile << j.dump(4);

	return true;
}

bool AppBase::RegistryWriteString(const std::string &theValueName, const std::string &theString)
{
	return RegistryWrite(theValueName, JSON_STRING, (uchar *)theString.c_str(), theString.length());
}

bool AppBase::RegistryWriteInteger(const std::string &theValueName, int theValue)
{
	return RegistryWrite(theValueName, JSON_INTEGER, (uchar *)&theValue, sizeof(int));
}

bool AppBase::RegistryWriteBoolean(const std::string &theValueName, bool theValue)
{
	int aValue = theValue ? 1 : 0;
	return RegistryWrite(theValueName, JSON_BOOLEAN, (uchar *)&aValue, sizeof(int));
}

bool AppBase::RegistryWriteData(const std::string &theValueName, const uchar *theValue, ulong theLength)
{
	return RegistryWrite(theValueName, JSON_DATA, (uchar *)theValue, theLength);
}

void AppBase::WriteToRegistry()
{
	RegistryWriteInteger("MusicVolume", (int)(mMusicVolume * 100));
	RegistryWriteInteger("SfxVolume", (int)(mSfxVolume * 100));
	RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount > 0) ? 1 : 0);
	RegistryWriteInteger("ScreenMode", mIsWindowed ? 0 : 1);
	RegistryWriteInteger("PreferredX", mPreferredX);
	RegistryWriteInteger("PreferredY", mPreferredY);
	RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);
	RegistryWriteInteger("InProgress", 0);
	RegistryWriteBoolean("WaitForVSync", mWaitForVSync);
}

bool AppBase::RegistryEraseKey(const PopString &_theKeyName)
{
	std::filesystem::path basePath = GetAppDataFolder();
	std::filesystem::path keyPath = basePath / _theKeyName / "registry.json";

	if (std::filesystem::exists(keyPath))
	{
		std::error_code ec;
		std::filesystem::remove(keyPath, ec);
		return !ec;
	}

	return true;
}

void AppBase::RegistryEraseValue(const PopString &_theValueName)
{
	// H522
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	std::string keyName = _theValueName;

	if (!std::filesystem::exists(configPath))
		return;

	nlohmann::json j;
	std::ifstream inFile(configPath);
	if (inFile)
	{
		try
		{
			inFile >> j;
		}
		catch (...)
		{
			return;
		}
	}

	if (j.contains(keyName))
	{
		j.erase(keyName);

		std::ofstream outFile(configPath);
		if (outFile)
			outFile << j.dump(4);
	}

	return;
}

bool AppBase::RegistryGetSubKeys(const std::string &theKeyName, StringVector *theSubKeys)
{
	return false;
}

bool AppBase::RegistryRead(const std::string &theValueName, JSON_RTYPE *theType, uchar *theValue, ulong *theLength)
{
	return RegistryReadKey(theValueName, theType, theValue, theLength, 0);
}

bool AppBase::RegistryReadKey(const std::string &theValueName, JSON_RTYPE *theType, uchar *theValue, ulong *theLength,
							  ulong theKey)
{
	// H522
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	if (!std::filesystem::exists(configPath) || !theType || !theValue || !theLength)
		return false;

	nlohmann::json j;
	std::ifstream inFile(configPath);
	if (!inFile)
		return false;
	try
	{
		inFile >> j;
	}
	catch (...)
	{
		return false;
	}

	if (!j.contains(theValueName))
		return false;

	auto &entry = j[theValueName];
	// JSON_STRING
	if (entry.is_string())
	{
		std::string s = entry.get<std::string>();
		if (s.size() > *theLength)
			return false;
		std::memcpy(theValue, s.data(), s.size());
		*theLength = static_cast<ulong>(s.size());
		*theType = JSON_STRING;
		return true;
	}
	// JSON_INTEGER
	else if (entry.is_number_integer())
	{
		if (*theLength < sizeof(int))
			return false;
		int v = entry.get<int>();
		std::memcpy(theValue, &v, sizeof(int));
		*theLength = sizeof(int);
		*theType = JSON_INTEGER;
		return true;
	}
	// JSON_BOOLEAN
	else if (entry.is_boolean())
	{
		if (*theLength < sizeof(int))
			return false;
		int b = entry.get<bool>() ? 1 : 0;
		std::memcpy(theValue, &b, sizeof(int));
		*theLength = sizeof(int);
		*theType = JSON_BOOLEAN;
		return true;
	}
	// JSON_DATA
	else if (entry.is_array())
	{
		size_t size = entry.size();
		if (size > *theLength)
			return false;
		for (size_t i = 0; i < size; ++i)
			theValue[i] = static_cast<uchar>(entry[i].get<int>());
		*theLength = static_cast<ulong>(size);
		*theType = JSON_DATA;
		return true;
	}

	return false;
}

bool AppBase::RegistryReadString(const std::string &theKey, std::string *theString)
{
	// H522
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	if (!std::filesystem::exists(configPath) || !theString)
		return false;

	nlohmann::json j;
	std::ifstream inFile(configPath);
	if (!inFile)
		return false;
	try
	{
		inFile >> j;
	}
	catch (...)
	{
		return false;
	}

	if (j.contains(theKey) && j[theKey].is_string())
	{
		*theString = j[theKey].get<std::string>();
		return true;
	}
	return false;
}

bool AppBase::RegistryReadInteger(const std::string &theKey, int *theValue)
{
	// H522
	if (!theValue)
		return false;

	std::string s;

	nlohmann::json j;
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	std::ifstream inFile(configPath);
	if (!inFile)
		return false;
	try
	{
		inFile >> j;
	}
	catch (...)
	{
		return false;
	}

	if (j.contains(theKey) && j[theKey].is_number_integer())
	{
		*theValue = j[theKey].get<int>();
		return true;
	}
	return false;
}

bool AppBase::RegistryReadBoolean(const std::string &theKey, bool *theValue)
{
	// H522
	if (!theValue)
		return false;

	nlohmann::json j;
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	std::ifstream inFile(configPath);
	if (!inFile)
		return false;
	try
	{
		inFile >> j;
	}
	catch (...)
	{
		return false;
	}

	if (j.contains(theKey) && j[theKey].is_boolean())
	{
		*theValue = j[theKey].get<bool>();
		return true;
	}
	return false;
}

bool AppBase::RegistryReadData(const std::string &theKey, uchar *theValue, ulong *theLength)
{
	// H522
	if (!theValue || !theLength)
		return false;

	nlohmann::json j;
	std::filesystem::path configPath = GetAppDataFolder() + mRegKey + "/registry.json";
	std::ifstream inFile(configPath);
	if (!inFile)
		return false;
	try
	{
		inFile >> j;
	}
	catch (...)
	{
		return false;
	}

	if (j.contains(theKey) && j[theKey].is_array())
	{
		size_t size = j[theKey].size();
		if (size > *theLength)
			return false;
		for (size_t i = 0; i < size; ++i)
			theValue[i] = static_cast<uchar>(j[theKey][i].get<int>());
		*theLength = static_cast<ulong>(size);
		return true;
	}
	return false;
}

void AppBase::ReadFromRegistry()
{
	mReadFromRegistry = true;
	mRegKey = GetString("RegistryKey", mRegKey);

	if (mRegKey.length() == 0)
		return;

	int anInt;
	if (RegistryReadInteger("MusicVolume", &anInt))
		mMusicVolume = anInt / 100.0;

	if (RegistryReadInteger("SfxVolume", &anInt))
		mSfxVolume = anInt / 100.0;

	if (RegistryReadInteger("Muted", &anInt))
		mMuteCount = anInt;

	if (RegistryReadInteger("ScreenMode", &anInt))
		mIsWindowed = anInt == 0;

	RegistryReadInteger("PreferredX", &mPreferredX);
	RegistryReadInteger("PreferredY", &mPreferredY);

	if (RegistryReadInteger("CustomCursors", &anInt))
		EnableCustomCursors(anInt != 0);

	RegistryReadBoolean("WaitForVSync", &mWaitForVSync);

	if (RegistryReadInteger("InProgress", &anInt))
		mLastShutdownWasGraceful = anInt == 0;

	if (!IsScreenSaver())
		RegistryWriteInteger("InProgress", 1);
}

bool AppBase::WriteBytesToFile(const std::string &theFileName, const void *theData, ulong theDataLen)
{
	MkDir(GetFileDir(theFileName));
	FILE *aFP = fopen(theFileName.c_str(), "w+b");

	if (aFP == nullptr)
	{
		return false;
	}

	fwrite(theData, 1, theDataLen, aFP);
	fclose(aFP);

	return true;
}

bool AppBase::WriteBufferToFile(const std::string &theFileName, const Buffer *theBuffer)
{
	return WriteBytesToFile(theFileName, theBuffer->GetDataPtr(), theBuffer->GetDataLen());
}

bool AppBase::ReadBufferFromFile(const std::string &theFileName, Buffer *theBuffer)
{
	FILE *aFP = fopen(theFileName.c_str(), "rb");

	if (!aFP)
		return false;

	fseek(aFP, 0, SEEK_END);
	int aFileSize = ftell(aFP);
	fseek(aFP, 0, SEEK_SET);

	uchar *aData = new uchar[aFileSize];

	fread(aData, 1, aFileSize, aFP);
	fclose(aFP);

	theBuffer->Clear();
	theBuffer->SetData(aData, aFileSize);

	delete[] aData;

	return true;
}

bool AppBase::FileExists(const std::string &theFileName)
{
	FILE *aFP = fopen(theFileName.c_str(), "rb");

	if (!aFP)
		return false;

	fclose(aFP);
	return true;
}

bool AppBase::EraseFile(const std::string &theFileName)
{
	return remove(theFileName.c_str()) != 0;
}

void AppBase::SEHOccured()
{
	SetMusicVolume(0);
	mSEHOccured = true;
	EnforceCursor();
}

std::string AppBase::GetGameSEHInfo()
{
	int aSecLoaded = (SDL_GetTicks() - mTimeLoaded) / 1000;

	char aTimeStr[16];
	sprintf(aTimeStr, "%02d:%02d:%02d", (aSecLoaded / 60 / 60), (aSecLoaded / 60) % 60, aSecLoaded % 60);

	char aThreadIdStr[16];
	sprintf(aThreadIdStr, "%X", mPrimaryThreadId);

	std::string anInfoString = "Product: " + mProdName + "\r\n" + "Version: " + mProductVersion + "\r\n";

	anInfoString += "Time Loaded: " + std::string(aTimeStr) +
					"\r\n"
					"Fullscreen: " +
					(mIsWindowed ? std::string("No") : std::string("Yes")) +
					"\r\n"
					"Primary ThreadId: " +
					aThreadIdStr + "\r\n";

	return anInfoString;
}

void AppBase::GetSEHWebParams(DefinesMap *theDefinesMap)
{
}

void AppBase::ShutdownHook()
{
}

void AppBase::Shutdown()
{
	if ((mPrimaryThreadId != 0) && (SDL_GetCurrentThreadID() != mPrimaryThreadId))
	{
		mLoadingFailed = true;
	}
	else if (!mShutdown)
	{
		mExitToTop = true;
		mShutdown = true;
		ShutdownHook();

		// Blah
		while (mCursorThreadRunning)
		{
			SDL_Delay(10);
		}

		if (mMusicInterface != nullptr)
			mMusicInterface->StopAllMusic();

		RestoreScreenResolution();

		if (mReadFromRegistry)
			WriteToRegistry();
	}
}

void AppBase::RestoreScreenResolution()
{
	if (mFullScreenWindow)
	{
		mFullScreenWindow = false;
	}
}

void AppBase::DoExit(int theCode)
{
	RestoreScreenResolution();
	exit(theCode);
}

extern bool demoWind;
extern bool debugWind;

void AppBase::UpdateFrames()
{
	mUpdateCount++;

	if (!mMinimized)
	{
		if (mWidgetManager->UpdateFrame())
			++mFPSDirtyCount;
	}

	mMusicInterface->Update();
	CleanSharedImages();
}

void AppBase::DoUpdateFramesF(float theFrac)
{
	if ((mVSyncUpdates) && (!mMinimized))
		mWidgetManager->UpdateFrameF(theFrac);
}

bool AppBase::DoUpdateFrames()
{
	AUTO_PERF("AppBase::DoUpdateFrames");

	if (gScreenSaverActive)
		return false;

	if ((mLoadingThreadCompleted) && (!mLoaded))
	{
		SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
		mLoaded = true;
		mYieldMainThread = false;
		LoadingThreadCompleted();
	}

	UpdateFrames();
	return true;
}

bool gIsFailing = false;

void AppBase::Redraw(Rect *theClipRect)
{
	AUTO_PERF("AppBase::Redraw");

	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if ((mIsDrawing) || (mShutdown))
		return;

	if (gScreenSaverActive)
		return;

	mSDLInterface->Redraw(theClipRect);

	mFPSFlipCount++;
}

///////////////////////////// FPS Stuff

#include "graphics/HeaderFont/LiberationSansRegular.h"

static PerfTimer gFPSTimer;
static int gFrameCount;
static int gFPSDisplay;
static bool gForceDisplay = false;
static void CalculateFPS()
{
	gFrameCount++;

	static SysFont aFont(gAppBase, LiberationSans_Regular, LiberationSans_Regular_Size, 8);
	if (gFPSImage == nullptr)
	{
		gFPSImage = new SDLImage(gAppBase->mSDLInterface);
		gFPSImage->Create(50, aFont.GetHeight() + 4);
		gFPSImage->SetImageMode(false, false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		// gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	if (gFPSTimer.GetDuration() >= 1000 || gForceDisplay)
	{
		gFPSTimer.Stop();
		if (!gForceDisplay)
			gFPSDisplay = (int)(gFrameCount * 1000 / gFPSTimer.GetDuration() + 0.5f);
		else
		{
			gForceDisplay = false;
			gFPSDisplay = 0;
		}

		gFPSTimer.Start();
		gFrameCount = 0;

		Graphics aDrawG(gFPSImage);
		aDrawG.SetFont(&aFont);
		PopString aFPS = StrFormat("FPS: %d", gFPSDisplay);
		aDrawG.SetColor(Color(0, 0, 0));
		aDrawG.FillRect(0, 0, gFPSImage->GetWidth(), gFPSImage->GetHeight());
		aDrawG.SetColor(Color(255, 255, 255));
		aDrawG.DrawString(aFPS, 2, aFont.GetAscent());
		// gFPSImage->mKeepBits = false;
		// gFPSImage->GenerateDDSurface();
		gFPSImage->mBitsChangedCount++;
	}
}

///////////////////////////// FPS Stuff to draw mouse coords
static void FPSDrawCoords(int theX, int theY)
{
	static SysFont aFont(gAppBase, LiberationSans_Regular, LiberationSans_Regular_Size, 8);
	if (gFPSImage == nullptr)
	{
		gFPSImage = new SDLImage(gAppBase->mSDLInterface);
		gFPSImage->Create(50, aFont.GetHeight() + 4);
		gFPSImage->SetImageMode(false, false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		// gFPSImage->mWantDDSurface = true;
		gFPSImage->PurgeBits();
	}

	Graphics aDrawG(gFPSImage);
	aDrawG.SetFont(&aFont);
	PopString aFPS = StrFormat("%d,%d", theX, theY);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0, 0, gFPSImage->GetWidth(), gFPSImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS, 2, aFont.GetAscent());
	gFPSImage->mBitsChangedCount++;
}

static void UpdateScreenSaverInfo(uint32_t theTick)
{
	if (gAppBase->IsScreenSaver() || !gAppBase->mIsPhysWindowed)
		return;
}

bool AppBase::DrawDirtyStuff()
{
	AUTO_PERF("AppBase::DrawDirtyStuff");
	MTAutoDisallowRand aDisallowRand;

	if (gIsFailing) // just try to reinit
	{
		Redraw(nullptr);
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}

	if (mShowFPS)
	{
		switch (mShowFPSMode)
		{
		case FPS_ShowFPS:
			CalculateFPS();
			break;
		case FPS_ShowCoords:
			if (mWidgetManager != nullptr)
				FPSDrawCoords(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
			break;
		}
	}

	uint32_t aStartTime = SDL_GetTicks();

	// Update user input and screen saver info
	static uint32_t aPeriodicTick = 0;
	if (aStartTime - aPeriodicTick > 1000)
	{
		aPeriodicTick = aStartTime;
		UpdateScreenSaverInfo(aStartTime);
	}

	if (gScreenSaverActive)
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}

	mIsDrawing = true;
	bool drewScreen = mWidgetManager->DrawScreen();
	mIsDrawing = false;

	if ((drewScreen || (aStartTime - mLastDrawTick >= 1000) || (mCustomCursorDirty)) &&
		((int)(aStartTime - mNextDrawTick) >= 0))
	{
		mLastDrawWasEmpty = false;

		mDrawCount++;

		uint32_t aMidTime = SDL_GetTicks();

		mFPSCount++;
		mFPSTime += aMidTime - aStartTime;

		mDrawTime += aMidTime - aStartTime;

		if (mShowFPS)
		{
			Graphics g(mSDLInterface->GetScreenImage());
			g.DrawImage(gFPSImage, mWidth - gFPSImage->GetWidth() - 10, mHeight - gFPSImage->GetHeight() - 10);
		}

		if (mWaitForVSync && mIsPhysWindowed && mSoftVSyncWait)
		{
			uint32_t aTick = SDL_GetTicks();
			if (aTick - mLastDrawTick < mSDLInterface->mMillisecondsPerFrame)
				SDL_Delay(mSDLInterface->mMillisecondsPerFrame - (aTick - mLastDrawTick));
		}

		uint32_t aPreScreenBltTime = SDL_GetTicks();
		mLastDrawTick = aPreScreenBltTime;

		Redraw(nullptr);

		// This is our one UpdateFTimeAcc if we are vsynched
		UpdateFTimeAcc();

		uint32_t aEndTime = SDL_GetTicks();

		mScreenBltTime = aEndTime - aPreScreenBltTime;

#ifdef _DEBUG
		/*if (mFPSTime >= 5000) // Show FPS about every 5 seconds
		{
			ulong aTickNow = SDL_GetTicks();

			OutputDebugString(StrFormat(_S("Theoretical FPS: %d\r\n"), (int) (mFPSCount * 1000 / mFPSTime)).c_str());
			OutputDebugString(StrFormat(_S("Actual      FPS: %d\r\n"), (mFPSFlipCount * 1000) / max((aTickNow -
		mFPSStartTick), 1)).c_str()); OutputDebugString(StrFormat(_S("Dirty Rate     : %d\r\n"), (mFPSDirtyCount * 1000)
		/ max((aTickNow - mFPSStartTick), 1)).c_str());

			mFPSTime = 0;
			mFPSCount = 0;
			mFPSFlipCount = 0;
			mFPSStartTick = aTickNow;
			mFPSDirtyCount = 0;
		}*/
#endif

		if ((mLoadingThreadStarted) && (!mLoadingThreadCompleted))
		{
			int aTotalTime = aEndTime - aStartTime;

			mNextDrawTick += 35 + std::max(aTotalTime, 15);

			if ((int)(aEndTime - mNextDrawTick) >= 0)
				mNextDrawTick = aEndTime;

			/*char aStr[256];
			sprintf(aStr, "Next Draw Time: %d\r\n", mNextDrawTick);
			OutputDebugString(aStr);*/
		}
		else
			mNextDrawTick = aEndTime;

		mHasPendingDraw = false;
		mCustomCursorDirty = false;

		return true;
	}
	else
	{
		mHasPendingDraw = false;
		mLastDrawWasEmpty = true;
		return false;
	}
}

void AppBase::LogScreenSaverError(const std::string &theError)
{
	static bool firstTime = true;
	char aBuf[512];

	const char *aFlag = firstTime ? "w" : "a+";
	firstTime = false;

	FILE *aFile = fopen("ScrError.txt", aFlag);
	if (aFile != nullptr)
	{
#ifdef _WIN32
		fprintf(aFile, "%s %s %u\n", theError.c_str(), _strtime(aBuf), SDL_GetTicks());
#else
		char aBuf[9];  // HH:MM:SS
		time_t now = time(nullptr);
		strftime(aBuf, sizeof(aBuf), "%H:%M:%S", localtime(&now));

		fprintf(aFile, "%s %s %u\n", theError.c_str(), aBuf, SDL_GetTicks());
#endif
		fclose(aFile);
	}
}

void AppBase::BeginPopup()
{
	// if (!mIsPhysWindowed)
	//{
	// if (mSDLInterface && mDDInterface->mDD)
	//{
	// mDDInterface->mDD->FlipToGDISurface();
	// mNoDefer = true;
	//}
	//}
}

void AppBase::EndPopup()
{
	if (!mIsPhysWindowed)
		mNoDefer = false;

	ClearUpdateBacklog();
	ClearKeysDown();

	if (mWidgetManager->mDownButtons)
	{
		mWidgetManager->DoMouseUps();
//		ReleaseCapture();
	}
}

int AppBase::MsgBox(const std::string &theText, const std::string &theTitle, int theFlags)
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(theText);
		return 0;
	}

	SDL_MessageBoxData messageBoxData = {
		SDL_MESSAGEBOX_INFORMATION, NULL, theTitle.c_str(), theText.c_str(), NULL, NULL, NULL};

	if (theFlags & MsgBoxFlags::MsgBox_OK)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
		};

		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}
	else if (theFlags & MsgBoxFlags::MsgBox_OKCANCEL)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
			{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel"},
		};
		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}
	else if (theFlags & MsgBoxFlags::MsgBox_ABORTRETRYIGNORE)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Abort"},
			{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Retry"},
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "Ignore"},
		};
		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}
	else if (theFlags & MsgBoxFlags::MsgBox_YESNOCANCEL)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes"},
			{0, 1, "No"},
			{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel"},
		};
		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}
	else if (theFlags & MsgBoxFlags::MsgBox_YESNO)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes"},
			{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "No"},
		};
		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}
	else if (theFlags & MsgBoxFlags::MsgBox_RETRYCANCEL)
	{
		SDL_MessageBoxButtonData buttons[] = {
			{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry"},
			{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel"},
		};
		messageBoxData.numbuttons = SDL_arraysize(buttons);
		messageBoxData.buttons = buttons;
	}

	BeginPopup();
	int aResult = mSDLInterface->MakeResultMessageBox(messageBoxData);
	EndPopup();

	return aResult;
}

void AppBase::Popup(const std::string &theString)
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(theString);
		return;
	}

	BeginPopup();
	if (!mShutdown)
		mSDLInterface->MakeSimpleMessageBox(GetString("FATAL_ERROR", "FATAL ERROR").c_str(),
											theString.c_str(), SDL_MESSAGEBOX_ERROR);
	EndPopup();
}

void AppBase::SafeDeleteWidget(Widget *theWidget)
{
	WidgetSafeDeleteInfo aWidgetSafeDeleteInfo;
	aWidgetSafeDeleteInfo.mUpdateAppDepth = mUpdateAppDepth;
	aWidgetSafeDeleteInfo.mWidget = theWidget;
	mSafeDeleteList.push_back(aWidgetSafeDeleteInfo);
}

void AppBase::HandleNotifyGameMessage(int theType, int theParam)
{
}

void AppBase::RehupFocus()
{
	bool wantHasFocus = mActive && !mMinimized;

	if (wantHasFocus != mHasFocus)
	{
		mHasFocus = wantHasFocus;

		if (mHasFocus)
		{
			if (mMuteOnLostFocus)
				Unmute(true);

			mWidgetManager->GotFocus();
			GotFocus();
		}
		else
		{
			if (mMuteOnLostFocus)
				Mute(true);

			mWidgetManager->LostFocus();
			LostFocus();

//			ReleaseCapture();
			mWidgetManager->DoMouseUps();
		}
	}
}

void AppBase::ClearKeysDown()
{
	if (mWidgetManager != nullptr) // fix stuck alt-key problem
	{
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
			mWidgetManager->mKeyDown[aKeyNum] = false;
	}
	mCtrlDown = false;
	mAltDown = false;
}

void AppBase::ShowMemoryUsage()
{
	/*
	uint32_t aTotal = 0;
	uint32_t aFree = 0;

	if (mDDInterface->mDD7 != nullptr)
	{
		DDSCAPS2 aCaps;
		ZeroMemory(&aCaps,sizeof(aCaps));
		aCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
		mDDInterface->mDD7->GetAvailableVidMem(&aCaps,&aTotal,&aFree);
	}

	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	typedef std::pair<int,int> FormatUsage;
	typedef std::map<PixelFormat,FormatUsage> FormatMap;
	FormatMap aFormatMap;
	int aTextureMemory = 0;
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		if (aMemoryImage->mD3DData != nullptr)
		{
			TextureData *aData = (TextureData*)aMemoryImage->mD3DData;
			aTextureMemory += aData->mTexMemSize;

			FormatUsage &aUsage = aFormatMap[aData->mPixelFormat];
			aUsage.first++;
			aUsage.second += aData->mTexMemSize;
		}

		++anItr;
	}

	std::string aStr;

	const char *aDesc;
	if (Is3DAccelerationRecommended())
		aDesc = "Recommended";
	else if (Is3DAccelerationSupported())
		aDesc = "Supported";
	else
		aDesc = "Unsupported";

	aStr += StrFormat("3D-Mode is %s (3D is %s on this system)\r\n\r\n",Is3DAccelerated()?"On":"Off",aDesc);

	aStr += StrFormat("Num Images: %d\r\n",(int)mMemoryImageSet.size());
	aStr += StrFormat("Num Sounds: %d\r\n",mSoundManager->GetNumSounds());
	aStr += StrFormat("Video Memory: %s/%s KB\r\n", PopStringToString(CommaSeperate((aTotal-aFree)/1024)).c_str(),
	PopStringToString(CommaSeperate(aTotal/1024)).c_str()); aStr += StrFormat("Texture Memory: %s
	KB\r\n",CommaSeperate(aTextureMemory/1024).c_str());

	FormatUsage aUsage = aFormatMap[PixelFormat_A8R8G8B8];
	aStr += StrFormat("A8R8G8B8: %d - %s
	KB\r\n",aUsage.first,PopStringToString(CommaSeperate(aUsage.second/1024)).c_str()); aUsage =
	aFormatMap[PixelFormat_A4R4G4B4]; aStr += StrFormat("A4R4G4B4: %d - %s
	KB\r\n",aUsage.first,PopStringToString(CommaSeperate(aUsage.second/1024)).c_str()); aUsage =
	aFormatMap[PixelFormat_R5G6B5]; aStr += StrFormat("R5G6B5: %d - %s
	KB\r\n",aUsage.first,PopStringToString(CommaSeperate(aUsage.second/1024)).c_str()); aUsage =
	aFormatMap[PixelFormat_Palette8]; aStr += StrFormat("Palette8: %d - %s
	KB\r\n",aUsage.first,PopStringToString(CommaSeperate(aUsage.second/1024)).c_str());

	MsgBox(aStr,"Video Stats",MB_OK);
	mLastTime = SDL_GetTicks();
	*/
}

bool AppBase::IsAltKeyUsed(long wParam)
{
	int aChar = tolower(wParam);
	switch (aChar)
	{
	case 13: // alt-enter
	case 'r':
		return true;
	default:
		return false;
	}
}

bool AppBase::DebugKeyDown(int theKey)
{
	if ((theKey == 'R') && (mWidgetManager->mKeyDown[KEYCODE_MENU]))
	{
#ifndef RELEASEFINAL
		if (!ReparseModValues())
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mWidgetManager->mKeyDown[aKeyNum] = false;
		}
#endif
	}
	else if (theKey == SDLK_F3)
	{
		if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mShowFPS = true;
			if (++mShowFPSMode >= Num_FPS_Types)
				mShowFPSMode = 0;
		}
		else
			mShowFPS = !mShowFPS;

		mWidgetManager->MarkAllDirty();

		if (mShowFPS)
		{
			gFPSTimer.Start();
			gFrameCount = 0;
			gFPSDisplay = 0;
			gForceDisplay = true;
		}
	}
	else if (theKey == SDLK_F8)
	{
		if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			Set3DAcclerated(!Is3DAccelerated());

			char aBuf[512];
			sprintf(aBuf, "3D-Mode: %s", Is3DAccelerated() ? "ON" : "OFF");
			MsgBox(aBuf, "Mode Switch", 0);
			mLastTime = SDL_GetTicks();
		}
		else
			ShowMemoryUsage();

		return true;
	}
	else if (theKey == SDLK_F10)
	{
#ifndef RELEASEFINAL
		if (mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			if (mUpdateMultiplier == 0.25)
				mUpdateMultiplier = 1.0;
			else
				mUpdateMultiplier = 0.25;
		}
		else if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mStepMode = 0;
			ClearUpdateBacklog();
		}
		else
			mStepMode = 1;
#endif

		return true;
	}
	else if (theKey == SDLK_F11)
	{
		SDL_Keymod mod = SDL_GetModState();
		if (mod & SDL_KMOD_LSHIFT)
			DumpProgramInfo();
		else
			TakeScreenshot();

		return true;
	}
	else if (theKey == SDLK_0)
	{
		if (demoWind)
			demoWind = false;
		else
			demoWind = true;
	}
	else if (theKey == SDLK_9)
	{
		if (debugWind)
			debugWind = false;
		else
			debugWind = true;
	}
	else if (theKey == SDLK_F2)
	{
		bool isPerfOn = !Perf::IsPerfOn();
		if (isPerfOn)
		{
			//			MsgBox("Perf Monitoring: ON", "Perf Monitoring", MB_OK);
			ClearUpdateBacklog();
			Perf::BeginPerf();
		}
		else
		{
			Perf::EndPerf();
			MsgBox(Perf::GetResults().c_str(), "Perf Results", 0);
			ClearUpdateBacklog();
		}
	}
	else
		return false;

	return false;
}

bool AppBase::DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown)
{
	return false;
}

void AppBase::CloseRequestAsync()
{
}

// Why did I defer messages?  Oh, incase a dialog comes up such as a crash
//  it won't keep crashing and stuff
bool AppBase::ProcessDeferredMessages(bool singleMessage)
{
	SDL_Event event;
	if (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL3_ProcessEvent(&event);

		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			Shutdown();
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			if ((!gInAssert) && (!mSEHOccured) && (!mShutdown))
			{
				mActive = true;
				RehupFocus();
				if (!mIsWindowed)
					mWidgetManager->MarkAllDirty();
				if (mIsOpeningURL && !mActive)
					URLOpenSucceeded(mOpeningURL);
			}
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			mActive = false;
			RehupFocus();
			if (mIsOpeningURL && mActive)
				URLOpenFailed(mOpeningURL);
			break;
		case SDL_EVENT_WINDOW_MINIMIZED:
			mMinimized = true;
			if (mMuteOnLostFocus)
				Mute(true);
			break;
		case SDL_EVENT_WINDOW_RESTORED:
			mMinimized = false;
			if (mMuteOnLostFocus)
				Unmute(true);
			mWidgetManager->MarkAllDirty();
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (!gInAssert && !mSEHOccured)
			{
				int x = event.motion.x;
				int y = event.motion.y;
				mWidgetManager->RemapMouse(x, y);
				mLastUserInputTick = mLastTimerTime;
				mWidgetManager->MouseMove(x, y);
				if (!mMouseIn)
				{
					mMouseIn = true;
					EnforceCursor();
				}
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (!gInAssert && !mSEHOccured)
			{
				int btnCode = 0;
				bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;

				switch (event.button.button)
				{
				case SDL_BUTTON_LEFT:
					btnCode = 1;
					break;
				case SDL_BUTTON_RIGHT:
					btnCode = -1;
					break;
				case SDL_BUTTON_MIDDLE:
					btnCode = 3;
					break;
				}

				int x = event.button.x;
				int y = event.button.y;

				int renderWidth, renderHeight;
				SDL_GetCurrentRenderOutputSize(mSDLInterface->mRenderer, &renderWidth, &renderHeight);

				int scaledX = static_cast<int>(event.button.x * ((float)mWidth / renderWidth));
				int scaledY = static_cast<int>(event.button.y * ((float)mHeight / renderHeight));

				if (down)
					mWidgetManager->MouseDown(scaledX, scaledY, btnCode);
				else
					mWidgetManager->MouseUp(scaledX, scaledY, btnCode);
			}
			break;
		case SDL_EVENT_MOUSE_WHEEL:
			mWidgetManager->MouseWheel(event.wheel.y);

			break;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP: {
			bool isDown = event.type == SDL_EVENT_KEY_DOWN;
			SDL_Keycode key = event.key.key;

			mLastUserInputTick = mLastTimerTime;

			if (isDown && mDebugKeysEnabled && DebugKeyDown(key))
				break;

			if (isDown)
				mWidgetManager->KeyDown(GetKeyCodeFromSDLKeycode(key));
			else
				mWidgetManager->KeyUp(GetKeyCodeFromSDLKeycode(key));
		}
		break;
		case SDL_EVENT_TEXT_INPUT: {
			mLastUserInputTick = mLastTimerTime;

			PopChar aChar = event.text.text[0]; // assumes UTF-8 safe

			mWidgetManager->KeyChar((PopChar)aChar);
			break;
		}
		}
	}

	return SDL_HasEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
}

void AppBase::Done3dTesting()
{
}

// return file name that you want to upload
std::string AppBase::NotifyCrashHook()
{
	return "";
}

void AppBase::MakeWindow()
{
	if (mSDLInterface == nullptr)
	{
		mSDLInterface = new SDLInterface(this);

		// Enable 3d setting
		bool is3D = false;
		bool is3DOptionSet = RegistryReadBoolean("Is3D", &is3D);
		if (is3DOptionSet)
		{
			if (mAutoEnable3D)
			{
				mAutoEnable3D = false;
				mTest3D = true;
			}

			if (is3D)
				mTest3D = true;

			mSDLInterface->mIs3D = is3D;
		}
	}

	int aResult = InitSDLInterface();

	bool isActive = mActive;
	// mActive = GetActiveWindow() == mHWnd;

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mSDLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();

	//	SetTimer(mHWnd, 100, mFrameTime, nullptr);
}

void AppBase::DeleteNativeImageData()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage *aMemoryImage = *anItr;
		aMemoryImage->DeleteNativeData();
		++anItr;
	}
}

void AppBase::DeleteExtraImageData()
{
	AutoCrit anAutoCrit(mSDLInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage *aMemoryImage = *anItr;
		aMemoryImage->DeleteExtraBuffers();
		++anItr;
	}
}

void AppBase::ReInitImages()
{
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage *aMemoryImage = *anItr;
		aMemoryImage->ReInit();
		++anItr;
	}
}

void AppBase::LoadingThreadProc()
{
}

void AppBase::LoadingThreadCompleted()
{
}

int AppBase::LoadingThreadProcStub(void *theArg)
{
	AppBase *aPopLibApp = (AppBase *)theArg;

	aPopLibApp->LoadingThreadProc();

	char aStr[256];
	sprintf(aStr, "Resource Loading Time: %d\r\n", (SDL_GetTicks() - aPopLibApp->mTimeLoaded));
	SDL_Log(aStr);

	aPopLibApp->mLoadingThreadCompleted = true;

	return 0;
}

void AppBase::StartLoadingThread()
{
	if (!mLoadingThreadStarted)
	{
		mYieldMainThread = true;
		SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
		mLoadingThreadStarted = true;
		SDL_Thread *aThread = SDL_CreateThread(LoadingThreadProcStub, "LoadingThread", (void *)this);
		SDL_DetachThread(aThread);
	}
}
void AppBase::CursorThreadProc()
{
	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	SDL_Point aLastCursorPos = {0, 0};
	int aLastDrawCount = 0;

	while (!mShutdown)
	{

		SDL_Point aCursorPos;

		float x, y;

		SDL_GetMouseState(&x, &y);

		aCursorPos.x = (int)x;
		aCursorPos.y = (int)y;

		if (aLastDrawCount != mDrawCount)
		{
			// We did a draw so we may have committed a pending mNextCursorX/Y
			aLastCursorPos.x = mSDLInterface->mCursorX;
			aLastCursorPos.y = mSDLInterface->mCursorY;
		}

		if ((aCursorPos.x != aLastCursorPos.x) || (aCursorPos.y != aLastCursorPos.y))
		{
			Uint32 aTimeNow = SDL_GetTicks();
			if (aTimeNow - mNextDrawTick > mSDLInterface->mMillisecondsPerFrame + 5)
			{
				// Do the special drawing if we are rendering at less than full framerate
				mSDLInterface->SetCursorPos(aCursorPos.x, aCursorPos.y);
				aLastCursorPos = aCursorPos;
			}
			else
			{
				// Set them up to get assigned in the next screen redraw
				mSDLInterface->mNextCursorX = aCursorPos.x;
				mSDLInterface->mNextCursorY = aCursorPos.y;
			}
		}

		SDL_Delay(10);
	}

	mCursorThreadRunning = false;
}

int AppBase::CursorThreadProcStub(void *theArg)
{
	AppBase *aPopLibApp = (AppBase *)theArg;
	aPopLibApp->CursorThreadProc();
	return 0;
}

void AppBase::StartCursorThread()
{
	if (!mCursorThreadRunning)
	{
		mCursorThreadRunning = true;
		SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
		SDL_Thread *aThread = SDL_CreateThread(CursorThreadProcStub, "CursorThread", (void *)this);
		SDL_DetachThread(aThread);
	}
}

void AppBase::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mForceFullscreen)
		wantWindowed = false;

	if (mIsWindowed == wantWindowed && !force)
	{
		Set3DAcclerated(is3d);
		return;
	}

	// Set 3d acceleration preference
	Set3DAcclerated(is3d, false);

	// Always make the app windowed when playing demos, in order to
	//  make it easier to track down bugs.  We place this after the
	//  sanity check just so things get re-initialized and stuff
	// if (mPlayingDemoBuffer)
	//	wantWindowed = true;

	mIsWindowed = wantWindowed;

	MakeWindow();

	if (mSoundManager != nullptr)
	{
		mSoundManager->SetCooperativeWindow(mIsWindowed);
	}

	mLastTime = SDL_GetTicks();
}

void AppBase::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

void AppBase::SwitchScreenMode()
{
	SwitchScreenMode(mIsWindowed, Is3DAccelerated(), true);
}

void AppBase::SetAlphaDisabled(bool isDisabled)
{
	if (mAlphaDisabled != isDisabled)
	{
		mAlphaDisabled = isDisabled;
		mSDLInterface->SetVideoOnlyDraw(mAlphaDisabled);
		mWidgetManager->mImage = mSDLInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
}

void AppBase::EnforceCursor()
{
	bool wantSysCursor = true;

	if (mSDLInterface == nullptr)
		return;

	if ((mSEHOccured) || (!mMouseIn))
	{
		mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_DEFAULT);
		if (mSDLInterface->SetCursorImage(nullptr))
			mCustomCursorDirty = true;
	}
	else
	{
		if ((mCursorImages[mCursorNum] == nullptr) || ((!mCustomCursorsEnabled) && (mCursorNum != CURSOR_CUSTOM)))
		{
			switch (mCursorNum)
			{
			case CURSOR_POINTER:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_DEFAULT);
				break;
			case CURSOR_HAND:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_POINTER);
				break;
			case CURSOR_TEXT:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_TEXT);
				break;
			case CURSOR_DRAGGING:
				SDL_SetCursor(mDraggingCursor);
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_TEXT);
				break;
			case CURSOR_CIRCLE_SLASH:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
				break;
			case CURSOR_SIZEALL:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_MOVE);
				break;
			case CURSOR_SIZENESW:
			case CURSOR_SIZENS:
			case CURSOR_SIZENWSE:
			case CURSOR_SIZEWE:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
				break;
			case CURSOR_WAIT:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_WAIT);
				break;
			case CURSOR_CUSTOM:
			case CURSOR_NONE:
				SDL_SetCursor(nullptr);
				break;
			default:
				mSDLInterface->SetCursor(SDL_SYSTEM_CURSOR_DEFAULT);
				break;
			}

			if (mSDLInterface->SetCursorImage(nullptr))
				mCustomCursorDirty = true;
		}
		else
		{
			if (mSDLInterface->SetCursorImage(mCursorImages[mCursorNum]))
				mCustomCursorDirty = true;

			wantSysCursor = false;
		}
	}

	if (wantSysCursor != mSysCursor)
	{
		mSysCursor = wantSysCursor;

		// Don't hide the hardware cursor when playing back a demo buffer
		//		if (!mPlayingDemoBuffer)
		//			::ShowCursor(mSysCursor);
	}
}

void AppBase::ProcessSafeDeleteList()
{
	MTAutoDisallowRand aDisallowRand;

	WidgetSafeDeleteList::iterator anItr = mSafeDeleteList.begin();
	while (anItr != mSafeDeleteList.end())
	{
		WidgetSafeDeleteInfo *aWidgetSafeDeleteInfo = &(*anItr);
		if (mUpdateAppDepth <= aWidgetSafeDeleteInfo->mUpdateAppDepth)
		{
			delete aWidgetSafeDeleteInfo->mWidget;
			anItr = mSafeDeleteList.erase(anItr);
		}
		else
			++anItr;
	}
}

void AppBase::UpdateFTimeAcc()
{
	uint32_t aCurTime = SDL_GetTicks();

	if (mLastTimeCheck != 0)
	{
		int aDeltaTime = aCurTime - mLastTimeCheck;

		mUpdateFTimeAcc = std::min(mUpdateFTimeAcc + aDeltaTime, 200.0);

		if (mRelaxUpdateBacklogCount > 0)
			mRelaxUpdateBacklogCount = std::max(mRelaxUpdateBacklogCount - aDeltaTime, 0);
	}

	mLastTimeCheck = aCurTime;
}

// int aNumCalls = 0;
// uint32_t aLastCheck = 0;

bool AppBase::Process(bool allowSleep)
{
	/*uint32_t aTimeNow = SDL_GetTicks();
	if (aTimeNow - aLastCheck >= 10000)
	{
		OutputDebugString(StrFormat(_S("FUpdates: %d\n"), aNumCalls).c_str());
		aLastCheck = aTimeNow;
		aNumCalls = 0;
	}*/

	if (mLoadingFailed)
		Shutdown();

	bool isVSynched = mVSyncUpdates && (!mLastDrawWasEmpty) && (!mVSyncBroken) &&
					  ((!mIsPhysWindowed) || (mIsPhysWindowed && mWaitForVSync && !mSoftVSyncWait));
	double aFrameFTime;
	double anUpdatesPerUpdateF;

	if (mVSyncUpdates)
	{
		aFrameFTime = (1000.0 / mSyncRefreshRate) / mUpdateMultiplier;
		anUpdatesPerUpdateF = (float)(1000.0 / (mFrameTime * mSyncRefreshRate));
	}
	else
	{
		aFrameFTime = mFrameTime / mUpdateMultiplier;
		anUpdatesPerUpdateF = 1.0;
	}

	// Make sure we're not paused
	if ((!mPaused) && (mUpdateMultiplier > 0))
	{
		ulong aStartTime = SDL_GetTicks();

		ulong aCurTime = aStartTime;
		int aCumSleepTime = 0;

		// When we are VSynching, only calculate this FTimeAcc right after drawing

		if (!isVSynched)
			UpdateFTimeAcc();

		// mNonDrawCount is used to make sure we draw the screen at least
		// 10 times per second, even if it means we have to slow down
		// the updates to make it draw 10 times per second in "game time"

		bool didUpdate = false;

		if (mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			if ((++mNonDrawCount < (int)ceil(10 * mUpdateMultiplier)) || (!mLoaded))
			{
				bool doUpdate = false;

				if (isVSynched)
				{
					// Synch'ed to vertical refresh, so update as soon as possible after draw
					doUpdate = (!mHasPendingDraw) || (mUpdateFTimeAcc >= (int)(aFrameFTime * 0.75));
				}
				else if (mUpdateFTimeAcc >= aFrameFTime)
				{
					doUpdate = true;
				}

				if (doUpdate)
				{
					// Do VSyncBroken test.  This test fails if we're in fullscreen and
					// "don't vsync" has been forced in Advanced settings up Display Properties
					if (mUpdateMultiplier == 1.0)
					{
						mVSyncBrokenTestUpdates++;
						if (mVSyncBrokenTestUpdates >= (uint32_t)((1000 + mFrameTime - 1) / mFrameTime))
						{
							// It has to be running 33% fast to be "broken" (25% = 1/0.800)
							if (aStartTime - mVSyncBrokenTestStartTick <= 800)
							{
								// The test has to fail 3 times in a row before we decide that
								//  vsync is broken overall
								mVSyncBrokenCount++;
								if (mVSyncBrokenCount >= 3)
									mVSyncBroken = true;
							}
							else
								mVSyncBrokenCount = 0;

							mVSyncBrokenTestStartTick = aStartTime;
							mVSyncBrokenTestUpdates = 0;
						}
					}

					bool hadRealUpdate = DoUpdateFrames();
					if (hadRealUpdate)
						mUpdateAppState = UPDATESTATE_PROCESS_2;

					mHasPendingDraw = true;
					didUpdate = true;
				}
			}
		}
		else if (mUpdateAppState == UPDATESTATE_PROCESS_2)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mPendingUpdatesAcc += anUpdatesPerUpdateF;
			mPendingUpdatesAcc -= 1.0;
			ProcessSafeDeleteList();

			// Process any extra updates
			while (mPendingUpdatesAcc >= 1.0)
			{

				++mNonDrawCount;
				bool hasRealUpdate = DoUpdateFrames();
				DBG_ASSERTE(hasRealUpdate);

				if (!hasRealUpdate)
					break;

				ProcessSafeDeleteList();
				mPendingUpdatesAcc -= 1.0;
			}

			// aNumCalls++;
			DoUpdateFramesF((float)anUpdatesPerUpdateF);
			ProcessSafeDeleteList();

			// Don't let mUpdateFTimeAcc dip below 0
			//  Subtract an extra 0.2ms, because sometimes refresh rates have some
			//  fractional component that gets truncated, and it's better to take off
			//  too much to keep our timing tending toward occuring right after
			//  redraws
			if (isVSynched)
				mUpdateFTimeAcc = std::max(mUpdateFTimeAcc - aFrameFTime - 0.2f, 0.0);
			else
				mUpdateFTimeAcc -= aFrameFTime;

			if (mRelaxUpdateBacklogCount > 0)
				mUpdateFTimeAcc = 0;

			didUpdate = true;
		}

		if (!didUpdate)
		{
			mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mNonDrawCount = 0;

			if (mHasPendingDraw)
			{
				DrawDirtyStuff();
			}
			else
			{
				// Let us take into account the time it took to draw dirty stuff
				int aTimeToNextFrame = (int)(aFrameFTime - mUpdateFTimeAcc);
				if (aTimeToNextFrame > 0)
				{
					if (!allowSleep)
						return false;

					// Wait till next processing cycle
					++mSleepCount;
					SDL_Delay(aTimeToNextFrame);

					aCumSleepTime += aTimeToNextFrame;
				}
			}
		}

		if (mYieldMainThread)
		{
			// This is to make sure that the title screen doesn't take up any more than
			// 1/3 of the processor time

			ulong anEndTime = SDL_GetTicks();
			int anElapsedTime = (anEndTime - aStartTime) - aCumSleepTime;
			int aLoadingYieldSleepTime = std::min(250, (anElapsedTime * 2) - aCumSleepTime);

			if (aLoadingYieldSleepTime >= 0)
			{
				if (!allowSleep)
					return false;

				SDL_Delay(aLoadingYieldSleepTime);
			}
		}
	}

	ProcessSafeDeleteList();
	return true;
}

/*void AppBase::DoMainLoop()
{
	Dialog* aDialog = nullptr;
	if (theModalDialogId != -1)
	{
		aDialog = GetDialog(theModalDialogId);
		DBG_ASSERTE(aDialog != nullptr);
		if (aDialog == nullptr)
			return;
	}

	while (!mShutdown)
	{
		MSG msg;
		while ((PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) && (!mShutdown))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		ProcessDemo();
		ProcessDeferredMessages();

		if ((aDialog != nullptr) && (aDialog->mResult != -1))
			return;

		if (!mShutdown)
		{
			//++aCount;
			Process();
		}
	}
}*/

void AppBase::DoMainLoop()
{
	while (!mShutdown)
	{
		if (mExitToTop)
			mExitToTop = false;
		UpdateApp();
	}
}

bool AppBase::UpdateAppStep(bool *updated)
{
	if (updated != nullptr)
		*updated = false;

	if (mExitToTop)
		return false;

	if (mUpdateAppState == UPDATESTATE_PROCESS_DONE)
		mUpdateAppState = UPDATESTATE_MESSAGES;

	mUpdateAppDepth++;

	// We update in two stages to avoid doing a Process if our loop termination
	//  condition has already been met by processing windows messages
	if (mUpdateAppState == UPDATESTATE_MESSAGES)
	{
		if (!ProcessDeferredMessages(true))
		{
			mUpdateAppState = UPDATESTATE_PROCESS_1;
		}
	}
	else
	{
		// Process changes state by itself
		if (mStepMode)
		{
			if (mStepMode == 2)
			{
				SDL_Delay(mFrameTime);
				mUpdateAppState = UPDATESTATE_PROCESS_DONE; // skip actual update until next step
			}
			else
			{
				mStepMode = 2;
				DoUpdateFrames();
				DoUpdateFramesF(1.0f);
				DrawDirtyStuff();
			}
		}
		else
		{
			int anOldUpdateCnt = mUpdateCount;
			Process();
			if (updated != nullptr)
				*updated = mUpdateCount != anOldUpdateCnt;
		}
	}

	mUpdateAppDepth--;

	return true;
}

bool AppBase::UpdateApp()
{
	bool updated;
	for (;;)
	{
		if (!UpdateAppStep(&updated))
			return false;
		if (updated)
			return true;
	}
}

int AppBase::InitSDLInterface()
{
	PreSDLInterfaceInitHook();
	DeleteNativeImageData();
	int aResult = mSDLInterface->Init(mIsWindowed);
	if (SDLInterface::RESULT_OK == aResult)
	{
		mScreenBounds.mX = (mWidth - mSDLInterface->mWidth) / 2;
		mScreenBounds.mY = (mHeight - mSDLInterface->mHeight) / 2;
		mScreenBounds.mWidth = mSDLInterface->mWidth;
		mScreenBounds.mHeight = mSDLInterface->mHeight;
		mSDLInterface->UpdateViewport();
		mWidgetManager->Resize(mScreenBounds, mSDLInterface->mPresentationRect);
		PostSDLInterfaceInitHook();

		if (mIGUIManager)
			delete mIGUIManager;
		mIGUIManager = new ImGuiManager(mSDLInterface);
		RegisterImGuiWindows();

	}
	return aResult;
}

void AppBase::PreTerminate()
{
}

void AppBase::Start()
{
	if (mShutdown)
		return;

	StartCursorThread();

	if (mAutoStartLoadingThread)
		StartLoadingThread();

	SDL_RaiseWindow(mSDLInterface->mWindow);

	int aCount = 0;
	int aSleepCount = 0;

	Uint64 aStartTime = SDL_GetTicks();

	mRunning = true;
	mLastTime = aStartTime;
	mLastUserInputTick = aStartTime;
	mLastTimerTime = aStartTime;

	DoMainLoop();
	ProcessSafeDeleteList();

	mRunning = false;

	WaitForLoadingThread();

	SDL_Log("Seconds       = %g\r\n", (SDL_GetTicks() - aStartTime) / 1000.0);
	// sprintf(aString, "Count         = %d\r\n", aCount);
	// OutputDebugString(aString);
	SDL_Log("Sleep Count   = %d\r\n", mSleepCount);
	SDL_Log("Update Count  = %d\r\n", mUpdateCount);
	SDL_Log("Draw Count    = %d\r\n", mDrawCount);
	SDL_Log("Screen Blt    = %d\r\n", mScreenBltTime);
	if (mDrawTime + mScreenBltTime > 0)
	{
		SDL_Log("Avg FPS       = %d\r\n", (mDrawCount * 1000) / (mDrawTime + mScreenBltTime));
	}

	PreTerminate();

	WriteToRegistry();
}

bool AppBase::CheckSignature(const Buffer &theBuffer, const std::string &theFileName)
{
	// Add your own signature checking code here
	return false;
}

bool AppBase::LoadProperties(const std::string &theFileName, bool required, bool checkSig)
{
	Buffer aBuffer;
	if (!ReadBufferFromFile(theFileName, &aBuffer))
	{
		if (!required)
			return true;
		else
		{
			Popup(GetString("UNABLE_OPEN_PROPERTIES", "Unable to open properties file ") + theFileName);
			return false;
		}
	}
	if (checkSig)
	{
		if (!CheckSignature(aBuffer, theFileName))
		{
			Popup(GetString("PROPERTIES_SIG_FAILED", "Signature check failed on ") + theFileName + "'");
			return false;
		}
	}

	PropertiesParser aPropertiesParser(this);

	// Load required language-file properties
	if (!aPropertiesParser.ParsePropertiesBuffer(aBuffer))
	{
		Popup(aPropertiesParser.GetErrorText());
		return false;
	}
	else
		return true;
}

bool AppBase::LoadProperties()
{
	// Load required language-file properties
	return LoadProperties("properties/default.xml", true, false);
}

void AppBase::LoadResourceManifest()
{
	if (!mResourceManager->ParseResourcesFile("properties/resources.xml"))
		ShowResourceError(true);
}

void AppBase::ShowResourceError(bool doExit)
{
	Popup(mResourceManager->GetErrorText());
	if (doExit)
		DoExit(0);
}

bool AppBase::GetBoolean(const std::string &theId)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);
	DBG_ASSERTE(anItr != mBoolProperties.end());

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return false;
}

bool AppBase::GetBoolean(const std::string &theId, bool theDefault)
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);

	if (anItr != mBoolProperties.end())
		return anItr->second;
	else
		return theDefault;
}

int AppBase::GetInteger(const std::string &theId)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);
	DBG_ASSERTE(anItr != mIntProperties.end());

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return false;
}

int AppBase::GetInteger(const std::string &theId, int theDefault)
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);

	if (anItr != mIntProperties.end())
		return anItr->second;
	else
		return theDefault;
}

double AppBase::GetDouble(const std::string &theId)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);
	DBG_ASSERTE(anItr != mDoubleProperties.end());

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return false;
}

double AppBase::GetDouble(const std::string &theId, double theDefault)
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);

	if (anItr != mDoubleProperties.end())
		return anItr->second;
	else
		return theDefault;
}

PopString AppBase::GetString(const std::string &theId)
{
	StringStringMap::iterator anItr = mStringProperties.find(theId);
	DBG_ASSERTE(anItr != mStringProperties.end());

	if (anItr != mStringProperties.end())
		return anItr->second;
	else
		return "";
}

PopString AppBase::GetString(const std::string &theId, const PopString &theDefault)
{
	StringStringMap::iterator anItr = mStringProperties.find(theId);

	if (anItr != mStringProperties.end())
		return anItr->second;
	else
		return theDefault;
}

StringVector AppBase::GetStringVector(const std::string &theId)
{
	StringStringVectorMap::iterator anItr = mStringVectorProperties.find(theId);
	DBG_ASSERTE(anItr != mStringVectorProperties.end());

	if (anItr != mStringVectorProperties.end())
		return anItr->second;
	else
		return StringVector();
}

void AppBase::SetString(const std::string &theId, const std::string &theValue)
{
	std::pair<StringStringMap::iterator, bool> aPair =
		mStringProperties.insert(StringStringMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void AppBase::SetBoolean(const std::string &theId, bool theValue)
{
	std::pair<StringBoolMap::iterator, bool> aPair = mBoolProperties.insert(StringBoolMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void AppBase::SetInteger(const std::string &theId, int theValue)
{
	std::pair<StringIntMap::iterator, bool> aPair = mIntProperties.insert(StringIntMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void AppBase::SetDouble(const std::string &theId, double theValue)
{
	std::pair<StringDoubleMap::iterator, bool> aPair =
		mDoubleProperties.insert(StringDoubleMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void AppBase::DoParseCmdLine()
{
	/*
	char *aCmdLine = GetCommandLineA();
	char *aCmdLinePtr = aCmdLine;
	if (aCmdLinePtr[0] == '"')
	{
		aCmdLinePtr = strchr(aCmdLinePtr + 1, '"');
		if (aCmdLinePtr != nullptr)
			aCmdLinePtr++;
	}

	if (aCmdLinePtr != nullptr)
	{
		aCmdLinePtr = strchr(aCmdLinePtr, ' ');
		if (aCmdLinePtr != nullptr)
			ParseCmdLine(aCmdLinePtr + 1);
	}
	*/

	mCmdLineParsed = true;
}

void AppBase::ParseCmdLine(const std::string &theCmdLine)
{
	// Command line example:  -play -demofile="game demo.dmo"
	// Results in HandleCmdLineParam("-play", ""); HandleCmdLineParam("-demofile", "game demo.dmo");
	std::string aCurParamName;
	std::string aCurParamValue;

	int aSpacePos = 0;
	bool inQuote = false;
	bool onValue = false;

	for (int i = 0; i < (int)theCmdLine.length(); i++)
	{
		char c = theCmdLine[i];
		bool atEnd = false;

		if (c == '"')
		{
			inQuote = !inQuote;

			if (!inQuote)
				atEnd = true;
		}
		else if ((c == ' ') && (!inQuote))
			atEnd = true;
		else if (c == '=')
			onValue = true;
		else if (onValue)
			aCurParamValue += c;
		else
			aCurParamName += c;

		if (i == theCmdLine.length() - 1)
			atEnd = true;

		if (atEnd && !aCurParamName.empty())
		{
			HandleCmdLineParam(aCurParamName, aCurParamValue);
			aCurParamName = "";
			aCurParamValue = "";
			onValue = false;
		}
	}
}

static int GetMaxDemoFileNum(const std::string &theDemoPrefix, int theMaxToKeep, bool doErase)
{
	typedef std::set<int> IntSet;
    IntSet aSet;

    std::string prefixDir = fs::path(theDemoPrefix).parent_path().string();
    std::string prefixBase = fs::path(theDemoPrefix).filename().string(); // just the prefix, e.g., "demo"

    if (prefixDir.empty()) prefixDir = "."; // use current dir if none

    std::regex pattern("^" + prefixBase + R"((\d+)\.dmo$)");

    for (const auto& entry : fs::directory_iterator(prefixDir)) {
        if (!entry.is_regular_file()) continue;

        std::smatch match;
        std::string filename = entry.path().filename().string();

        if (std::regex_match(filename, match, pattern)) {
            int num = std::stoi(match[1]);
            aSet.insert(num);
        }
    }

    if (!aSet.empty() && (int)aSet.size() > theMaxToKeep - 1 && doErase) {
        auto anItr = aSet.begin();
        std::string fileToDelete = theDemoPrefix + std::to_string(*anItr) + ".dmo";
        fs::remove(fileToDelete);
    }

    if (aSet.empty())
        return 0;

    return *aSet.rbegin(); // last (max) value
}

void AppBase::HandleCmdLineParam(const std::string &theParamName, const std::string &theParamValue)
{
	if (theParamName == "-crash")
	{
		// Try to access nullptr
		char *a = 0;
		*a = '!';
	}
	else if (theParamName == "-screensaver")
	{
		mIsScreenSaver = true;
	}
	else if (theParamName == "-changedir")
	{
		mChangeDirTo = theParamValue;
	}
	else
	{
		Popup(GetString("INVALID_COMMANDLINE_PARAM", "Invalid command line parameter: ") + theParamName);
		DoExit(0);
	}
}

void AppBase::PreDisplayHook()
{
}

void AppBase::PreSDLInterfaceInitHook()
{
}

void AppBase::PostSDLInterfaceInitHook()
{
}

bool AppBase::ChangeDirHook(const char *theIntendedPath)
{
	return false;
}

MusicInterface *AppBase::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;
	else
		return new BassMusicInterface();
}

void AppBase::InitPropertiesHook()
{
}

void AppBase::InitHook()
{
}

void AppBase::Init()
{
	mPrimaryThreadId = SDL_GetCurrentThreadID();
	mErrorHandler = new ErrorHandler(this);

	if (mShutdown)
		return;

	InitPropertiesHook();
	ReadFromRegistry();

	if (!mCmdLineParsed)
		DoParseCmdLine();

	if (IsScreenSaver())
		mOnlyAllowOneCopyToRun = false;

	// Change directory
	if (!ChangeDirHook(mChangeDirTo.c_str()))
		chdir(mChangeDirTo.c_str());

	gPakInterface->AddPakFile("main.gpak");

	// Create a globally unique mutex
	mMutex = new std::mutex();

	mRandSeed = SDL_GetTicks();
	SRand(mRandSeed);

	srand(SDL_GetTicks());

	// Let app do something before showing window, or switching to fullscreen mode
	// NOTE: Moved call to PreDisplayHook above mIsWindowed and GetSystemsMetrics
	// checks because the checks below use values that could change in PreDisplayHook.
	// PreDisplayHook must call mWidgetManager->Resize if it changes mWidth or mHeight.
	PreDisplayHook();

	mWidgetManager->Resize(Rect(0, 0, mWidth, mHeight), Rect(0, 0, mWidth, mHeight));

	// Check to see if we CAN run windowed or not...
	if (mIsWindowed && !mFullScreenWindow)
	{
		// How can we be windowed if our screen isn't even big enough?
		SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(displayID);
		int screenWidth = dm->w;
		int screenHeight = dm->h;

		if (mWidth >= screenWidth || mHeight >= screenHeight)
		{
			mIsWindowed = false;
			mForceFullscreen = true;
		}
	}

	MakeWindow();

	if (mSoundManager == nullptr)
		mSoundManager = new OpenALSoundManager();

	SetSfxVolume(mSfxVolume);

	mMusicInterface = CreateMusicInterface();

	SetMusicVolume(mMusicVolume);

	if (IsScreenSaver())
	{
		SetCursor(CURSOR_NONE);
	}

	InitHook();

	mInitialized = true;
}

void AppBase::HandleGameAlreadyRunning()
{
	if (mOnlyAllowOneCopyToRun)
	{
		DoExit(0);
	}
}

void AppBase::CopyToClipboard(const std::string &theString)
{
	SDL_SetClipboardText(theString.c_str());
}

std::string AppBase::GetClipboard()
{
	std::string aString;
	aString = SDL_GetClipboardText();
	return aString;
}

void AppBase::SetCursor(int theCursorNum)
{
	mCursorNum = theCursorNum;
	EnforceCursor();
}

int AppBase::GetCursor()
{
	return mCursorNum;
}

void AppBase::EnableCustomCursors(bool enabled)
{
	mCustomCursorsEnabled = enabled;
	EnforceCursor();
}

void AppBase::SetTaskBarIcon(const std::string &theFileName)
{
	// H521
	int width, height, channels;
	unsigned char *data = stbi_load(theFileName.c_str(), &width, &height, &channels, 4);
	if (!data)
	{
		SDL_Log("failed to load image: %s\n", stbi_failure_reason());
		return;
	}

	SDL_Surface *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width*4);
	if (!surface)
	{
		SDL_Log("no surface?\n");
		stbi_image_free(data);
		return;
	}

	SDL_SetWindowIcon(mSDLInterface->mWindow, surface);

	stbi_image_free(surface->pixels);
	SDL_DestroySurface(surface);

	return;
}

PopLib::SDLImage *AppBase::GetImage(const std::string &theFileName, bool commitBits)
{
	ImageLib::Image *aLoadedImage = ImageLib::GetImage(theFileName, true);

	if (aLoadedImage == nullptr)
		return nullptr;

	SDLImage *anImage = new SDLImage(mSDLInterface);
	anImage->mFilePath = theFileName;
	anImage->SetBits((uint32_t*)aLoadedImage->GetBits(), aLoadedImage->GetWidth(), aLoadedImage->GetHeight(), commitBits);
	delete aLoadedImage;

	return anImage;
}

PopLib::SDLImage *AppBase::CreateCrossfadeImage(PopLib::Image *theImage1, const Rect &theRect1,
												 PopLib::Image *theImage2, const Rect &theRect2, double theFadeFactor)
{
	MemoryImage *aMemoryImage1 = dynamic_cast<MemoryImage *>(theImage1);
	MemoryImage *aMemoryImage2 = dynamic_cast<MemoryImage *>(theImage2);

	if ((aMemoryImage1 == nullptr) || (aMemoryImage2 == nullptr))
		return nullptr;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) || (theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect1 out of bounds");
		return nullptr;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) || (theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect2 out of bounds");
		return nullptr;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	SDLImage *anImage = new SDLImage(mSDLInterface);
	anImage->Create(aWidth, aHeight);

	ulong *aDestBits = anImage->GetBits();
	ulong *aSrcBits1 = aMemoryImage1->GetBits();
	ulong *aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	ulong aMult = (int)(theFadeFactor * 256);
	ulong aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		ulong *s1 = &aSrcBits1[(y + theRect1.mY) * aSrc1Width + theRect1.mX];
		ulong *s2 = &aSrcBits2[(y + theRect2.mY) * aSrc2Width + theRect2.mX];
		ulong *d = &aDestBits[y * aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			ulong p1 = *s1++;
			ulong p2 = *s2++;

			// p1 = 0;
			// p2 = 0xFFFFFFFF;

			*d++ = ((((p1 & 0x000000FF) * aOMM + (p2 & 0x000000FF) * aMult) >> 8) & 0x000000FF) |
				   ((((p1 & 0x0000FF00) * aOMM + (p2 & 0x0000FF00) * aMult) >> 8) & 0x0000FF00) |
				   ((((p1 & 0x00FF0000) * aOMM + (p2 & 0x00FF0000) * aMult) >> 8) & 0x00FF0000) |
				   ((((p1 >> 24) * aOMM + (p2 >> 24) * aMult) << 16) & 0xFF000000);
		}
	}

	anImage->BitsChanged();

	return anImage;
}

void AppBase::ColorizeImage(Image *theImage, const Color &theColor)
{
	MemoryImage *aSrcMemoryImage = dynamic_cast<MemoryImage *>(theImage);

	if (aSrcMemoryImage == nullptr)
		return;

	ulong *aBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == nullptr)
	{
		aBits = aSrcMemoryImage->GetBits();
		aNumColors = theImage->GetWidth() * theImage->GetHeight();
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;
		aNumColors = 256;
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && (theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			aBits[i] = ((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
					   ((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
					   ((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00) |
					   ((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	aSrcMemoryImage->BitsChanged();
}

SDLImage *AppBase::CreateColorizedImage(Image *theImage, const Color &theColor)
{
	MemoryImage *aSrcMemoryImage = dynamic_cast<MemoryImage *>(theImage);

	if (aSrcMemoryImage == nullptr)
		return nullptr;

	SDLImage *anImage = new SDLImage(mSDLInterface);

	anImage->Create(theImage->GetWidth(), theImage->GetHeight());

	ulong *aSrcBits;
	ulong *aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == nullptr)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth() * theImage->GetHeight();
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new ulong[256];
		aNumColors = 256;

		anImage->mColorIndices = new uchar[anImage->mWidth * theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth * theImage->mHeight);
	}

	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && (theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			aDestBits[i] = ((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
						   ((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
						   ((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00) |
						   ((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}

	anImage->BitsChanged();

	return anImage;
}

SDLImage *AppBase::CopyImage(Image *theImage, const Rect &theRect)
{
	SDLImage *anImage = new SDLImage(mSDLInterface);

	anImage->Create(theRect.mWidth, theRect.mHeight);

	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

SDLImage *AppBase::CopyImage(Image *theImage)
{
	return CopyImage(theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

void AppBase::MirrorImage(Image *theImage)
{
	MemoryImage *aSrcMemoryImage = dynamic_cast<MemoryImage *>(theImage);

	ulong *aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		ulong *aLeftBits = aSrcBits + (y * aPhysSrcWidth);
		ulong *aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			ulong aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void AppBase::FlipImage(Image *theImage)
{
	MemoryImage *aSrcMemoryImage = dynamic_cast<MemoryImage *>(theImage);

	ulong *aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		ulong *aTopBits = aSrcBits + x;
		ulong *aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			ulong aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();
}

void AppBase::RotateImageHue(PopLib::MemoryImage *theImage, int theDelta)
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	uint32_t* aPtr = reinterpret_cast<uint32_t*>(theImage->GetBits());
	for (int i = 0; i < aSize; i++)
	{
		uint32_t aPixel = *aPtr;
		int alpha = aPixel & 0xff000000;
		int r = (aPixel >> 16) & 0xff;
		int g = (aPixel >> 8) & 0xff;
		int b = aPixel & 0xff;

		int maxval = std::max(r, std::max(g, b));
		int minval = std::min(r, std::min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval + maxval) / 2;
		int delta = maxval - minval;

		if (delta != 0)
		{
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));

			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval - b) * 256) / delta) : 256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ? 256 + (((maxval - r) * 256) / delta) : 768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ? 768 + (((maxval - g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v = (l < 128) ? (l * (255 + s)) / 255 : (l + s - l * s / 255);

		int y = (int)(2 * l - v);

		int aColorDiv = (6 * h) / 256;
		int x = (int)(y + (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
		if (x > 255)
			x = 255;

		int z = (int)(v - (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
		if (z < 0)
			z = 0;

		switch (aColorDiv)
		{
		case 0:
			r = (int)v;
			g = x;
			b = y;
			break;
		case 1:
			r = z;
			g = (int)v;
			b = y;
			break;
		case 2:
			r = y;
			g = (int)v;
			b = x;
			break;
		case 3:
			r = y;
			g = z;
			b = (int)v;
			break;
		case 4:
			r = x;
			g = y;
			b = (int)v;
			break;
		case 5:
			r = (int)v;
			g = y;
			b = z;
			break;
		default:
			r = (int)v;
			g = x;
			b = y;
			break;
		}

		*aPtr++ = alpha | (r << 16) | (g << 8) | (b);
	}

	theImage->BitsChanged();
}

ulong AppBase::HSLToRGB(int h, int s, int l)
{
	int r;
	int g;
	int b;

	double v = (l < 128) ? (l * (255 + s)) / 255 : (l + s - l * s / 255);

	int y = (int)(2 * l - v);

	int aColorDiv = (6 * h) / 256;
	int x = (int)(y + (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
	if (x > 255)
		x = 255;

	int z = (int)(v - (v - y) * ((h - (aColorDiv * 256 / 6)) * 6) / 255);
	if (z < 0)
		z = 0;

	switch (aColorDiv)
	{
	case 0:
		r = (int)v;
		g = x;
		b = y;
		break;
	case 1:
		r = z;
		g = (int)v;
		b = y;
		break;
	case 2:
		r = y;
		g = (int)v;
		b = x;
		break;
	case 3:
		r = y;
		g = z;
		b = (int)v;
		break;
	case 4:
		r = x;
		g = y;
		b = (int)v;
		break;
	case 5:
		r = (int)v;
		g = y;
		b = z;
		break;
	default:
		r = (int)v;
		g = x;
		b = y;
		break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

ulong AppBase::RGBToHSL(int r, int g, int b)
{
	int maxval = std::max(r, std::max(g, b));
	int minval = std::min(r, std::min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval + maxval) / 2;
	int delta = maxval - minval;

	if (delta != 0)
	{
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));

		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval - b) * 256) / delta) : 256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ? 256 + (((maxval - r) * 256) / delta) : 768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ? 768 + (((maxval - g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));

		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);
}

void AppBase::HSLToRGB(const ulong *theSource, ulong *theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void AppBase::RGBToHSL(const ulong *theSource, ulong *theDest, int theSize)
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] =
			(src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}

void AppBase::PrecacheAdditive(MemoryImage *theImage)
{
	theImage->GetRLAdditiveData(mSDLInterface);
}

void AppBase::PrecacheAlpha(MemoryImage *theImage)
{
	theImage->GetRLAlphaData();
}

void AppBase::PrecacheNative(MemoryImage *theImage)
{
	theImage->GetNativeAlphaData(mSDLInterface);
}

void AppBase::PlaySample(int theSoundNum)
{
	if (!mSoundManager)
		return;

	SoundInstance *aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != nullptr)
	{
		aSoundInstance->Play(false, true);
	}
}

void AppBase::PlaySample(int theSoundNum, int thePan)
{
	if (!mSoundManager)
		return;

	SoundInstance *aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != nullptr)
	{
		aSoundInstance->SetPan(thePan);
		aSoundInstance->Play(false, true);
	}
}

bool AppBase::IsMuted()
{
	return mMuteCount > 0;
}

void AppBase::Mute(bool autoMute)
{
	mMuteCount++;
	if (autoMute)
		mAutoMuteCount++;

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

void AppBase::Unmute(bool autoMute)
{
	if (mMuteCount > 0)
	{
		mMuteCount--;
		if (autoMute)
			mAutoMuteCount--;
	}

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

double AppBase::GetMusicVolume()
{
	return mMusicVolume;
}

void AppBase::SetMusicVolume(double theVolume)
{
	mMusicVolume = theVolume;

	if (mMusicInterface != nullptr)
		mMusicInterface->SetVolume((mMuteCount > 0) ? 0.0 : mMusicVolume * 10);
}

double AppBase::GetSfxVolume()
{
	return mSfxVolume;
}

void AppBase::SetSfxVolume(double theVolume)
{
	mSfxVolume = theVolume;

	if (mSoundManager != nullptr)
		mSoundManager->SetVolume((mMuteCount > 0) ? 0.0 : mSfxVolume);
}

double AppBase::GetMasterVolume()
{
	return mSoundManager->GetMasterVolume();
}

void AppBase::SetMasterVolume(double theMasterVolume)
{
	mSfxVolume = theMasterVolume;
	mSoundManager->SetMasterVolume(mSfxVolume);
}

void AppBase::AddMemoryImage(MemoryImage *theMemoryImage)
{
	AutoCrit anAutoCrit(mSDLInterface->mCritSect);
	mMemoryImageSet.insert(theMemoryImage);
}

void AppBase::RemoveMemoryImage(MemoryImage *theMemoryImage)
{
	// AutoCrit anAutoCrit(mSDLInterface->mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.find(theMemoryImage);
	if (anItr != mMemoryImageSet.end())
		mMemoryImageSet.erase(anItr);

	Remove3DData(theMemoryImage);
}

void AppBase::Remove3DData(MemoryImage *theMemoryImage)
{
	if (mSDLInterface)
		mSDLInterface->Remove3DData(theMemoryImage);
}

bool AppBase::Is3DAccelerated()
{
	return mSDLInterface->mIs3D;
}

bool AppBase::Is3DAccelerationSupported()
{
	// if (mSDLInterface->mD3DTester)
	//	return mDDInterface->mD3DTester->Is3DSupported();
	// else
	return true;
}

bool AppBase::Is3DAccelerationRecommended()
{
	// if (mSDLInterface->mD3DTester)
	//	return mDDInterface->mD3DTester->Is3DRecommended();
	// else
	return true;
}

void AppBase::Set3DAcclerated(bool is3D, bool reinit)
{
	if (mSDLInterface->mIs3D == is3D)
		return;

	mUserChanged3DSetting = true;
	mSDLInterface->mIs3D = is3D;

	if (reinit)
	{
		int aResult = InitSDLInterface();

		if (is3D && aResult == SDLInterface::RESULT_3D_FAIL)
		{
			Set3DAcclerated(false, reinit);
			return;
		}

		if (aResult != SDLInterface::RESULT_OK)
		{
			Popup(GetString("FAILED_INIT_DIRECTDRAW", "Failed to initialize DirectDraw: ") + SDL_GetError());
			DoExit(1);
		}

		ReInitImages();

		mWidgetManager->mImage = mSDLInterface->GetScreenImage();
		mWidgetManager->MarkAllDirty();
	}
}

SharedImageRef AppBase::GetSharedImage(const std::string &theFileName, const std::string &theVariant, bool *isNew)
{
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mSDLInterface->mCritSect);
		aResultPair = mSharedImageMap.insert(
			SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
	}

	if (isNew != nullptr)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		// Pass in a '!' as the first char of the file name to create a new image
		if ((theFileName.length() > 0) && (theFileName[0] == '!'))
			aSharedImageRef.mSharedImage->mImage = new SDLImage(mSDLInterface);
		else
			aSharedImageRef.mSharedImage->mImage = GetImage(theFileName, false);
	}

	return aSharedImageRef;
}

void AppBase::CleanSharedImages()
{
	AutoCrit anAutoCrit(mSDLInterface->mCritSect);

	if (mCleanupSharedImages)
	{
		// Delete shared images with reference counts of 0
		// This doesn't occur in ~SharedImageRef because sometimes we can not only access the image
		//  through the SharedImageRef returned by GetSharedImage, but also by calling GetSharedImage
		//  again with the same params -- so we can have instances where we do the 'final' deref on
		//  an image but immediately re-request it via GetSharedImage
		SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
		while (aSharedImageItr != mSharedImageMap.end())
		{
			SharedImage *aSharedImage = &aSharedImageItr->second;
			if (aSharedImage->mRefCount == 0)
			{
				delete aSharedImage->mImage;
				mSharedImageMap.erase(aSharedImageItr++);
			}
			else
				++aSharedImageItr;
		}

		mCleanupSharedImages = false;
	}
}
