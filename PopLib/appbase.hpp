#ifndef __APPBASE_HPP__
#define __APPBASE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "math/rect.hpp"
#include "graphics/color.hpp"
#include "widget/buttonlistener.hpp"
#include "widget/dialoglistener.hpp"
#include "misc/buffer.hpp"
#include "misc/critsect.hpp"
#include "graphics/sharedimage.hpp"
#include "math/ratio.hpp"
#include <mutex>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// H522

/**
 * @brief registry types, but json
 */
enum JSON_RTYPE
{
	JSON_NONE = 0,
	JSON_STRING,
	JSON_INTEGER,
	JSON_BOOLEAN,
	JSON_DATA,
	JSON_LAST
};

/**
 * @brief the interface types
 */
enum InterfaceType
{
	INTERFACE_NONE = 0,
	INTERFACE_SDL,
	INTERFACE_NUM
};

namespace ImageLib
{
class Image;
};

namespace PopLib
{

class WidgetManager;
class Interface;
class Image;
class Widget;
class SoundManager;
class MusicInterface;
class GPUImage;
class HTTPTransfer;
class ImGuiManager;
class Dialog;

class ResourceManager;

class WidgetSafeDeleteInfo
{
  public:
	int mUpdateAppDepth;
	Widget *mWidget;
};

typedef std::list<WidgetSafeDeleteInfo> WidgetSafeDeleteList;
typedef std::set<GPUImage *> GPUImageSet;
typedef std::map<int, Dialog *> DialogMap;
typedef std::list<Dialog *> DialogList;
typedef std::vector<std::string> StringVector;
// typedef std::basic_string<TCHAR> tstring; // string of TCHARs

typedef std::map<std::string, PopString> StringPopStringMap;
typedef std::map<std::string, std::string> StringStringMap;
typedef std::map<std::string, bool> StringBoolMap;
typedef std::map<std::string, int> StringIntMap;
typedef std::map<std::string, double> StringDoubleMap;
typedef std::map<std::string, StringVector> StringStringVectorMap;

/**
 * @brief cursor types
 */
enum CursorType
{
	CURSOR_NONE = 0,
	CURSOR_POINTER,
	CURSOR_HAND,
	CURSOR_DRAGGING,
	CURSOR_TEXT,
	CURSOR_CIRCLE_SLASH,
	CURSOR_SIZEALL,
	CURSOR_SIZENESW,
	CURSOR_SIZENS,
	CURSOR_SIZENWSE,
	CURSOR_SIZEWE,
	CURSOR_WAIT,
	CURSOR_CUSTOM,
	NUM_CURSORS
};

/**
 * @brief show fps types
 */
enum
{
	FPS_ShowFPS,
	FPS_ShowCoords,
	Num_FPS_Types
};

/**
 * @brief update states
 */
enum
{
	UPDATESTATE_MESSAGES,
	UPDATESTATE_PROCESS_1,
	UPDATESTATE_PROCESS_2,
	UPDATESTATE_PROCESS_DONE
};

/**
 * @brief message boxes flags
 */
enum MsgBoxFlags
{
	MsgBox_OK = 0,
	MsgBox_OKCANCEL = 1,
	MsgBox_ABORTRETRYIGNORE = 2,
	MsgBox_YESNOCANCEL = 3,
	MsgBox_YESNO = 4,
	MsgBox_RETRYCANCEL = 5,
};

/**
 * @brief handles the game window
 *
 * the AppBase class is basically the root of all games, demos and stuff.
 * uses Interface for window handling, and everything else window-related
 */
class AppBase : public ButtonListener, public DialogListener
{
  public:
	/// @brief the seed for MRand class
	ulong mRandSeed;

	/// @brief defines the company name, also used in save files
	std::string mCompanyName;
	/// @brief unused (probably)
	std::string mFullCompanyName;
	/// @brief product (game) name
	std::string mProdName;
	/// @brief the title for the window
	PopString mTitle;
	/// @brief TBA
	std::string mRegKey;
	/// @brief TBA
	std::string mChangeDirTo;

	/// @brief TBA
	int mRelaxUpdateBacklogCount; // app doesn't try to catch up for this many frames
	/// @brief preferred X position of window
	int mPreferredX;
	/// @brief preferred Y position of window
	int mPreferredY;
	/// @brief window width
	int mWidth;
	/// @brief window height
	int mHeight;
	/// @brief TBA
	int mFullscreenBits;
	/// @brief saved music volume
	double mMusicVolume;
	/// @brief saved sfx volume
	double mSfxVolume;
	/// @brief returns true if no sound is needed
	bool mNoSoundNeeded;
	/// @brief returns true if arguments given when running the game
	bool mCmdLineParsed;
	/// @brief skips signature checks
	bool mSkipSignatureChecks;
	/// @brief TBA
	bool mStandardWordWrap;
	/// @brief TBA
	bool mbAllowExtendedChars;

	/// @brief imgui manager object, IGUI = ImGui
	ImGuiManager *mIGUIManager;

	/// @brief TBA
	std::mutex *mMutex;
	/// @brief true if allowing only one game to run at the same time
	bool mOnlyAllowOneCopyToRun;
	/// @brief critsect for the whole class
	CritSect mCritSect;
	/// @brief TBA
	bool mBetaValidate;
	/// @brief TBA
	uchar mAdd8BitMaxTable[512];
	/// @brief the widget manager, allows for widgets to exist
	WidgetManager *mWidgetManager;
	/// @brief class dialog map
	DialogMap mDialogMap;
	/// @brief class dialog list
	DialogList mDialogList;
	/// @brief threading
	SDL_ThreadID mPrimaryThreadId;
	/// @brief crash handler, true if crashed
	bool mSEHOccured;
	/// @brief shutdowns
	bool mShutdown;
	/// @brief TBA
	bool mExitToTop;
	/// @brief true if windowed
	bool mIsWindowed;
	/// @brief true if windowed (can be windowed)
	bool mIsPhysWindowed;
	/// @brief true if fullscreen, uses ChangeDisplaySettings to run fullscreen with mIsWindowed true
	bool mFullScreenWindow;
	/// @brief forces fullscreen
	bool mForceFullscreen;
	/// @brief forces windowed
	bool mForceWindowed;
	/// @brief true if initialized
	bool mInitialized;
	/// @brief TBA
	bool mProcessInTimer;
	/// @brief TBA
	uint32_t mTimeLoaded;
	/// @brief mostly unused, true if is screensaver
	bool mIsScreenSaver;
	/// @brief true if allowing monitor powersaving
	bool mAllowMonitorPowersave;
	/// @brief TBA
	bool mNoDefer;
	/// @brief TBA
	bool mFullScreenPageFlip;
	/// @brief true if tablet pc
	bool mTabletPC;
	/// @brief the window interface
	Interface *mInterface;
	/// @brief TBA
	bool mAlphaDisabled;
	/// @brief (MusicInterface) the music interface, uses BASS
	MusicInterface *mMusicInterface;
	/// @brief TBA
	bool mReadFromRegistry;
	/// @brief TBA
	std::string mRegisterLink;
	/// @brief the game(product) version
	std::string mProductVersion;
	/// @brief cursor images, max.: 13
	Image *mCursorImages[NUM_CURSORS];
	/// @brief titlebar icon, can use .rc for that if on windows
	Image *mTitleBarIcon;
	/// @brief if set, overrides the cursor
	SDL_Cursor *mOverrideCursor;
	/// @brief true if opening URL
	bool mIsOpeningURL;
	/// @brief true if the app will shut down on URL open
	bool mShutdownOnURLOpen;
	/// @brief what url is the app opening
	std::string mOpeningURL;
	/// @brief the time when URL was opened
	uint32_t mOpeningURLTime;
	/// @brief TBA
	uint32_t mLastTimerTime;
	/// @brief TBA
	uint32_t mLastBigDelayTime;
	/// @brief saved music volume
	double mUnmutedMusicVolume;
	/// @brief saved sfx volume
	double mUnmutedSfxVolume;
	/// @brief TBA
	int mMuteCount;
	/// @brief TBA
	int mAutoMuteCount;
	/// @brief TBA
	bool mMuteOnLostFocus;
	/// @brief TBA
	GPUImageSet mGPUImageSet;
	/// @brief TBA
	SharedImageMap mSharedImageMap;
	/// @brief TBA
	bool mCleanupSharedImages;

	/// @brief TBA
	int mNonDrawCount;
	/// @brief current frame time
	int mFrameTime;

	/// @brief true if drawing
	bool mIsDrawing;
	/// @brief true if last draw was empty
	bool mLastDrawWasEmpty;
	/// @brief true if the app has a pending draw
	bool mHasPendingDraw;
	/// @brief TBA
	double mPendingUpdatesAcc;
	/// @brief TBA
	double mUpdateFTimeAcc;
	/// @brief TBA
	uint64_t mLastTimeCheck;
	/// @brief last time
	uint64_t mLastTime;
	/// @brief last user input tick
	uint64_t mLastUserInputTick;

	/// @brief (total?) sleep count
	int mSleepCount;
	/// @brief (total?) draw count
	int mDrawCount;
	/// @brief (total?) update count
	int mUpdateCount;
	/// @brief current update app state
	int mUpdateAppState;
	/// @brief current update app depth
	int mUpdateAppDepth;
	/// @brief update multiplier (2,3,etc.)
	double mUpdateMultiplier;
	/// @brief true if paused
	bool mPaused;
	/// @brief TBA
	int mFastForwardToUpdateNum;
	/// @brief TBA
	bool mFastForwardToMarker;
	/// @brief TBA
	bool mFastForwardStep;
	/// @brief last drawing tick
	uint64_t mLastDrawTick;
	/// @brief next drawing tick
	uint64_t mNextDrawTick;
	/// @brief current step mode. 0 = off, 1 = step, 2 = waiting for step
	int mStepMode;

	/// @brief the sdl window
	SDL_Window *mWindow;
	/// @brief the interface type
	InterfaceType mInterfaceType;

	/// @brief cursor number
	int mCursorNum;
	/// @brief (SoundManager) app sound manager
	SoundManager *mSoundManager;
	/// @brief current hand cursor
	SDL_Cursor *mHandCursor;
	/// @brief current dragging cursor
	SDL_Cursor *mDraggingCursor;
	/// @brief list of widgets to be safely deleted
	WidgetSafeDeleteList mSafeDeleteList;
	/// @brief TBA
	bool mMouseIn;
	/// @brief true if app running
	bool mRunning;
	/// @brief true if app active
	bool mActive;
	/// @brief true if app minimized
	bool mMinimized;
	/// @brief true if app minimized (can be)
	bool mPhysMinimized;
	/// @brief true if app disabled
	bool mIsDisabled;
	/// @brief true if app has focus on
	bool mHasFocus;
	/// @brief draw time as an integer
	int mDrawTime;
	/// @brief TBA
	uint64_t mFPSStartTick;
	/// @brief TBA
	int mFPSFlipCount;
	/// @brief TBA
	int mFPSDirtyCount;
	/// @brief TBA
	int mFPSTime;
	/// @brief total frames count
	int mFPSCount;
	/// @brief true if should show current FPS
	bool mShowFPS;
	/// @brief the fps showing mode (if mShowFPS is true)
	int mShowFPSMode;
	/// @brief screen blit time
	int mScreenBltTime;
	/// @brief true if loading thread started
	bool mAutoStartLoadingThread;
	/// @brief true if loading thread started
	bool mLoadingThreadStarted;
	/// @brief true if loading thread completed
	bool mLoadingThreadCompleted;
	/// @brief true if app loaded
	bool mLoaded;
	/// @brief true if main thread yielded
	bool mYieldMainThread;
	/// @brief true if loading failed
	bool mLoadingFailed;
	/// @brief true if cursor thread running
	bool mCursorThreadRunning;
	/// @brief true if has system cursor
	bool mSysCursor;
	/// @brief true if custom cursors are enabled
	bool mCustomCursorsEnabled;
	/// @brief true if custom cursors are dirty
	bool mCustomCursorDirty;
	/// @brief true if last shutdown was 'graceful'
	bool mLastShutdownWasGraceful;
	/// @brief true if widescreen window
	bool mIsWideWindow;

	/// @brief the number of loading thread tasks
	int mNumLoadingThreadTasks;
	/// @brief the number of completed loading thread tasks
	int mCompletedLoadingThreadTasks;

	/// @brief true if debug keys are enabled
	bool mDebugKeysEnabled;
	/// @brief true if the maximize button is enabled
	bool mEnableMaximizeButton;
	/// @brief true if the ctrl key is down
	bool mCtrlDown;
	/// @brief true if the alt key is down
	bool mAltDown;

	/// @brief TBA
	int mSyncRefreshRate;
	/// @brief TBA
	bool mVSyncUpdates;
	/// @brief true if vsync is broken
	bool mVSyncBroken;
	/// @brief the number of broken vsync's
	int mVSyncBrokenCount;
	/// @brief TBA
	uint32_t mVSyncBrokenTestStartTick;
	/// @brief TBA
	uint32_t mVSyncBrokenTestUpdates;
	/// @brief true if waiting for vsync
	bool mWaitForVSync;
	/// @brief true if soft vsync wait (required mWaitForVSync)
	bool mSoftVSyncWait;
	/// @brief true if user changed the 3D mode setting
	bool mUserChanged3DSetting;
	/// @brief true if automaticly should enable 3D mode
	bool mAutoEnable3D;
	/// @brief TBA
	bool mTest3D;
	/// @brief the minimum amount of video memory for 3D mode
	uint32_t mMinVidMemory3D;
	/// @brief same as mMinVidMemory3D but recommended
	uint32_t mRecommendedVidMemory3D;

	/// @brief true if app is aware of widescreen
	bool mWidescreenAware;
	/// @brief screen bounds as rect
	Rect mScreenBounds;
	/// @brief true if enabled window aspect
	bool mEnableWindowAspect;
	/// @brief window aspect as ratio
	Ratio mWindowAspect;

	/// @brief TBA
	StringStringMap mStringProperties;
	/// @brief TBA
	StringBoolMap mBoolProperties;
	/// @brief TBA
	StringIntMap mIntProperties;
	/// @brief TBA
	StringDoubleMap mDoubleProperties;
	/// @brief TBA
	StringStringVectorMap mStringVectorProperties;
	/// @brief (ResourceManager) the app resource manager
	ResourceManager *mResourceManager;

  protected:
	/// @brief TBA
	void RehupFocus();
	/// @brief TBA
	void ClearKeysDown();
	/// @brief processes deferred messages, unused
	/// @param singleMessage 
	/// @return true if success
	bool ProcessDeferredMessages(bool singleMessage);
	/// @brief TBA
	void UpdateFTimeAcc();
	/// @brief process
	/// @param allowSleep 
	/// @return true if success
	virtual bool Process(bool allowSleep = true);
	/// @brief updates frames
	virtual void UpdateFrames();
	/// @brief calls UpdateFrames
	/// @return true if success
	virtual bool DoUpdateFrames();
	/// @brief TBA
	/// @param theFrac 
	virtual void DoUpdateFramesF(float theFrac);
	/// @brief creates the window
	virtual void MakeWindow();
	/// @brief enforces the cursor
	virtual void EnforceCursor();
	/// @brief reinitialize images
	virtual void ReInitImages();
	/// @brief deletes native images data
	virtual void DeleteNativeImageData();
	/// @brief deletes extra images data
	virtual void DeleteExtraImageData();

	// Loading thread methods

	/// @brief TBA
	virtual void LoadingThreadCompleted();
	/// @brief stub for loading thread
	static int LoadingThreadProcStub(void *theArg);

	// Cursor thread methods

	/// @brief TBA
	void CursorThreadProc();
	/// @brief stub for cursor thread
	static int CursorThreadProcStub(void *theArg);
	/// @brief TBA
	void StartCursorThread();

	/// @brief TBA
	void WaitForLoadingThread();
	/// @brief TBA
	void ProcessSafeDeleteList();
	/// @brief restores screen resolution
	void RestoreScreenResolution();
	/// @brief exits with a code
	/// @param theCode 
	void DoExit(int theCode);

	/// @brief takes an in-game screenshot
	void TakeScreenshot();
	/// @brief dumps the game program info
	void DumpProgramInfo();
	/// @brief shows the current memory usage
	void ShowMemoryUsage();

	// Registry helpers

	/// @brief reads a saved setting from .json
	/// @param theValueName 
	/// @param theType 
	/// @param theValue 
	/// @param theLength 
	/// @return true if success
	bool RegistryRead(const std::string &theValueName, JSON_RTYPE *theType, uchar *theValue, ulong *theLength);
	/// @brief reads a saved setting from .json
	/// @param theValueName 
	/// @param theType 
	/// @param theValue 
	/// @param theLength 
	/// @param theMainKey 
	/// @return true if success
	bool RegistryReadKey(const std::string &theValueName, JSON_RTYPE *theType, uchar *theValue, ulong *theLength,
						 ulong theMainKey = 0);
	/// @brief writes a setting to .json
	/// @param theValueName 
	/// @param theType 
	/// @param theValue 
	/// @param theLength 
	/// @return true if success
	bool RegistryWrite(const std::string &theValueName, JSON_RTYPE theType, const uchar *theValue, ulong theLength);

  public:
	/// @brief constructor
	AppBase();
	/// @brief destructor
	virtual ~AppBase();

	// Common overrides:
	
	/// @brief creates a MusicInterface
	/// @return the created MusicInterface
	virtual MusicInterface *CreateMusicInterface();
	/// @brief init hook, for games
	virtual void InitHook();
	/// @brief shutdown hook, for games
	virtual void ShutdownHook();
	/// @brief pre ~task~ terminate
	virtual void PreTerminate();
	/// @brief loading thread process
	virtual void LoadingThreadProc();
	/// @brief writes saved settings to json
	virtual void WriteToRegistry();
	/// @brief reads saved settings from json
	virtual void ReadFromRegistry();
	/// @brief creates a new dialog
	/// @param theDialogId 
	/// @param isModal 
	/// @param theDialogHeader 
	/// @param theDialogLines 
	/// @param theDialogFooter 
	/// @param theButtonMode 
	/// @return the created Dialog
	virtual Dialog *NewDialog(int theDialogId, bool isModal, const PopString &theDialogHeader,
							  const PopString &theDialogLines, const PopString &theDialogFooter,
							  int theButtonMode);
	/// @brief pre display hook
	virtual void PreDisplayHook();

	// Public methods

	/// @brief begins a ~widget~ popup
	virtual void BeginPopup();
	/// @brief ends a ~widget~ popup
	virtual void EndPopup();
	/// @brief message box
	/// @param theText 
	/// @param theTitle 
	/// @param theFlags 
	/// @return button id??
	virtual int MsgBox(const std::string &theText, const std::string &theTitle = "Message", int theFlags = 0);
	/// @brief message box (wide string)
	/// @param theText
	/// @param theTitle
	/// @param theFlags
	/// @return button id??
	virtual void Popup(const std::string &theString);
	/// @brief popup (wide string)
	/// @param theString 
	virtual void LogScreenSaverError(const std::string &theError);
	/// @brief safely deletes a widget
	/// @param theWidget 
	virtual void SafeDeleteWidget(Widget *theWidget);

	/// @brief if url open failed
	/// @param theURL 
	virtual void URLOpenFailed(const std::string &theURL);
	/// @brief if url open succeeded
	/// @param theURL 
	virtual void URLOpenSucceeded(const std::string &theURL);
	/// @brief opens a URL
	/// @param theURL 
	/// @param shutdownOnOpen 
	/// @return true if success
	virtual bool OpenURL(const std::string &theURL, bool shutdownOnOpen = false);
	/// @brief gets the current product version
	/// @param thePath 
	/// @return the product version
	virtual std::string GetProductVersion(const std::string &thePath);

	/// @brief TBA
	virtual void SEHOccured();
	/// @brief TBA
	virtual std::string GetGameSEHInfo();
	/// @brief TBA
	virtual void GetSEHWebParams(DefinesMap *theDefinesMap);
	/// @brief shutdowns the app
	virtual void Shutdown();

	/// @brief do parse the command line arguments
	virtual void DoParseCmdLine();
	/// @brief parses an command line argument
	/// @param theCmdLine 
	virtual void ParseCmdLine(const std::string &theCmdLine);
	/// @brief TBA
	/// @param theParamName 
	/// @param theParamValue 
	virtual void HandleCmdLineParam(const std::string &theParamName, const std::string &theParamValue);
	/// @brief handles notify game messages
	/// @param theType 
	/// @param theParam 
	/// 
	/// for HWND_BROADCAST of mNotifyGameMessage (0-1000 are reserved for AppBase for theType)
	virtual void HandleNotifyGameMessage(
		int theType,
		int theParam);
	/// @brief handles the game already running
	virtual void HandleGameAlreadyRunning();

	/// @brief starts the app
	virtual void Start();
	/// @brief initializes the app
	virtual void Init();
	/// @brief TBA
	virtual void PreInterfaceInitHook();
	/// @brief TBA
	virtual void PostInterfaceInitHook();
	/// @brief change directory hook
	/// @param theIntendedPath 
	/// @return true if success
	virtual bool ChangeDirHook(const char *theIntendedPath);
	/// @brief plays a sample
	/// @param theSoundNum 
	virtual void PlaySample(int theSoundNum);
	/// @brief plays a sample
	/// @param theSoundNum 
	/// @param thePan 
	virtual void PlaySample(int theSoundNum, int thePan);

	/// @brief gets the master volume
	/// @return master volume 
	virtual double GetMasterVolume();
	/// @brief gets the music volume
	/// @return music volume 
	virtual double GetMusicVolume();
	/// @brief gets the sfx volume
	/// @return sfx volume 
	virtual double GetSfxVolume();
	/// @brief are we muted?
	/// @return true if muted 
	virtual bool IsMuted();

	/// @brief sets the master volume
	/// @param theVolume 
	virtual void SetMasterVolume(double theVolume);
	/// @brief sets the music volume
	/// @param theVolume 
	virtual void SetMusicVolume(double theVolume);
	/// @brief sets the sfx volume
	/// @param theVolume 
	virtual void SetSfxVolume(double theVolume);
	/// @brief mutes the app
	/// @param autoMute 
	virtual void Mute(bool autoMute = false);
	/// @brief unmuted the app
	/// @param autoMute 
	virtual void Unmute(bool autoMute = false);

	/// @brief starts the loading thread
	void StartLoadingThread();
	/// @brief gets the loading thread progress
	/// @return the loading thread progress
	virtual double GetLoadingThreadProgress();

	/// @brief copies a string to clipboard
	/// @param theString 
	void CopyToClipboard(const std::string &theString);
	/// @brief gets the current clipboard
	/// @return string
	std::string GetClipboard();

	/// @brief sets a cursor by id
	/// @param theCursorNum 
	void SetCursor(int theCursorNum);
	/// @brief gets the current cursor id
	/// @return current cursor id as int
	int GetCursor();
	/// @brief enables custom cursors
	/// @param enabled 
	void EnableCustomCursors(bool enabled);
	/// @brief gets an image
	/// @param theFileName 
	/// @param commitBits 
	/// @return GPUImage
	virtual GPUImage *GetImage(const std::string &theFileName, bool commitBits = true);
	/// @brief gets a shared image
	/// @param theFileName 
	/// @param theVariant 
	/// @param isNew 
	/// @return SharedImageRef
	virtual SharedImageRef GetSharedImage(const std::string &theFileName, const std::string &theVariant = "",
										  bool *isNew = NULL);

	/// @brief sets taskbar icon
	/// @param theFileName 
	void SetTaskBarIcon(const std::string &theFileName);

	/// @brief cleans shared images
	void CleanSharedImages();
	/// @brief TBA
	/// @param theImage 
	void PrecacheAdditive(GPUImage *theImage);
	/// @brief TBA
	/// @param theImage 
	void PrecacheAlpha(GPUImage *theImage);
	/// @brief TBA
	/// @param theImage 
	void PrecacheNative(GPUImage *theImage);
	/// @brief sets a cursor by id and image
	/// @param theCursorNum 
	/// @param theImage 
	void SetCursorImage(int theCursorNum, Image *theImage);

	/// @brief creates a crossfade image
	/// @param theImage1 
	/// @param theRect1 
	/// @param theImage2 
	/// @param theRect2 
	/// @param theFadeFactor 
	/// @return GPUImage
	GPUImage *CreateCrossfadeImage(Image *theImage1, const Rect &theRect1, Image *theImage2, const Rect &theRect2,
								   double theFadeFactor);
	/// @brief TBA
	/// @param theImage 
	/// @param theColor 
	void ColorizeImage(Image *theImage, const Color &theColor);
	/// @brief creates a colorized image
	/// @param theImage 
	/// @param theColor 
	/// @return GPUImage
	GPUImage *CreateColorizedImage(Image *theImage, const Color &theColor);
	/// @brief copies an image
	/// @param theImage 
	/// @param theRect 
	/// @return GPUImage
	GPUImage *CopyImage(Image *theImage, const Rect &theRect);
	/// @brief copies an image
	/// @param theImage 
	/// @return GPUImage
	GPUImage *CopyImage(Image *theImage);
	/// @brief mirrors an image
	/// @param theImage 
	void MirrorImage(Image *theImage);
	/// @brief flips an image
	/// @param theImage 
	void FlipImage(Image *theImage);
	/// @brief rotates the image's hue
	/// @param theImage 
	/// @param theDelta 
	void RotateImageHue(PopLib::GPUImage *theImage, int theDelta);
	/// @brief converts HSL to RGB
	/// @param r 
	/// @param g 
	/// @param b 
	/// @return ulong
	ulong HSLToRGB(int h, int s, int l);
	/// @brief converts RGB to HSL
	/// @param r 
	/// @param g 
	/// @param b 
	/// @return ulong
	ulong RGBToHSL(int r, int g, int b);
	/// @brief converts HSL to RGB
	/// @param theSource 
	/// @param theDest 
	/// @param theSize 
	void HSLToRGB(const ulong *theSource, ulong *theDest, int theSize);
	/// @brief converts RGB to HSL
	/// @param theSource 
	/// @param theDest 
	/// @param theSize 
	void RGBToHSL(const ulong *theSource, ulong *theDest, int theSize);

	/// @brief adds a memory image
	/// @param theGPUImage 
	void AddGPUImage(GPUImage *theGPUImage);
	/// @brief removes a memory image
	/// @param theGPUImage 
	void RemoveGPUImage(GPUImage *theGPUImage);
	/// @brief removes 3d data
	/// @param theGPUImage 
	void Remove3DData(GPUImage *theGPUImage);
	/// @brief switches the current screenmode
	virtual void SwitchScreenMode();
	/// @brief switches the current screenmode
	/// @param wantWindowed 
	virtual void SwitchScreenMode(bool wantWindowed);
	/// @brief switches the current screenmode
	/// @param wantWindowed 
	/// @param is3d 
	/// @param force 
	virtual void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false);
	/// @brief disables alpha
	/// @param isDisabled 
	virtual void SetAlphaDisabled(bool isDisabled);

	/// @brief does a dialog
	/// @param theDialogId 
	/// @param isModal 
	/// @param theDialogHeader 
	/// @param theDialogLines 
	/// @param theDialogFooter 
	/// @param theButtonMode 
	/// @return Dialog
	virtual Dialog *DoDialog(int theDialogId, bool isModal, const PopString &theDialogHeader,
							 const PopString &theDialogLines, const PopString &theDialogFooter,
							 int theButtonMode);
	/// @brief gets a dialog
	/// @param theDialogId 
	/// @return Dialog
	virtual Dialog *GetDialog(int theDialogId);
	/// @brief adds a dialog
	/// @param theDialogId 
	/// @param theDialog 
	virtual void AddDialog(int theDialogId, Dialog *theDialog);
	/// @brief adds a dialog
	/// @param theDialog 
	virtual void AddDialog(Dialog *theDialog);
	/// @brief destroys a dialog
	/// @param theDialogId 
	/// @param removeWidget 
	/// @param deleteWidget 
	/// @return true if success
	virtual bool KillDialog(int theDialogId, bool removeWidget, bool deleteWidget);
	/// @brief destroys a dialog by id only
	/// @param theDialogId 
	/// @return true if success
	virtual bool KillDialog(int theDialogId);
	/// @brief destroys a dialog by object
	/// @param theDialog 
	/// @return true if success
	virtual bool KillDialog(Dialog *theDialog);
	/// @brief get current dialog count
	/// @return int
	virtual int GetDialogCount();
	/// @brief opens the modal
	virtual void ModalOpen();
	/// @brief closes the modal
	virtual void ModalClose();
	/// @brief TBA
	/// @param theDialogId 
	/// @param theButtonId 
	virtual void DialogButtonPress(int theDialogId, int theButtonId);
	/// @brief TBA
	/// @param theDialogId 
	/// @param theButtonId 
	virtual void DialogButtonDepress(int theDialogId, int theButtonId);

	/// @brief got foucs?
	virtual void GotFocus();
	/// @brief lost focus?
	virtual void LostFocus();
	/// @brief is alt key used?
	/// @param wParam 
	/// @return true if yes 
	virtual bool IsAltKeyUsed(long wParam);
	/// @brief is the debug key down?
	/// @param theKey 
	/// @return true if yes
	virtual bool DebugKeyDown(int theKey);
	/// @brief (async) is the debug key down?
	/// @param theKey 
	/// @param ctrlDown 
	/// @param altDown 
	/// @return true if yes
	virtual bool DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown);
	/// @brief closes the async request
	virtual void CloseRequestAsync();
	/// @brief is 3d accelerated?
	/// @return true if yes
	bool Is3DAccelerated();
	/// @brief is 3d accelerated supported?
	/// @return true if yes
	bool Is3DAccelerationSupported();
	/// @brief is 3d accelerated recommended?
	/// @return true if yes
	bool Is3DAccelerationRecommended();
	/// @brief sets 3d acceleration
	/// @param is3D 
	/// @param reinit 
	void Set3DAcclerated(bool is3D, bool reinit = true);
	/// @brief done 3d testing
	virtual void Done3dTesting();
	/// @brief notify crash hook
	/// @return return file name that you want to upload
	virtual std::string NotifyCrashHook();

	/// @brief TBA
	/// @param theBuffer 
	/// @param theFileName 
	/// @return true if success
	virtual bool CheckSignature(const Buffer &theBuffer, const std::string &theFileName);
	/// @brief TBA
	/// @return true if success
	virtual bool DrawDirtyStuff();
	/// @brief redraw
	/// @param theClipRect 
	virtual void Redraw(Rect *theClipRect);

	// Properties access methods

	/// @brief loads properties
	/// @param theFileName 
	/// @param required 
	/// @param checkSig 
	/// @return true if success
	bool LoadProperties(const std::string &theFileName, bool required, bool checkSig);
	/// @brief loads properties
	/// @return true if success
	bool LoadProperties();
	/// @brief initalize properties hook
	virtual void InitPropertiesHook();

	// Resource access methods

	/// @brief loads resource manifest
	void LoadResourceManifest();
	/// @brief shows resource error
	/// @param doExit 
	void ShowResourceError(bool doExit = false);

	/// @brief gets a bool from id (string)
	/// @param theId 
	/// @return the bool
	bool GetBoolean(const std::string &theId);
	/// @brief gets a bool from id (string) 
	/// @param theId 
	/// @param theDefault 
	/// @return the bool
	bool GetBoolean(const std::string &theId, bool theDefault);
	/// @brief gets an integer from id (string) 
	/// @param theId 
	/// @return the int
	int GetInteger(const std::string &theId);
	/// @brief gets an integer from id (string) 
	/// @param theId 
	/// @param theDefault 
	/// @return the int
	int GetInteger(const std::string &theId, int theDefault);
	/// @brief gets a double from id (string) 
	/// @param theId 
	/// @return the double
	double GetDouble(const std::string &theId);
	/// @brief gets a double from id (string) 
	/// @param theId 
	/// @param theDefault 
	/// @return the double
	double GetDouble(const std::string &theId, double theDefault);
	/// @brief gets a PopString from id (string)
	/// @param theId 
	/// @return the PopString
	PopString GetString(const std::string &theId);
	/// @brief gets a PopString from id (string)
	/// @param theId 
	/// @param theDefault 
	/// @return the PopString
	PopString GetString(const std::string &theId, const PopString &theDefault);

	/// @brief gets a string vector from id (string) 
	/// @param theId 
	/// @return the string vector
	StringVector GetStringVector(const std::string &theId);

	/// @brief sets a bool by id (string)
	/// @param theId 
	/// @param theValue 
	void SetBoolean(const std::string &theId, bool theValue);
	/// @brief sets an int by id (string)
	/// @param theId 
	/// @param theValue 
	void SetInteger(const std::string &theId, int theValue);
	/// @brief sets a double by id (string)
	/// @param theId 
	/// @param theValue 
	void SetDouble(const std::string &theId, double theValue);
	/// @brief sets a string by id (string, widestring)
	/// @param theId 
	/// @param theValue
	void SetString(const std::string &theId, const std::string &theValue);

	// Registry access methods

	/// @brief gets subkeys from json
	/// @param theKeyName 
	/// @param theSubKeys 
	/// @return true if success
	bool RegistryGetSubKeys(const std::string &theKeyName, StringVector *theSubKeys);
	/// @brief reads a string from json
	/// @param theValueName 
	/// @param theString 
	/// @return true if success
	bool RegistryReadString(const std::string &theValueName, std::string *theString);
	/// @brief reads an int from json
	/// @param theValueName 
	/// @param theValue 
	/// @return true if success
	bool RegistryReadInteger(const std::string &theValueName, int *theValue);
	/// @brief reads a bool from json
	/// @param theValueName 
	/// @param theValue 
	/// @return true if success
	bool RegistryReadBoolean(const std::string &theValueName, bool *theValue);
	/// @brief reads data from json
	/// @param theValueName 
	/// @param theValue 
	/// @param theLength 
	/// @return true if success 
	bool RegistryReadData(const std::string &theValueName, uchar *theValue, ulong *theLength);
	/// @brief writes a string to json
	/// @param theValueName 
	/// @param theString 
	/// @return true if success
	bool RegistryWriteString(const std::string &theValueName, const std::string &theString);
	/// @brief writes an int to json
	/// @param theValueName 
	/// @param theValue 
	/// @return true if success
	bool RegistryWriteInteger(const std::string &theValueName, int theValue);
	/// @brief writes a bool to json
	/// @param theValueName 
	/// @param theValue 
	/// @return true if success
	bool RegistryWriteBoolean(const std::string &theValueName, bool theValue);
	/// @brief writes data to json
	/// @param theValueName 
	/// @param theValue 
	/// @param theLength 
	/// @return true if success
	bool RegistryWriteData(const std::string &theValueName, const uchar *theValue, ulong theLength);
	/// @brief erases a key from json
	/// @param theKeyName 
	/// @return true if success
	bool RegistryEraseKey(const PopString &theKeyName);
	/// @brief erases a value from json
	/// @param theValueName 
	void RegistryEraseValue(const PopString &theValueName);

	// File access methods

	/// @brief writes buffer to file
	/// @param theFileName 
	/// @param theBuffer 
	/// @return true on success
	bool WriteBufferToFile(const std::string &theFileName, const Buffer *theBuffer);
	/// @brief reads buffer from file
	/// @param theFileName 
	/// @param theBuffer 
	/// @return true if success
	bool ReadBufferFromFile(const std::string &theFileName, Buffer *theBuffer); // UNICODE
	/// @brief writes bytes to file
	/// @param theFileName 
	/// @param theData 
	/// @param theDataLen 
	/// @return true if success
	bool WriteBytesToFile(const std::string &theFileName, const void *theData, ulong theDataLen);
	/// @brief checks if a file exists
	/// @param theFileName 
	/// @return true if yes
	bool FileExists(const std::string &theFileName);
	/// @brief erases a file
	/// @param theFileName 
	/// @return true if success
	bool EraseFile(const std::string &theFileName);

	// Misc methods

	/// @brief does the main loop
	virtual void DoMainLoop();
	/// @brief TBA
	/// @param updated 
	/// @return true if success
	virtual bool UpdateAppStep(bool *updated);
	/// @brief updates the app
	/// @return true if success
	virtual bool UpdateApp();
	/// @brief initializes the SDL interface
	/// @return int
	int InitInterface();
	/// @brief TBA
	/// @param relaxForASecond 
	void ClearUpdateBacklog(bool relaxForASecond = false);
	/// @brief is the app a screensaver?
	/// @return true if yes
	bool IsScreenSaver();
	/// @brief can the app restore?
	/// @return true if yes
	virtual bool AppCanRestore();
};

/// @brief the global appbase object
extern AppBase *gAppBase;

}; // namespace PopLib

#endif