#include "res.hpp"
#include "PopLib/resources/resourcemanager.hpp"
#include <cstdint>

using namespace PopLib;

#pragma warning(disable : 4311 4312)

static bool gNeedRecalcVariableToIdMap = false;

bool PopLib::ExtractResourcesByName(ResourceManager *theManager, const char *theName)
{
	if (strcmp(theName, "Game") == 0)
		return ExtractGameResources(theManager);
	if (strcmp(theName, "Init") == 0)
		return ExtractInitResources(theManager);
	if (strcmp(theName, "TitleScreen") == 0)
		return ExtractTitleScreenResources(theManager);
	return false;
}

PopLib::ResourceId PopLib::GetIdByStringId(const char *theStringId)
{
	typedef std::map<std::string, int> MyMap;
	static MyMap aMap;
	if (aMap.empty())
	{
		for (int i = 0; i < RESOURCE_ID_MAX; i++)
			aMap[GetStringIdById(i)] = i;
	}

	MyMap::iterator anItr = aMap.find(theStringId);
	if (anItr == aMap.end())
		return RESOURCE_ID_MAX;
	else
		return (ResourceId)anItr->second;
}

// Game Resources
Image *PopLib::IMAGE_TESTPIXEL;
Image *PopLib::IMAGE_BRICK;

bool PopLib::ExtractGameResources(ResourceManager *theManager)
{
	gNeedRecalcVariableToIdMap = true;

	ResourceManager &aMgr = *theManager;
	try
	{
		IMAGE_TESTPIXEL = aMgr.GetImageThrow("IMAGE_TESTPIXEL");
		IMAGE_BRICK = aMgr.GetImageThrow("IMAGE_BRICK");
	}
	catch (ResourceManagerException &)
	{
		return false;
	}
	return true;
}

// Init Resources
Font *PopLib::FONT_DEFAULT;
Font *PopLib::FONT_HUNGARR;
Image *PopLib::IMAGE_CUSTOM_DRAGGING;
Image *PopLib::IMAGE_CUSTOM_HAND;
Image *PopLib::IMAGE_CUSTOM_POINTER;
Image *PopLib::IMAGE_CUSTOM_TEXT;
Image *PopLib::IMAGE_HUNGARR_LOGO;

bool PopLib::ExtractInitResources(ResourceManager *theManager)
{
	gNeedRecalcVariableToIdMap = true;

	ResourceManager &aMgr = *theManager;
	try
	{
		FONT_DEFAULT = aMgr.GetFontThrow("FONT_DEFAULT");
		FONT_HUNGARR = aMgr.GetFontThrow("FONT_HUNGARR");
		IMAGE_CUSTOM_DRAGGING = aMgr.GetImageThrow("IMAGE_CUSTOM_DRAGGING");
		IMAGE_CUSTOM_HAND = aMgr.GetImageThrow("IMAGE_CUSTOM_HAND");
		IMAGE_CUSTOM_POINTER = aMgr.GetImageThrow("IMAGE_CUSTOM_POINTER");
		IMAGE_CUSTOM_TEXT = aMgr.GetImageThrow("IMAGE_CUSTOM_TEXT");
		IMAGE_HUNGARR_LOGO = aMgr.GetImageThrow("IMAGE_HUNGARR_LOGO");
	}
	catch (ResourceManagerException &)
	{
		return false;
	}
	return true;
}

// TitleScreen Resources
Image *PopLib::IMAGE_LOADER_BAR;
Image *PopLib::IMAGE_LOADER_LOADINGTXT;

bool PopLib::ExtractTitleScreenResources(ResourceManager *theManager)
{
	gNeedRecalcVariableToIdMap = true;

	ResourceManager &aMgr = *theManager;
	try
	{
		IMAGE_LOADER_BAR = aMgr.GetImageThrow("IMAGE_LOADER_BAR");
		IMAGE_LOADER_LOADINGTXT = aMgr.GetImageThrow("IMAGE_LOADER_LOADINGTXT");
	}
	catch (ResourceManagerException &)
	{
		return false;
	}
	return true;
}

static void *gResources[] = {&FONT_DEFAULT,
							 &FONT_HUNGARR,
							 &IMAGE_CUSTOM_POINTER,
							 &IMAGE_CUSTOM_HAND,
							 &IMAGE_CUSTOM_DRAGGING,
							 &IMAGE_CUSTOM_TEXT,
							 &IMAGE_HUNGARR_LOGO,
							 IMAGE_TESTPIXEL,
							 IMAGE_BRICK,
							 NULL};

Image *PopLib::LoadImageById(ResourceManager *theManager, int theId)
{
	return (*((Image **)gResources[theId]) = theManager->LoadImage(GetStringIdById(theId)));
}

Image *PopLib::GetImageById(int theId)
{
	return *(Image **)gResources[theId];
}

Font *PopLib::GetFontById(int theId)
{
	return *(Font **)gResources[theId];
}

int PopLib::GetSoundById(int theId)
{
	return *(int *)gResources[theId];
}

static PopLib::ResourceId GetIdByVariable(const void *theVariable)
{
	typedef std::map<int, int> MyMap;
	static MyMap aMap;
	if (gNeedRecalcVariableToIdMap)
	{
		gNeedRecalcVariableToIdMap = false;
		aMap.clear();
		for (int i = 0; i < RESOURCE_ID_MAX; i++)
			aMap[*(int *)gResources[i]] = i;
	}

	MyMap::iterator anItr = aMap.find((uintptr_t)theVariable);
	if (anItr == aMap.end())
		return RESOURCE_ID_MAX;
	else
		return (ResourceId)anItr->second;
}

PopLib::ResourceId PopLib::GetIdByImage(Image *theImage)
{
	return GetIdByVariable(theImage);
}

PopLib::ResourceId PopLib::GetIdByFont(Font *theFont)
{
	return GetIdByVariable(theFont);
}

PopLib::ResourceId PopLib::GetIdBySound(int theSound)
{
	return GetIdByVariable((void *)(uintptr_t)theSound);
}

const char *PopLib::GetStringIdById(int theId)
{
	switch (theId)
	{
	case FONT_DEFAULT_ID:
		return "FONT_DEFAULT";
	case FONT_HUNGARR_ID:
		return "FONT_HUNGARR";
	case IMAGE_CUSTOM_POINTER_ID:
		return "IMAGE_CUSTOM_POINTER";
	case IMAGE_CUSTOM_HAND_ID:
		return "IMAGE_CUSTOM_HAND";
	case IMAGE_CUSTOM_DRAGGING_ID:
		return "IMAGE_CUSTOM_DRAGGING";
	case IMAGE_CUSTOM_TEXT_ID:
		return "IMAGE_CUSTOM_TEXT";
	case IMAGE_HUNGARR_LOGO_ID:
		return "IMAGE_HUNGARR_LOGO";
	case IMAGE_TESTPIXEL_ID:
		return "IMAGE_TESTPIXEL";
	case IMAGE_BRICK_ID:
		return "IMAGE_BRICK";
	default:
		return "";
	}
}
