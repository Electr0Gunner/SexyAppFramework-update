#ifndef __RENDERUTILS_HPP__
#define __RENDERUTILS_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "renderer.hpp"

namespace PopLib
{
	static const int MAX_TEXTURE_SIZE = 1024;

	static int gMinTextureWidth;
	static int gMinTextureHeight;
	static int gMaxTextureWidth;
	static int gMaxTextureHeight;
	static int gMaxTextureAspectRatio;
	static ulong gSupportedPixelFormats;
	static bool gTextureSizeMustBePow2;

	static int GetClosestPowerOf2Above(int theNum)
	{
		int aPower2 = 1;
		while (aPower2 < theNum)
			aPower2<<=1;

		return aPower2;
	}


	static bool IsPowerOf2(int theNum)
	{
		int aNumBits = 0;
		while (theNum>0)
		{
			aNumBits += theNum&1;
			theNum >>= 1;
		}

		return aNumBits==1;
	}

	static void GetBestTextureDimensions(int &theWidth, int &theHeight, bool isEdge, bool usePow2, ulong theImageFlags)
	{
	//	theImageFlags = Flag_MinimizeNumSubdivisions;
		if (theImageFlags & Flag_Use64By64Subdivisions)
		{
			theWidth = theHeight = 64;
			return;
		}

		static int aGoodTextureSize[MAX_TEXTURE_SIZE];
		static bool haveInited = false;
		if (!haveInited)
		{
			haveInited = true;
			int i;
			int aPow2 = 1;
			for (i=0; i<MAX_TEXTURE_SIZE; i++)
			{
				if (i > aPow2)
					aPow2 <<= 1;

				int aGoodValue = aPow2;
				if ((aGoodValue - i ) > 64)
				{
					aGoodValue >>= 1;
					while (true)
					{
						int aLeftOver = i % aGoodValue;
						if (aLeftOver<64 || IsPowerOf2(aLeftOver))
							break;

						aGoodValue >>= 1;
					}
				}
				aGoodTextureSize[i] = aGoodValue;
			}
		}

		int aWidth = theWidth;
		int aHeight = theHeight;

		if (usePow2)
		{
			if (isEdge || (theImageFlags & Flag_MinimizeNumSubdivisions))
			{
				aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : GetClosestPowerOf2Above(aWidth);
				aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : GetClosestPowerOf2Above(aHeight);
			}
			else
			{
				aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : aGoodTextureSize[aWidth];
				aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : aGoodTextureSize[aHeight];
			}
		}

		if (aWidth < gMinTextureWidth)
			aWidth = gMinTextureWidth;

		if (aHeight < gMinTextureHeight)
			aHeight = gMinTextureHeight;

		if (aWidth > aHeight)
		{
			while (aWidth > gMaxTextureAspectRatio*aHeight)
				aHeight <<= 1;
		}
		else if (aHeight > aWidth)
		{
			while (aHeight > gMaxTextureAspectRatio*aWidth)
				aWidth <<= 1;
		}

		theWidth = aWidth;
		theHeight = aHeight;
	}

}


#endif // __RENDERUTILS_HPP__