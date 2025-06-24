#ifndef __TRIVERTEX_HPP__
#define __TRIVERTEX_HPP__
#ifdef _WIN32
#pragma once
#endif

#include <cstdint>

namespace PopLib
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class TriVertex
{
  public:
	float x, y, u, v;
	uint32_t color; // ARGB (0 = use color specified in function call)

  public:
	TriVertex()
	{
		color = 0;
	}
	TriVertex(float theX, float theY) : x(theX), y(theY)
	{
		color = 0;
	}
	TriVertex(float theX, float theY, float theU, float theV) : x(theX), y(theY), u(theU), v(theV)
	{
		color = 0;
	}
	TriVertex(float theX, float theY, float theU, float theV, uint32_t theColor)
		: x(theX), y(theY), u(theU), v(theV), color(theColor)
	{
	}
};

} // namespace PopLib

#endif
