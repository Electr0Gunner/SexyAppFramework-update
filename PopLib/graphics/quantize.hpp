#ifndef __QUANTIZE_HPP__
#define __QUANTIZE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"

namespace PopLib
{

bool Quantize8Bit(const ulong *theSrcBits, int theWidth, int theHeight, uchar *theDestColorIndices,
				  ulong *theDestColorTable);

}

#endif // __QUANTIZE_HPP__