#include "font.hpp"
#include "image.hpp"

using namespace PopLib;

Font::Font()
{
	mAscent = 0;
	mHeight = 0;
	mAscentPadding = 0;
	mLineSpacingOffset = 0;
}

Font::Font(const Font &theFont)
	: mAscent(theFont.mAscent), mHeight(theFont.mHeight), mAscentPadding(theFont.mAscentPadding),
	  mLineSpacingOffset(theFont.mLineSpacingOffset)
{
}

Font::~Font()
{
}

int Font::GetAscent()
{
	return mAscent;
}

int Font::GetAscentPadding()
{
	return mAscentPadding;
}

int Font::GetDescent()
{
	return mHeight - mAscent;
}

int Font::GetHeight()
{
	return mHeight;
}

int Font::GetLineSpacingOffset()
{
	return mLineSpacingOffset;
}

int Font::GetLineSpacing()
{
	return mHeight + mLineSpacingOffset;
}

int Font::StringWidth(const PopString &theString)
{
	return 0;
}

int Font::CharWidth(PopChar theChar)
{
	PopString aString(1, theChar);
	return StringWidth(aString);
}

int Font::CharWidthKern(PopChar theChar, PopChar thePrevChar)
{
	return CharWidth(theChar);
}

void Font::DrawString(Graphics *g, int theX, int theY, const PopString &theString, const Color &theColor,
					  const Rect &theClipRect)
{
}
