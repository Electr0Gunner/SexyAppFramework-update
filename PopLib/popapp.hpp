#ifndef __POPAPP_HPP__
#define __POPAPP_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "appbase.hpp"

namespace PopLib
{
class DiscordRPC;

class PopApp : public AppBase
{
  public:
#ifdef _FEATURE_DISCORD_RPC
	DiscordRPC *mDiscordRPC;

	std::string mRPCAppID;
#endif

	std::string mBetaSupportSiteOverride;
	std::string mBetaSupportProdNameOverride;
	std::string mReferId;
	std::string mVariation;
	ulong mDownloadId;
	std::string mRegSource;
	ulong mLastVerCheckQueryTime;
	bool mSkipAd;
	bool mDontUpdate;

	int mBuildNum;
	std::string mBuildDate;

	std::string mUserName;
	std::string mRegUserName;
	std::string mRegCode;
	bool mIsRegistered;
	bool mBuildUnlocked;

	int mTimesPlayed;
	int mTimesExecuted;
	bool mTimedOut;

  public:
	virtual void UpdateFrames();

	virtual void WriteToRegistry();
	virtual void ReadFromRegistry();

	virtual bool CheckSignature(const Buffer &theBuffer, const std::string &theFileName);

	virtual bool ShouldCheckForUpdate();
	virtual void UpdateCheckQueried();

	virtual void URLOpenSucceeded(const std::string &theURL);

  public:
	PopApp();
	virtual ~PopApp();

	virtual void Init();
	virtual void InitHook();
	virtual void InitPropertiesHook();
	virtual void OpenUpdateURL();

	bool Validate(const std::string &theUserName, const std::string &theRegCode);

	virtual bool OpenRegisterPage(DefinesMap theDefinesMap);
	virtual bool OpenRegisterPage();

	virtual void HandleCmdLineParam(const std::string &theParamName, const std::string &theParamValue);
	virtual bool OpenHTMLTemplate(const std::string &theTemplateFile, const DefinesMap &theDefinesMap);
#if _FEATURE_DISCORD_RPC
	virtual void InitDiscordRPC();
#endif
};

extern PopApp *gPopApp;
} // namespace PopLib

#endif