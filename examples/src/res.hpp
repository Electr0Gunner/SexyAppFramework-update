#ifndef __RES_H_
#define __RES_H_
#ifdef _WIN32
#pragma once
#endif

namespace PopLib
{
class ResourceManager;
class Image;
class Font;

Image *LoadImageById(ResourceManager *theManager, int theId);
bool ExtractResourcesByName(ResourceManager *theManager, const char *theName);

// Game Resources
bool ExtractGameResources(ResourceManager *theMgr);
extern Image *IMAGE_TESTPIXEL;
extern Image *IMAGE_BRICK;

// Init Resources
bool ExtractInitResources(ResourceManager *theMgr);
extern Font *FONT_DEFAULT;
extern Font *FONT_HUNGARR;
extern Image *IMAGE_CUSTOM_DRAGGING;
extern Image *IMAGE_CUSTOM_HAND;
extern Image *IMAGE_CUSTOM_POINTER;
extern Image *IMAGE_CUSTOM_TEXT;
extern Image *IMAGE_HUNGARR_LOGO;

// TitleScreen Resources
bool ExtractTitleScreenResources(ResourceManager *theMgr);
extern Image *IMAGE_LOADER_BAR;
extern Image *IMAGE_LOADER_LOADINGTXT;

enum ResourceId
{
	FONT_DEFAULT_ID,
	FONT_HUNGARR_ID,
	IMAGE_CUSTOM_POINTER_ID,
	IMAGE_CUSTOM_HAND_ID,
	IMAGE_CUSTOM_DRAGGING_ID,
	IMAGE_CUSTOM_TEXT_ID,
	IMAGE_HUNGARR_LOGO_ID,
	IMAGE_TESTPIXEL_ID,
	IMAGE_BRICK_ID,
	RESOURCE_ID_MAX
};

Image *GetImageById(int theId);
Font *GetFontById(int theId);
int GetSoundById(int theId);

ResourceId GetIdByImage(Image *theImage);
ResourceId GetIdByFont(Font *theFont);
ResourceId GetIdBySound(int theSound);
const char *GetStringIdById(int theId);
ResourceId GetIdByStringId(const char *theStringId);

} // namespace PopLib

#endif
