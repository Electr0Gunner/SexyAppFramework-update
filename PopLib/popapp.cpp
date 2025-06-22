#include "popapp.hpp"
#include "api/discord.hpp"
#include "math/mtrand.hpp"
#include <SDL3/SDL.h>
#include "debug/debug.hpp"
#include <fstream>
#include "common.hpp"
#include <time.h>
#include "readwrite/jsonparser.hpp"
#include <filesystem>

namespace fs = std::filesystem;

using namespace PopLib;

PopApp *PopLib::gPopApp = nullptr;

// Groups of 80-byte data
const char DYNAMIC_DATA_BLOCK[400] = "DYN00000PACPOPPOPCAPPACPOPPOPCAPBUILDINFOMARKERPACPOPPOPCAPPACPOPPOPCAPXXXXXXXXX"
									 "00000000PACPOPPOPCAPPACPOPPOPCAPBUILDINFOMARKERPACPOPPOPCAPPACPOPPOPCAPXXXXXXXXX";

const char *BUILD_INFO_MARKER = DYNAMIC_DATA_BLOCK + 80;
const char *SIGNATURE_CODE_MARKER = DYNAMIC_DATA_BLOCK + 80 * 2;
const char *BETA_ID_MARKER = DYNAMIC_DATA_BLOCK + 80 * 3;

PopApp::PopApp()
{
	gPopApp = this;

	mTimesPlayed = 0;
	mTimesExecuted = 0;
	mTimedOut = false;

	mIsRegistered = false;
	mBuildUnlocked = false;
	mDownloadId = 0;
	mRegSource = "ingame";
	mSkipAd = false;
	mDontUpdate = false;
	mLastVerCheckQueryTime = 0;

	mCompanyName = "PLACEHOLDER";
	mFullCompanyName = "PLACEHOLDER";
	mBetaValidate = true;

	char aStr[9] = {0};
	strncpy(aStr, BUILD_INFO_MARKER, 8);
	mBuildNum = atoi(aStr);

	if (mBuildNum != 0)
		mBuildDate = BUILD_INFO_MARKER + 8;

#ifdef _FEATURE_DISCORD_RPC
	mRPCAppID = "1369297870456488057";
	mDiscordRPC = nullptr;
#endif
}

PopApp::~PopApp()
{
#ifdef _FEATURE_DISCORD_RPC
	if (mDiscordRPC)
		delete mDiscordRPC;
#endif
}

void PopApp::Init()
{
	AppBase::Init();

	if (IsScreenSaver())
		mSkipAd = true;

	mTimesExecuted++;
}

void PopApp::InitHook()
{
#if _FEATURE_DISCORD_RPC
	InitDiscordRPC();
#endif
}

void PopApp::UpdateFrames()
{
	AppBase::UpdateFrames();

#ifdef _FEATURE_DISCORD_RPC
	if (mDiscordRPC && mDiscordRPC->mHasInitialized)
		mDiscordRPC->UpdateRPC();
#endif
}

bool PopApp::CheckSignature(const Buffer &theBuffer, const std::string &theFileName)
{
#ifdef _DEBUG
	// Don't check signatures on debug version because it's annoying and the build number
	//  will probably be 0 anyway
	return true;
#endif

	/*if (mSkipSignatureChecks)
		return true;

	char aSigStr[25];

	FILE* aFP = fopen((theFileName + ".sig").c_str(), "rb");
	if (aFP == NULL)
		return false;

	fread(aSigStr, 1, 24, aFP);
	aSigStr[24] = 0;

	fclose(aFP);

	char* aFileData = new char[theBuffer.GetDataLen()+4];
	int aFileDataPos = 0;

	char aStr[9] = {0};
	strncpy(aStr, SIGNATURE_CODE_MARKER, 8);
	int aSignatureCode = atoi(aStr);

	aFileData[aFileDataPos++] = (aSignatureCode & 0xFF);
	aFileData[aFileDataPos++] = ((aSignatureCode >> 8) & 0xFF);
	aFileData[aFileDataPos++] = ((aSignatureCode >> 16) & 0xFF);
	aFileData[aFileDataPos++] = ((aSignatureCode >> 24) & 0xFF);

	theBuffer.SeekFront();
	while (!theBuffer.AtEnd())
	{
		unsigned char c = theBuffer.ReadByte();
		fread(&c, 1, 1, aFP);
		if (!::isspace(c))
			aFileData[aFileDataPos++] = c;
	}

	// Public RSA stuff
	BigInt n("D99BC76AB7B2578738E606F7");
	BigInt e("11");

	BigInt aHash = HashData(aFileData, aFileDataPos, 94);
	delete aFileData;

	BigInt aSignature(aSigStr);
	BigInt aHashTest = aSignature.ModPow(e, n);

	return aHashTest == aHash;*/
	return true;
}

void PopApp::InitPropertiesHook()
{
	// Load properties if we need to
	bool checkSig = !IsScreenSaver();
	LoadProperties("properties\\partner.xml", false, checkSig);

	// Check to see if this build is unlocked.
	if (GetBoolean("NoReg", false))
	{
		mIsRegistered = true;
		mBuildUnlocked = true;
	}

	mProdName = GetString("ProdName", mProdName);
	mIsWindowed = GetBoolean("DefaultWindowed", mIsWindowed);

	PopString aNewTitle = GetString("Title", "");
	if (aNewTitle.length() > 0)
		mTitle = aNewTitle + " " + mProductVersion;
}

bool PopApp::Validate(const std::string &theUserName, const std::string &theRegCode)
{
	/*BigInt n("42BF94023BBA6D040C8B81D9");
	BigInt e("11");

	ulong i;
	std::string aDataString;
	bool space = false;
	for (i = 0; i < theUserName.size(); i++)
	{
		if (theUserName[i] == ' ')
		{
			if (aDataString.length() > 0)
				space = true;
		}
		else
		{
			if (space)
			{
				aDataString += " ";
				space = false;
			}

			char aChar = theUserName[i];
			for (int j = 0; gRegKeyTranslationTable[j][0] != 0; j++)
				if (gRegKeyTranslationTable[j][0] == aChar)
					aChar = gRegKeyTranslationTable[j][1];

			aDataString += toupper(aChar);
		}
	}

	std::string aProduct;
	aProduct = mProdName;
	for (i = 0; i < aProduct.length(); i++)
		aProduct[i] = toupper(aProduct[i]);

	aDataString += "\n";
	aDataString += aProduct;
	BigInt aHash = HashString(aDataString, 94);

	BigInt aSignature = KeyToInt(theRegCode);
	BigInt aHashTest = aSignature.ModPow(e, n);

	return aHashTest == aHash;*/
	return true;
}

void PopApp::WriteToRegistry()
{
	AppBase::WriteToRegistry();

	std::string jsonFileName = GetAppDataFolder() + "popcinfo.json";

	JsonParser parser;

	bool loaded = parser.OpenFile(jsonFileName);
	nlohmann::json &jData = parser.mJson;

	if (!loaded)
	{
		// Failed to load or file doesn't exist: start fresh JSON
		jData = nlohmann::json::object();
	}

	if (!jData.contains("products") || !jData["products"].is_array())
	{
		jData["products"] = nlohmann::json::array();
	}

	bool productFound = false;
	for (auto &product : jData["products"])
	{
		if (product.contains("name") && product["name"].is_string() && product["name"] == mProdName)
		{
			product["timesPlayed"] = mTimesPlayed;
			product["timesExecuted"] = mTimesExecuted;
			productFound = true;
			break;
		}
	}

	if (!productFound)
	{
		nlohmann::json newProduct;
		newProduct["name"] = mProdName;
		newProduct["timesPlayed"] = mTimesPlayed;
		newProduct["timesExecuted"] = mTimesExecuted;
		jData["products"].push_back(newProduct);
	}

	// Write updated JSON back to file
	std::ofstream outFile(jsonFileName, std::ios::trunc | std::ios::binary);
	if (outFile.is_open())
	{
		outFile << jData.dump(4); // pretty print
		outFile.close();
	}

	RegistryWriteInteger("LastVerCheckQueryTime", mLastVerCheckQueryTime);
	RegistryWriteInteger("TimesPlayed", mTimesPlayed);
	RegistryWriteInteger("TimesExecuted", mTimesExecuted);

	// This is for "compatibility"
	if ((mRegUserName.length() == 0) && (mUserName.length() > 0) && (mRegCode.length() > 0))
		mRegUserName = mUserName;

	if (mRegUserName.length() > 0)
		RegistryWriteString("RegName", mRegUserName);

	if (mRegCode.length() > 0)
		RegistryWriteString("RegCode", mRegCode);
}

void PopApp::ReadFromRegistry()
{
	AppBase::ReadFromRegistry();

	mTimesPlayed = 0;
	mTimesExecuted = 0;

	std::string jsonFileName = GetAppDataFolder() + "popcinfo.json";

	JsonParser parser;

	if (parser.OpenFile(jsonFileName))
	{
		nlohmann::json &jData = parser.mJson;

		if (jData.contains("products") && jData["products"].is_array())
		{
			const std::string prodNameStr = mProdName; // assuming std::string

			for (const auto &product : jData["products"])
			{
				if (product.contains("name") && product["name"].is_string() && product["name"] == prodNameStr)
				{
					if (product.contains("timesPlayed") && product["timesPlayed"].is_number_integer())
						mTimesPlayed = product["timesPlayed"].get<int>();

					if (product.contains("timesExecuted") && product["timesExecuted"].is_number_integer())
						mTimesExecuted = product["timesExecuted"].get<int>();

					break;
				}
			}
		}
	}
	// if file does not exist or parse failed, keep default values (0)

	RegistryReadString("ReferId", &mReferId);
	mReferId = GetString("ReferId", mReferId);
	mRegisterLink = "http://www.popcap.com/register.php?theGame=" + mProdName + "&referid=" + mReferId;
	RegistryReadString("RegisterLink", &mRegisterLink);

	int anInt;

	if (RegistryReadInteger("DontUpdate", &anInt))
		mDontUpdate = anInt != 0;

	if (RegistryReadInteger("DownloadId", &anInt))
		mDownloadId = anInt;

	RegistryReadString("Variation", &mVariation);

	if (RegistryReadInteger("TimesPlayed", &anInt))
	{
		if (mTimesPlayed != anInt)
			mTimesPlayed = 100;
	}

	if (RegistryReadInteger("TimesExecuted", &anInt))
	{
		if (mTimesExecuted != anInt)
			mTimesExecuted = 100;
	}

	if (RegistryReadInteger("LastVerCheckQueryTime", &anInt))
	{
		mLastVerCheckQueryTime = anInt;
	}
	else
	{
		time_t aTimeNow;
		time(&aTimeNow);

		mLastVerCheckQueryTime = aTimeNow;
	}

	RegistryReadString("RegName", &mRegUserName);
	RegistryReadString("RegCode", &mRegCode);

	mIsRegistered |= Validate(mRegUserName, mRegCode);

	// Override registry values with partner.xml values
	mRegisterLink = GetString("RegisterLink", mRegisterLink);
	mDontUpdate = GetBoolean("DontUpdate", mDontUpdate);
}

#ifdef _FEATURE_DISCORD_RPC
void PopApp::InitDiscordRPC()
{
	if (mDiscordRPC)
		delete mDiscordRPC;

	mDiscordRPC = new DiscordRPC(mRPCAppID.c_str());
}
#endif

void PopApp::HandleCmdLineParam(const std::string &theParamName, const std::string &theParamValue)
{
	if (theParamName == "-version")
	{
		// Just print version info and then quit

		std::string aVersionString = "Product: " + mProdName + "\n" + "Version: " + mProductVersion + "\n" +
									 "Build Num: " + StrFormat("%d", mBuildNum) + "\n" + "Build Date: " + mBuildDate;

		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Version Info", aVersionString.c_str(), NULL);
		DoExit(0);
	}
	else
		AppBase::HandleCmdLineParam(theParamName, theParamValue);
}

void PopApp::OpenUpdateURL()
{
	Shutdown();
}

bool PopApp::OpenHTMLTemplate(const std::string &theTemplateFile, const DefinesMap &theDefinesMap)
{
	std::fstream anInStream(theTemplateFile.c_str(), std::ios::in);

	if (!anInStream.is_open())
		return false;

	std::filesystem::path tempDir = "temp";
	if (fs::exists(tempDir) && fs::is_directory(tempDir))
	{
		for (auto &entry : fs::directory_iterator(tempDir))
		{
			if (entry.is_regular_file())
			{
				std::string filename = entry.path().filename().string();
				// Check if filename matches tpl*.html pattern
				if (filename.size() >= 8 && filename.substr(0, 3) == "tpl" &&
					filename.substr(filename.size() - 5) == ".html")
				{
					fs::remove(entry.path());
				}
			}
		}
	}

	MkDir("temp");

	std::string anOutFilename = StrFormat("temp\\tpl%04d.html", PopLib::Rand() % 10000);

	// TODO: A better failover case?
	std::fstream anOutStream(anOutFilename.c_str(), std::ios::out);
	if (!anOutStream.is_open())
		return false;

	char aStr[4096];
	while (!anInStream.eof())
	{
		anInStream.getline(aStr, 4096);

		std::string aNewString = Evaluate(aStr, theDefinesMap);

		anOutStream << aNewString.c_str() << std::endl;
	}

	return OpenURL(GetFullPath(anOutFilename));
}

bool PopApp::OpenRegisterPage(DefinesMap theStatsMap)
{
	// Insert standard defines
	DefinesMap aDefinesMap;

	aDefinesMap.insert(DefinesMap::value_type("Src", mRegSource));
	aDefinesMap.insert(DefinesMap::value_type("ProdName", mProdName));
	aDefinesMap.insert(DefinesMap::value_type("Version", mProductVersion));
	aDefinesMap.insert(DefinesMap::value_type("Variation", mVariation));
	aDefinesMap.insert(DefinesMap::value_type("ReferId", mReferId));
	aDefinesMap.insert(DefinesMap::value_type("DownloadId", StrFormat("%d", mDownloadId).c_str()));
	aDefinesMap.insert(DefinesMap::value_type("TimesPlayed", StrFormat("%d", mTimesPlayed).c_str()));
	aDefinesMap.insert(DefinesMap::value_type("TimesExecuted", StrFormat("%d", mTimesExecuted).c_str()));
	aDefinesMap.insert(DefinesMap::value_type("TimedOut", mTimedOut ? "Y" : "N"));

	// Insert game specific stats
	std::string aStatsString;
	DefinesMap::iterator anItr = theStatsMap.begin();
	while (anItr != theStatsMap.end())
	{
		std::string aKeyString = anItr->first;
		std::string aValueString = anItr->second;

		aStatsString += StrFormat("%04X", aKeyString.length()).c_str() + aKeyString + "S" +
						StrFormat("%04X", aValueString.length()).c_str() + aValueString;

		++anItr;
	}

	aDefinesMap.insert(DefinesMap::value_type("Stats", aStatsString));

	if (FileExists("register.tpl"))
	{
		return OpenHTMLTemplate("register.tpl", aDefinesMap);
	}
	else
	{
		return OpenURL(mRegisterLink);
	}
}

bool PopApp::ShouldCheckForUpdate()
{
	if (mDontUpdate)
		return false;
	time_t aTimeNow;
	time(&aTimeNow);

	// It is set to 0 if we crash, otherwise ask every week
	return ((mLastVerCheckQueryTime == 0) || (!mLastShutdownWasGraceful) ||
			((mLastVerCheckQueryTime != 0) && (aTimeNow - mLastVerCheckQueryTime > 7 * 24 * 60 * 60)));
}

void PopApp::UpdateCheckQueried()
{
	time_t aTimeNow;
	time(&aTimeNow);

	mLastVerCheckQueryTime = aTimeNow;
}

void PopApp::URLOpenSucceeded(const std::string &theURL)
{
	AppBase::URLOpenSucceeded(theURL);

	if (mShutdownOnURLOpen)
		mSkipAd = true;
}

bool PopApp::OpenRegisterPage()
{
	DefinesMap aStatsMap;
	return OpenRegisterPage(aStatsMap);
}
