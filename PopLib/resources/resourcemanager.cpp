#include <memory>
#include "resourcemanager.hpp"
#include "readwrite/xmlparser.hpp"
#include "audio/soundmanager.hpp"
#include "graphics/sdlimage.hpp"
#include "graphics/sdlinterface.hpp"
#include "graphics/imagefont.hpp"
#include "graphics/sysfont.hpp"
#include "imagelib/imagelib.hpp"

#include "debug/perftimer.hpp"

using namespace PopLib;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::ImageRes::DeleteResource()
{
	mImage.Release();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::SoundRes::DeleteResource()
{
	if (mSoundId >= 0)
		gAppBase->mSoundManager->ReleaseSound(mSoundId);

	mSoundId = -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::FontRes::DeleteResource()
{
	delete mFont;
	mFont = NULL;

	delete mImage;
	mImage = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ResourceManager::ResourceManager(AppBase *theApp)
{
	mApp = theApp;
	mHasFailed = false;
	mXMLParser = NULL;

	mAllowMissingProgramResources = false;
	mAllowAlreadyDefinedResources = false;
	mCurResGroupList = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ResourceManager::~ResourceManager()
{
	DeleteMap(mImageMap);
	DeleteMap(mSoundMap);
	DeleteMap(mFontMap);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::IsGroupLoaded(const std::string &theGroup)
{
	return mLoadedGroups.find(theGroup) != mLoadedGroups.end();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteMap(ResMap &theMap)
{
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
	{
		anItr->second->DeleteResource();
		delete anItr->second;
	}

	theMap.clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteResources(ResMap &theMap, const std::string &theGroup)
{
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
	{
		if (theGroup.empty() || anItr->second->mResGroup == theGroup)
			anItr->second->DeleteResource();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteResources(const std::string &theGroup)
{
	DeleteResources(mImageMap, theGroup);
	DeleteResources(mSoundMap, theGroup);
	DeleteResources(mFontMap, theGroup);
	mLoadedGroups.erase(theGroup);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteExtraImageBuffers(const std::string &theGroup)
{
	for (ResMap::iterator anItr = mImageMap.begin(); anItr != mImageMap.end(); ++anItr)
	{
		if (theGroup.empty() || anItr->second->mResGroup == theGroup)
		{
			ImageRes *aRes = (ImageRes *)anItr->second;
			MemoryImage *anImage = (MemoryImage *)aRes->mImage;
			if (anImage != NULL)
				anImage->DeleteExtraBuffers();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
std::string ResourceManager::GetErrorText()
{
	return mError;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::HadError()
{
	return mHasFailed;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::Fail(const std::string &theErrorText)
{
	if (!mHasFailed)
	{
		mHasFailed = true;
		if (!mXMLParser)
		{
			mError = theErrorText;
			return false;
		}

		int aLineNum = mXMLParser->GetCurrentLineNum();

		char aLineNumStr[16];
		sprintf(aLineNumStr, "%d", aLineNum);

		mError = theErrorText;

		if (aLineNum > 0)
			mError += std::string(" on Line ") + aLineNumStr;

		if (mXMLParser->GetFileName().length() > 0)
			mError += " in File '" + mXMLParser->GetFileName() + "'";
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseCommonResource(XMLElement &theElement, BaseRes *theRes, ResMap &theMap)
{
	mHadAlreadyDefinedError = false;

	const PopString &aPath = theElement.mAttributes["path"];
	if (aPath.empty())
		return Fail("No path specified.");

	theRes->mXMLAttributes = theElement.mAttributes;
	theRes->mFromProgram = false;
	if (aPath[0] == '!')
	{
		theRes->mPath = aPath;
		if (aPath == "!program")
			theRes->mFromProgram = true;
	}
	else
		theRes->mPath = mDefaultPath + aPath;

	std::string anId;
	XMLParamMap::iterator anItr = theElement.mAttributes.find("id");
	if (anItr == theElement.mAttributes.end())
		anId = mDefaultIdPrefix + GetFileName(theRes->mPath, true);
	else
		anId = mDefaultIdPrefix + anItr->second;

	theRes->mResGroup = mCurResGroup;
	theRes->mId = anId;

	std::pair<ResMap::iterator, bool> aRet = theMap.insert(ResMap::value_type(anId, theRes));
	if (!aRet.second)
	{
		mHadAlreadyDefinedError = true;
		return Fail("Resource already defined.");
	}

	mCurResGroupList->push_back(theRes);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseSoundResource(XMLElement &theElement)
{
	SoundRes *aRes = new SoundRes;
	aRes->mSoundId = -1;
	aRes->mVolume = -1;
	aRes->mPanning = 0;

	if (!ParseCommonResource(theElement, aRes, mSoundMap))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			SoundRes *oldRes = aRes;
			aRes = (SoundRes *)mSoundMap[oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
	}

	XMLParamMap::iterator anItr;

	anItr = theElement.mAttributes.find("volume");
	if (anItr != theElement.mAttributes.end())
		sscanf(anItr->second.c_str(), "%lf", &aRes->mVolume);

	anItr = theElement.mAttributes.find("pan");
	if (anItr != theElement.mAttributes.end())
		sscanf(anItr->second.c_str(), "%d", &aRes->mPanning);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void ReadIntVector(const PopString &theVal, std::vector<int> &theVector)
{
	theVector.clear();

	std::string::size_type aPos = 0;
	while (true)
	{
		theVector.push_back(atoi(theVal.c_str() + aPos));
		aPos = theVal.find_first_of(',', aPos);
		if (aPos == std::string::npos)
			break;

		aPos++;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseImageResource(XMLElement &theElement)
{
	ImageRes *aRes = new ImageRes;
	if (!ParseCommonResource(theElement, aRes, mImageMap))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			ImageRes *oldRes = aRes;
			aRes = (ImageRes *)mImageMap[oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
	}

	aRes->mPalletize = theElement.mAttributes.find("nopal") == theElement.mAttributes.end();
	aRes->mA4R4G4B4 = theElement.mAttributes.find("a4r4g4b4") != theElement.mAttributes.end();
	aRes->mDDSurface = theElement.mAttributes.find("ddsurface") != theElement.mAttributes.end();
	aRes->mPurgeBits =
		(theElement.mAttributes.find("nobits") != theElement.mAttributes.end()) ||
		((mApp->Is3DAccelerated()) && (theElement.mAttributes.find("nobits3d") != theElement.mAttributes.end())) ||
		((!mApp->Is3DAccelerated()) && (theElement.mAttributes.find("nobits2d") != theElement.mAttributes.end()));
	aRes->mA8R8G8B8 = theElement.mAttributes.find("a8r8g8b8") != theElement.mAttributes.end();
	aRes->mNearestFilter = theElement.mAttributes.find("nearestfilter") != theElement.mAttributes.end();
	aRes->mAutoFindAlpha = theElement.mAttributes.find("noalpha") == theElement.mAttributes.end();

	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find("alphaimage");
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaImage = mDefaultPath + anItr->second;

	aRes->mAlphaColor = 0xFFFFFF;
	anItr = theElement.mAttributes.find("alphacolor");
	if (anItr != theElement.mAttributes.end())
		sscanf(anItr->second.c_str(), "%x", &aRes->mAlphaColor);

	anItr = theElement.mAttributes.find("variant");
	if (anItr != theElement.mAttributes.end())
		aRes->mVariant = anItr->second;

	anItr = theElement.mAttributes.find("alphagrid");
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaGridImage = mDefaultPath + anItr->second;

	anItr = theElement.mAttributes.find("rows");
	if (anItr != theElement.mAttributes.end())
		aRes->mRows = atoi(anItr->second.c_str());
	else
		aRes->mRows = 1;

	anItr = theElement.mAttributes.find("cols");
	if (anItr != theElement.mAttributes.end())
		aRes->mCols = atoi(anItr->second.c_str());
	else
		aRes->mCols = 1;

	anItr = theElement.mAttributes.find("anim");
	AnimType anAnimType = AnimType_None;
	if (anItr != theElement.mAttributes.end())
	{
		const PopChar *aType = anItr->second.c_str();

		if (stricmp(aType, "none") == 0)
			anAnimType = AnimType_None;
		else if (stricmp(aType, "once") == 0)
			anAnimType = AnimType_Once;
		else if (stricmp(aType, "loop") == 0)
			anAnimType = AnimType_Loop;
		else if (stricmp(aType, "pingpong") == 0)
			anAnimType = AnimType_PingPong;
		else
		{
			Fail("Invalid animation type.");
			return false;
		}
	}
	aRes->mAnimInfo.mAnimType = anAnimType;
	if (anAnimType != AnimType_None)
	{
		int aNumCels = std::max(aRes->mRows, aRes->mCols);
		int aBeginDelay = 0, anEndDelay = 0;

		anItr = theElement.mAttributes.find("framedelay");
		if (anItr != theElement.mAttributes.end())
			aRes->mAnimInfo.mFrameDelay = atoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find("begindelay");
		if (anItr != theElement.mAttributes.end())
			aBeginDelay = atoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find("enddelay");
		if (anItr != theElement.mAttributes.end())
			anEndDelay = atoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find("perframedelay");
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second, aRes->mAnimInfo.mPerFrameDelay);

		anItr = theElement.mAttributes.find("framemap");
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second, aRes->mAnimInfo.mFrameMap);

		aRes->mAnimInfo.Compute(aNumCels, aBeginDelay, anEndDelay);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseFontResource(XMLElement &theElement)
{
	FontRes *aRes = new FontRes;
	aRes->mFont = NULL;
	aRes->mImage = NULL;

	if (!ParseCommonResource(theElement, aRes, mFontMap))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			FontRes *oldRes = aRes;
			aRes = (FontRes *)mFontMap[oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
	}

	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find("image");
	if (anItr != theElement.mAttributes.end())
		aRes->mImagePath = anItr->second;

	anItr = theElement.mAttributes.find("tags");
	if (anItr != theElement.mAttributes.end())
		aRes->mTags = anItr->second;

	if (strncmp(aRes->mPath.c_str(), "!sys:", 5) == 0)
	{
		aRes->mSysFont = true;
		aRes->mPath = aRes->mPath.substr(5);

		anItr = theElement.mAttributes.find("size");
		if (anItr == theElement.mAttributes.end())
			return Fail("SysFont needs point size");

		aRes->mSize = atoi(anItr->second.c_str());
		if (aRes->mSize <= 0)
			return Fail("SysFont needs point size");

		aRes->mBold = theElement.mAttributes.find("bold") != theElement.mAttributes.end();
		aRes->mItalic = theElement.mAttributes.find("italic") != theElement.mAttributes.end();
		aRes->mShadow = theElement.mAttributes.find("shadow") != theElement.mAttributes.end();
		aRes->mUnderline = theElement.mAttributes.find("underline") != theElement.mAttributes.end();
	}
	else
		aRes->mSysFont = false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParsePopAnimResource(XMLElement &theElement)
{
	// TODO: Implement ParsePopAnimResource

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParsePIEffectResource(XMLElement &theElement)
{
	// TODO: Implement ParsePIEffectResource

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseSetDefaults(XMLElement &theElement)
{
	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find("path");
	if (anItr != theElement.mAttributes.end())
		mDefaultPath = RemoveTrailingSlash(anItr->second) + '/';

	anItr = theElement.mAttributes.find("idprefix");
	if (anItr != theElement.mAttributes.end())
		mDefaultIdPrefix = RemoveTrailingSlash(anItr->second);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseResources()
{
	for (;;)
	{
		XMLElement aXMLElement;
		if (!mXMLParser->NextElement(&aXMLElement))
			return false;

		if (aXMLElement.mType == XMLElement::TYPE_START)
		{
			if (aXMLElement.mValue == "Image")
			{
				if (!ParseImageResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == "Sound")
			{
				if (!ParseSoundResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == "Font")
			{
				if (!ParseFontResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == "PopAnim")
			{
				if (!ParsePopAnimResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == "PIEffect")
			{
				if (!ParsePIEffectResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == "SetDefaults")
			{
				if (!ParseSetDefaults(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else
			{
				Fail("Invalid Section '" + aXMLElement.mValue + "'");
				return false;
			}
		}
		else if (aXMLElement.mType == XMLElement::TYPE_ELEMENT)
		{
			Fail("Element Not Expected '" + aXMLElement.mValue + "'");
			return false;
		}
		else if (aXMLElement.mType == XMLElement::TYPE_END)
		{
			return true;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::DoParseResources()
{
	if (!mXMLParser->HasFailed())
	{
		for (;;)
		{
			XMLElement aXMLElement;
			if (!mXMLParser->NextElement(&aXMLElement))
				break;

			if (aXMLElement.mType == XMLElement::TYPE_START)
			{
				if (aXMLElement.mValue == "Resources")
				{
					mCurResGroup = aXMLElement.mAttributes["id"];
					mCurResGroupList = &mResGroupMap[mCurResGroup];

					if (mCurResGroup.empty())
					{
						Fail("No id specified.");
						break;
					}

					if (!ParseResources())
						break;
				}
				else
				{
					Fail("Invalid Section '" + aXMLElement.mValue + "'");
					break;
				}
			}
			else if (aXMLElement.mType == XMLElement::TYPE_ELEMENT)
			{
				Fail("Element Not Expected '" + aXMLElement.mValue + "'");
				break;
			}
		}
	}

	if (mXMLParser->HasFailed())
		Fail(mXMLParser->GetErrorText());

	delete mXMLParser;
	mXMLParser = NULL;

	return !mHasFailed;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ParseResourcesFile(const std::string &theFilename)
{
	mXMLParser = new XMLParser();
	if (!mXMLParser->OpenFile(theFilename))
		Fail("Resource file not found: " + theFilename);

	XMLElement aXMLElement;
	while (!mXMLParser->HasFailed())
	{
		if (!mXMLParser->NextElement(&aXMLElement))
			Fail(mXMLParser->GetErrorText());

		if (aXMLElement.mType == XMLElement::TYPE_START)
		{
			if (aXMLElement.mValue != "ResourceManifest")
				break;
			else
				return DoParseResources();
		}
	}

	Fail("Expecting ResourceManifest tag");

	return DoParseResources();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ReparseResourcesFile(const std::string &theFilename)
{
	bool oldDefined = mAllowAlreadyDefinedResources;
	mAllowAlreadyDefinedResources = true;

	bool aResult = ParseResourcesFile(theFilename);

	mAllowAlreadyDefinedResources = oldDefined;

	return aResult;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::LoadAlphaGridImage(ImageRes *theRes, SDLImage *theImage)
{
	ImageLib::Image *anAlphaImage = ImageLib::GetImage(theRes->mAlphaGridImage, true);
	if (!anAlphaImage)
		return Fail(StrFormat("Failed to load image: %s", theRes->mAlphaGridImage.c_str()));

	std::unique_ptr<ImageLib::Image> aDelAlphaImage(anAlphaImage);

	int aNumRows = theRes->mRows;
	int aNumCols = theRes->mCols;

	int aCelWidth = theImage->mWidth / aNumCols;
	int aCelHeight = theImage->mHeight / aNumRows;

	if (anAlphaImage->mWidth != aCelWidth || anAlphaImage->mHeight != aCelHeight)
		return Fail(StrFormat("GridAlphaImage size mismatch between %s and %s", theRes->mPath.c_str(),
							  theRes->mAlphaGridImage.c_str()));

	ulong *aMasterRowPtr = theImage->mBits;
	for (int i = 0; i < aNumRows; i++)
	{
		ulong *aMasterColPtr = aMasterRowPtr;
		for (int j = 0; j < aNumCols; j++)
		{
			ulong *aRowPtr = aMasterColPtr;
			ulong *anAlphaBits = anAlphaImage->mBits;
			for (int y = 0; y < aCelHeight; y++)
			{
				ulong *aDestPtr = aRowPtr;
				for (int x = 0; x < aCelWidth; x++)
				{
					*aDestPtr = (*aDestPtr & 0x00FFFFFF) | ((*anAlphaBits & 0xFF) << 24);
					++anAlphaBits;
					++aDestPtr;
				}
				aRowPtr += theImage->mWidth;
			}

			aMasterColPtr += aCelWidth;
		}
		aMasterRowPtr += aCelHeight * theImage->mWidth;
	}

	theImage->BitsChanged();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::LoadAlphaImage(ImageRes *theRes, SDLImage *theImage)
{
	PERF_BEGIN("ResourceManager::GetImage");
	ImageLib::Image *anAlphaImage = ImageLib::GetImage(theRes->mAlphaImage, true);
	PERF_END("ResourceManager::GetImage");

	if (!anAlphaImage)
		return Fail(StrFormat("Failed to load image: %s", theRes->mAlphaImage.c_str()));

	std::unique_ptr<ImageLib::Image> aDelAlphaImage(anAlphaImage);

	if (anAlphaImage->mWidth != theImage->mWidth || anAlphaImage->mHeight != theImage->mHeight)
		return Fail(StrFormat("AlphaImage size mismatch between %s and %s", theRes->mPath.c_str(),
							  theRes->mAlphaImage.c_str()));

	ulong *aBits1 = theImage->mBits;

	ulong *aBits2 = anAlphaImage->mBits;
	int aSize = theImage->mWidth * theImage->mHeight;

	for (int i = 0; i < aSize; i++)
	{
		*aBits1 = (*aBits1 & 0x00FFFFFF) | ((*aBits2 & 0xFF) << 24);
		++aBits1;
		++aBits2;
	}

	theImage->BitsChanged();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::DoLoadImage(ImageRes *theRes)
{
	bool lookForAlpha = theRes->mAlphaImage.empty() && theRes->mAlphaGridImage.empty() && theRes->mAutoFindAlpha;

	PERF_BEGIN("ResourceManager:GetImage");

	// ImageLib::Image *anImage = ImageLib::GetImage(theRes->mPath, lookForAlpha);
	// PERF_END("ResourceManager:GetImage");

	bool isNew;
	ImageLib::gAlphaComposeColor = theRes->mAlphaColor;
	SharedImageRef aSharedImageRef = gAppBase->GetSharedImage(theRes->mPath, theRes->mVariant, &isNew);
	ImageLib::gAlphaComposeColor = 0xFFFFFF;

	SDLImage *aSDLImage = (SDLImage *)aSharedImageRef;
	if (!aSDLImage)
		return Fail(StrFormat("Failed to load image: %s", theRes->mPath.c_str()));

	if (isNew)
	{
		if (!theRes->mAlphaImage.empty())
		{
			if (!LoadAlphaImage(theRes, aSharedImageRef))
				return false;
		}

		if (!theRes->mAlphaGridImage.empty())
		{
			if (!LoadAlphaGridImage(theRes, aSharedImageRef))
				return false;
		}
	}

	aSDLImage->CommitBits();
	theRes->mImage = aSharedImageRef;
	aSDLImage->mPurgeBits = theRes->mPurgeBits;

	if (theRes->mDDSurface)
	{
		PERF_BEGIN("ResourceManager:DDSurface");

		aSDLImage->CommitBits();

		if (!aSDLImage->mHasAlpha)
		{
			// aSDLImage->mWantDDSurface = true;
			aSDLImage->mPurgeBits = true;
		}

		PERF_END("ResourceManager:DDSurface");
	}

	if (theRes->mPalletize)
	{
		// PERF_BEGIN("ResourceManager:Palletize");
		// if (aSDLImage->mSurface==NULL)
		//	aSDLImage->Palletize();
		// else
		// aSDLImage->mWantPal = true;
		// PERF_END("ResourceManager:Palletize");
	}

	if (theRes->mNearestFilter)
		aSDLImage->mImageFlags |= SDLImageFlag_NearestFiltering;

	if (theRes->mAnimInfo.mAnimType != AnimType_None)
		aSDLImage->mAnimInfo = new AnimInfo(theRes->mAnimInfo);

	aSDLImage->mNumRows = theRes->mRows;
	aSDLImage->mNumCols = theRes->mCols;

	if (aSDLImage->mPurgeBits)
		aSDLImage->PurgeBits();

	ResourceLoadedHook(theRes);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteImage(const std::string &theName)
{
	ReplaceImage(theName, NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SharedImageRef ResourceManager::LoadImage(const std::string &theName)
{
	ResMap::iterator anItr = mImageMap.find(theName);
	if (anItr == mImageMap.end())
		return NULL;

	ImageRes *aRes = (ImageRes *)anItr->second;
	if ((SDLImage *)aRes->mImage != NULL)
		return aRes->mImage;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadImage(aRes))
		return NULL;

	return aRes->mImage;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::DoLoadSound(SoundRes *theRes)
{
	SoundRes *aRes = theRes;

	PERF_BEGIN("ResourceManager:LoadSound");
	int aSoundId = mApp->mSoundManager->GetFreeSoundId();
	if (aSoundId < 0)
		return Fail("Out of free sound ids");

	if (!mApp->mSoundManager->LoadSound(aSoundId, aRes->mPath))
		return Fail(StrFormat("Failed to load sound: %s", aRes->mPath.c_str()));
	PERF_END("ResourceManager:LoadSound");

	if (aRes->mVolume >= 0)
		mApp->mSoundManager->SetBaseVolume(aSoundId, aRes->mVolume);

	if (aRes->mPanning != 0)
		mApp->mSoundManager->SetBasePan(aSoundId, aRes->mPanning);

	aRes->mSoundId = aSoundId;

	ResourceLoadedHook(theRes);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::DoLoadFont(FontRes *theRes)
{
	Font *aFont = NULL;

	PERF_BEGIN("ResourceManager:DoLoadFont");

	if (theRes->mSysFont)
	{
		bool bold = theRes->mBold, simulateBold = false;
		if (PopLib::CheckFor98Mill())
		{
			simulateBold = bold;
			bold = false;
		}
		aFont = new SysFont(theRes->mPath, theRes->mSize, bold, theRes->mItalic, theRes->mUnderline);
		SysFont *aSysFont = (SysFont *)aFont;
		aSysFont->mDrawShadow = theRes->mShadow;
		aSysFont->mSimulateBold = simulateBold;
	}
	else if (theRes->mImagePath.empty())
	{
		if (strncmp(theRes->mPath.c_str(), "!ref:", 5) == 0)
		{
			std::string aRefName = theRes->mPath.substr(5);
			Font *aRefFont = ((FontRes *)GetFontRef(aRefName)->mBaseResP)->mFont;
			if (!aRefFont)
				return Fail("Ref font not found: " + aRefName);

			aFont = aRefFont->Duplicate();
		}
		else
			aFont = new ImageFont(mApp, theRes->mPath);
	}
	else
	{
		Image *anImage = mApp->GetImage(theRes->mImagePath);
		if (!anImage)
			return Fail(StrFormat("Failed to load image: %s", theRes->mImagePath.c_str()));

		theRes->mImage = anImage;
		aFont = new ImageFont(anImage, theRes->mPath);
	}

	ImageFont *anImageFont = dynamic_cast<ImageFont *>(aFont);
	if (anImageFont)
	{
		if (!anImageFont->mFontData || !anImageFont->mFontData->mInitialized)
		{
			delete aFont;
			return Fail(StrFormat("Failed to load font: %s", theRes->mPath.c_str()));
		}

		if (!theRes->mTags.empty())
		{
			char aBuf[1024];
			strcpy(aBuf, theRes->mTags.c_str());
			const char *aPtr = strtok(aBuf, ", \n\t");
			while (aPtr)
			{
				anImageFont->AddTag(aPtr);
				aPtr = strtok(NULL, ", \n\t");
			}
			anImageFont->Prepare();
		}
	}

	theRes->mFont = aFont;

	PERF_END("ResourceManager:DoLoadFont");

	ResourceLoadedHook(theRes);
	return true;
}

bool ResourceManager::DoLoadResource(BaseRes *theRes, bool *fromProgram)
{
	if (theRes->mFromProgram)
	{
		*fromProgram = true;
		return true;
	}

	bool result;

	switch (theRes->mType)
	{
	case ResType_Image:
		PERF_BEGIN("ResourceManager::DoLoadResource(ResType_Image)");
		result = DoLoadImage((ImageRes *)theRes);
		PERF_END("ResourceManager::DoLoadResource(ResType_Image)");
		break;
	case ResType_Sound:
		PERF_BEGIN("ResourceManager::DoLoadResource(ResType_Sound)");
		result = DoLoadSound((SoundRes *)theRes);
		PERF_END("ResourceManager::DoLoadResource(ResType_Sound)");
		break;
	case ResType_Font:
		PERF_BEGIN("ResourceManager::DoLoadResource(ResType_Font)");
		result = DoLoadFont((FontRes *)theRes);
		PERF_END("ResourceManager::DoLoadResource(ResType_Font)");
		break;
	default:
		result = false;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Font *ResourceManager::LoadFont(const std::string &theName)
{
	ResMap::iterator anItr = mFontMap.find(theName);
	if (anItr == mFontMap.end())
		return NULL;

	FontRes *aRes = (FontRes *)anItr->second;
	if (aRes->mFont != NULL)
		return aRes->mFont;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadFont(aRes))
		return NULL;

	return aRes->mFont;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::DeleteFont(const std::string &theName)
{
	ReplaceFont(theName, NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::LoadNextResource()
{
	if (HadError())
		return false;

	if (!mCurResGroupList)
		return false;

	while (mCurResGroupListItr != mCurResGroupList->end())
	{
		BaseRes *aRes = *mCurResGroupListItr++;
		if (aRes->mFromProgram)
			continue;

		switch (aRes->mType)
		{
		case ResType_Image: {
			ImageRes *anImageRes = (ImageRes *)aRes;
			if ((SDLImage *)anImageRes->mImage != NULL)
				continue;

			return DoLoadImage(anImageRes);
		}

		case ResType_Sound: {
			SoundRes *aSoundRes = (SoundRes *)aRes;
			if (aSoundRes->mSoundId != -1)
				continue;

			return DoLoadSound(aSoundRes);
		}

		case ResType_Font: {
			FontRes *aFontRes = (FontRes *)aRes;
			if (aFontRes->mFont != NULL)
				continue;

			return DoLoadFont(aFontRes);
		}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::ResourceLoadedHook(BaseRes *theRes)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::StartLoadResources(const std::string &theGroup)
{
	mError = "";
	mHasFailed = false;

	mCurResGroup = theGroup;
	mCurResGroupList = &mResGroupMap[theGroup];
	mCurResGroupListItr = mCurResGroupList->begin();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void ResourceManager::DumpCurResGroup(std::string &theDestStr)
{
	const ResList *rl = &mResGroupMap.find(mCurResGroup)->second;
	ResList::const_iterator it = rl->begin();
	theDestStr =
		StrFormat("About to dump %d elements from current res group name %s\n", rl->size(), mCurResGroup.c_str());

	ResList::const_iterator rl_end = rl->end();
	while (it != rl_end)
	{
		BaseRes *br = *it++;
		std::string prefix = StrFormat("%s: %s\n", br->mId.c_str(), br->mPath.c_str());
		theDestStr += prefix;
		if (br->mFromProgram)
			theDestStr += std::string("     res is from program\n");
		else if (br->mType == ResType_Image)
			theDestStr += std::string("     res is an image\n");
		else if (br->mType == ResType_Sound)
			theDestStr += std::string("     res is a sound\n");
		else if (br->mType == ResType_Font)
			theDestStr += std::string("     res is a font\n");

		if (it == mCurResGroupListItr)
			theDestStr += std::string("iterator has reached mCurResGroupItr\n");
	}

	theDestStr += std::string("Done dumping resources\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::LoadResources(const std::string &theGroup)
{
	mError = "";
	mHasFailed = false;
	StartLoadResources(theGroup);
	while (LoadNextResource())
	{
	}

	if (!HadError())
	{
		mLoadedGroups.insert(theGroup);
		return true;
	}
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetNumResources(const std::string &theGroup, ResMap &theMap)
{
	if (theGroup.empty())
		return theMap.size();

	int aCount = 0;
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
	{
		BaseRes *aRes = anItr->second;
		if (aRes->mResGroup == theGroup && !aRes->mFromProgram)
			++aCount;
	}

	return aCount;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetNumImages(const std::string &theGroup)
{
	return GetNumResources(theGroup, mImageMap);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetNumSounds(const std::string &theGroup)
{
	return GetNumResources(theGroup, mSoundMap);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetNumFonts(const std::string &theGroup)
{
	return GetNumResources(theGroup, mFontMap);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetNumResources(const std::string &theGroup)
{
	return GetNumImages(theGroup) + GetNumSounds(theGroup) + GetNumFonts(theGroup);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SharedImageRef ResourceManager::GetImage(const std::string &theId)
{
	ResMap::iterator anItr = mImageMap.find(theId);
	if (anItr != mImageMap.end())
		return ((ImageRes *)anItr->second)->mImage;
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetSound(const std::string &theId)
{
	ResMap::iterator anItr = mSoundMap.find(theId);
	if (anItr != mSoundMap.end())
		return ((SoundRes *)anItr->second)->mSoundId;
	else
		return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Font *ResourceManager::GetFont(const std::string &theId)
{
	ResMap::iterator anItr = mFontMap.find(theId);
	if (anItr != mFontMap.end())
		return ((FontRes *)anItr->second)->mFont;
	else
		return NULL;
}

ResourceManager::BaseRes *ResourceManager::GetBaseRes(int type, const std::string &theId)
{
	switch (type)
	{
	case ResType_Image: {
		ResMap::iterator anItr = mImageMap.find(theId);
		if (anItr != mImageMap.end())
			return (ImageRes *)anItr->second;
		else
			return NULL;
	}
	case ResType_Sound: {
		ResMap::iterator anItr = mSoundMap.find(theId);
		if (anItr != mSoundMap.end())
			return (SoundRes *)anItr->second;
		else
			return NULL;
	}
	case ResType_Font: {
		ResMap::iterator anItr = mFontMap.find(theId);
		if (anItr != mFontMap.end())
			return (FontRes *)anItr->second;
		else
			return NULL;
	}
	}

	return NULL;
}

ResourceManager::ResourceRef *ResourceManager::GetFontRef(const std::string &theId)
{
	return GetResourceRef(2, theId);
}

ResourceManager::ResourceRef *ResourceManager::GetResourceRef(int type, const std::string &theId)
{
	BaseRes *base = GetBaseRes(type, theId);
	if (!base)
		return new ResourceRef();

	return GetResourceRef(base);
}

ResourceManager::ResourceRef *ResourceManager::GetResourceRef(BaseRes *base)
{
	ResourceRef *ref = new ResourceRef();

	bool fromProgram;
	if (base->mRefCount == 0)
		DoLoadResource(base, &fromProgram);

	ref->mBaseResP = base;
	base->mRefCount++;
	return ref;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SharedImageRef ResourceManager::GetImageThrow(const std::string &theId)
{
	ResMap::iterator anItr = mImageMap.find(theId);
	if (anItr != mImageMap.end())
	{
		ImageRes *aRes = (ImageRes *)anItr->second;
		if ((MemoryImage *)aRes->mImage != NULL)
			return aRes->mImage;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("Image resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int ResourceManager::GetSoundThrow(const std::string &theId)
{
	ResMap::iterator anItr = mSoundMap.find(theId);
	if (anItr != mSoundMap.end())
	{
		SoundRes *aRes = (SoundRes *)anItr->second;
		if (aRes->mSoundId != -1)
			return aRes->mSoundId;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return -1;
	}

	Fail(StrFormat("Sound resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Font *ResourceManager::GetFontThrow(const std::string &theId)
{
	ResMap::iterator anItr = mFontMap.find(theId);
	if (anItr != mFontMap.end())
	{
		FontRes *aRes = (FontRes *)anItr->second;
		if (aRes->mFont != NULL)
			return aRes->mFont;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("Font resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::SetAllowMissingProgramImages(bool allow)
{
	mAllowMissingProgramResources = allow;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ReplaceImage(const std::string &theId, Image *theImage)
{
	ResMap::iterator anItr = mImageMap.find(theId);
	if (anItr != mImageMap.end())
	{
		anItr->second->DeleteResource();
		((ImageRes *)anItr->second)->mImage = (MemoryImage *)theImage;
		((ImageRes *)anItr->second)->mImage.mOwnsUnshared = true;
		return true;
	}
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ReplaceSound(const std::string &theId, int theSound)
{
	ResMap::iterator anItr = mSoundMap.find(theId);
	if (anItr != mSoundMap.end())
	{
		anItr->second->DeleteResource();
		((SoundRes *)anItr->second)->mSoundId = theSound;
		return true;
	}
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::ReplaceFont(const std::string &theId, Font *theFont)
{
	ResMap::iterator anItr = mFontMap.find(theId);
	if (anItr != mFontMap.end())
	{
		anItr->second->DeleteResource();
		((FontRes *)anItr->second)->mFont = theFont;
		return true;
	}
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const XMLParamMap &ResourceManager::GetImageAttributes(const std::string &theId)
{
	static XMLParamMap aStrMap;

	ResMap::iterator anItr = mImageMap.find(theId);
	if (anItr != mImageMap.end())
		return anItr->second->mXMLAttributes;
	else
		return aStrMap;
}
