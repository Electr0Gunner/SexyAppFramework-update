#include "sysfont.hpp"
#include "appbase.hpp"
#include "graphics.hpp"
#include "imagefont.hpp"
#include "interface.hpp"
#include "widget/widgetmanager.hpp"

#include <stdlib.h>

using namespace PopLib;

SysFont::SysFont(const std::string &theFace, int thePointSize, bool bold, bool italics, bool underline)
{
	Init(gAppBase, theFace, thePointSize, 0, bold, italics, underline, false);
}

SysFont::SysFont(AppBase *theApp, const std::string &theFace, int thePointSize, int theScript, bool bold, bool italics,
				 bool underline)
{
	Init(theApp, theFace, thePointSize, theScript, bold, italics, underline, true);
}

SysFont::SysFont(AppBase *theApp, const unsigned char aData[], size_t aDataSize, int thePointSize, int theScript,
				 bool bold, bool italics, bool underline)
{
	mApp = theApp;
	SDL_IOStream *io = SDL_IOFromConstMem((void *)aData, aDataSize);
	if (!io)
	{
		mApp->mInterface->MakeSimpleMessageBox("Failed to create SDL_IOStream", SDL_GetError(),
												  MsgBox_OK);
		return;
	}

	mTTFFont = TTF_OpenFontIO(io, false, thePointSize);
	if (!mTTFFont)
	{
		mApp->mInterface->MakeSimpleMessageBox("Error", SDL_GetError(), MsgBox_OK);
	}

	TTF_SetFontStyle(mTTFFont, (bold ? TTF_STYLE_BOLD : 0) | (italics ? TTF_STYLE_ITALIC : 0) |
								   (underline ? TTF_STYLE_UNDERLINE : 0));

	mAscent = TTF_GetFontAscent(mTTFFont);
	mHeight = TTF_GetFontHeight(mTTFFont);

	mDrawShadow = false;
	mSimulateBold = false;
}

void SysFont::Init(AppBase *theApp, const std::string &theFace, int thePointSize, int theScript, bool bold,
				   bool italics, bool underline, bool useDevCaps)
{
	mApp = theApp;

	mTTFFont = TTF_OpenFont(theFace.c_str(), thePointSize);
	if (!mTTFFont)
	{
		mApp->mInterface->MakeSimpleMessageBox("Error", SDL_GetError(), MsgBox_OK);
	}

	TTF_SetFontStyle(mTTFFont, (bold ? TTF_STYLE_BOLD : 0) | (italics ? TTF_STYLE_ITALIC : 0) |
								   (underline ? TTF_STYLE_UNDERLINE : 0));

	mAscent = TTF_GetFontAscent(mTTFFont);
	mHeight = TTF_GetFontHeight(mTTFFont);

	mDrawShadow = false;
	mSimulateBold = false;
}

SysFont::SysFont(const SysFont &theSysFont)
{
	mTTFFont = theSysFont.mTTFFont;
	mApp = theSysFont.mApp;
	mHeight = theSysFont.mHeight;
	mAscent = theSysFont.mAscent;

	mDrawShadow = false;
	mSimulateBold = false;
}

SysFont::~SysFont()
{
	TTF_CloseFont(mTTFFont);
}

ImageFont *SysFont::CreateImageFont()
{
	/*
	int i;
	MemoryImage*			anImage;
	int anImageCharWidth, anImageXOff, anImageYOff;

	////////////////////////////////////////////////////
	// Step 1: Create image
	anImageCharWidth = CharWidth('W')*2;
	anImageXOff = anImageCharWidth/4;
	anImageYOff = mHeight/2;
	int aWidth = 257*anImageCharWidth;
	int aHeight = mHeight*2;

	BITMAPINFO aBitmapInfo;
	memset(&aBitmapInfo,0,sizeof(aBitmapInfo));
	BITMAPINFOHEADER &aHeader = aBitmapInfo.bmiHeader;
	aHeader.biSize = sizeof(aHeader);
	aHeader.biWidth = aWidth;
	aHeader.biHeight = -aHeight;
	aHeader.biPlanes = 1;
	aHeader.biBitCount = 32;
	aHeader.biCompression = BI_RGB;

	HDC aDC = CreateCompatibleDC(nullptr);

	DWORD *aBits = nullptr;
	HBITMAP aBitmap = CreateDIBSection(aDC,&aBitmapInfo,DIB_RGB_COLORS,(void**)&aBits,nullptr,0);

	HBITMAP anOldBitmap = (HBITMAP)SelectObject(aDC,aBitmap);
	HFONT anOldFont = (HFONT)SelectObject(aDC,mHFont);

	HBRUSH anOldBrush = (HBRUSH)SelectObject(aDC,GetStockObject(BLACK_BRUSH));
	Rectangle(aDC,0,0,aWidth,aHeight);

	SetBkMode(aDC, TRANSPARENT);
	SetTextColor(aDC, RGB(255,255,255));

	int xpos = anImageXOff;
	int ypos = anImageYOff;
	for (i=0; i<256; i++)
	{
		char aChar = i;
		TextOutA(aDC,xpos,ypos,&aChar,1);
		xpos += anImageCharWidth;
	}
	GdiFlush();


	SelectObject(aDC,anOldBrush);
	SelectObject(aDC,anOldBitmap);
	SelectObject(aDC,anOldFont);

	int aSize = aWidth*aHeight;
	anImage = new MemoryImage(mApp);
	anImage->Create(aWidth,aHeight);
	DWORD *src = aBits;
	DWORD *dst = anImage->GetBits();
	for (i=0; i<aSize; i++)
	{
		DWORD anAlpha = ((*src++)&0xff)<<24;
		*dst++ = anAlpha | 0xFFFFFF;
	}
	anImage->BitsChanged();
	DeleteObject(aBitmap);


	////////////////////////////////////////////////////
	// Step 2: Create image font

	ImageFont *aFont = new ImageFont(anImage);
	FontLayer *aFontLayer = &aFont->mFontData->mFontLayerList.back();

	aFontLayer->mAscent = mAscent;
	aFontLayer->mHeight = mHeight;

	for (i=0; i<256; i++)
	{
		char aChar = i;

		aFontLayer->mCharData[(uchar) aChar].mImageRect =
	Rect(aChar*anImageCharWidth,0,anImageCharWidth,anImage->mHeight); aFontLayer->mCharData[(uchar) aChar].mWidth =
	CharWidth(aChar); aFontLayer->mCharData[(uchar) aChar].mOffset = Point(-anImageXOff,-anImageYOff);
	}

	aFont->GenerateActiveFontLayers();
	aFont->mActiveListValid = true;

	return aFont;
	*/
	return nullptr; // TODO: implement
}

int SysFont::StringWidth(const PopString &theString)
{
	int w = 0;
	TTF_GetStringSize(mTTFFont, theString.c_str(), 0, &w, nullptr);

	return w;
}

void SysFont::DrawString(Graphics *g, int theX, int theY, const PopString &theString, const Color &theColor,
						 const Rect &theClipRect)
{
    Interface *interface = mApp->mInterface;

    if (mDrawShadow)
        interface->DrawText(theX + 1, theY + 1 - mAscent, theString, Color(0, 0, 0, theColor.mAlpha), mTTFFont);

    interface->DrawText(theX, theY - mAscent, theString, theColor, mTTFFont);
}

Font *SysFont::Duplicate()
{
	return new SysFont(*this);
}
