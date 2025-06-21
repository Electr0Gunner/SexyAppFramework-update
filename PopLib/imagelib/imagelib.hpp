#ifndef __IMAGELIB_HPP__
#define __IMAGELIB_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include <string>

namespace ImageLib
{

class Image
{
  public:
	int mWidth;
	int mHeight;
	ulong *mBits;
	int mNumChannels;

  public:
	Image();
	virtual ~Image();

	int GetWidth();
	int GetHeight();
	ulong *GetBits();
};

bool WriteImage(const std::string &theFileName, const std::string &theExtension, Image *theImage);
bool WriteImageRaw(const std::string &theFileName, const std::string &theExtension, unsigned char *theData,
				   int theWidth, int theHeight);
extern int gAlphaComposeColor;
extern bool gAutoLoadAlpha;

Image *GetImage(const std::string &theFileName, bool lookForAlphaImage = true);

} // namespace ImageLib

#endif //__IMAGELIB_HPP__