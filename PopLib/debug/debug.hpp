#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include <cassert>

extern bool gInAssert;

#ifdef TRACING_ENABLED
void Trace(const char *theStr);
#define TRACE(theStr) Trace(theStr)
#else
#define TRACE(theStr)
#endif

void TraceFmt(const PopChar *fmt, ...);
void OutputDebug(const PopChar *fmt, ...);

#ifdef NDEBUG
#define DBG_ASSERTE(exp) ((void)0)
#define DBG_ASSERT(exp) ((void)0)
#else

#define DBG_ASSERTE(exp)                                                                                               \
	{                                                                                                                  \
		gInAssert = true;                                                                                              \
		assert(exp);                                                                                                   \
		gInAssert = false;                                                                                             \
	}
#define DBG_ASSERT(exp)                                                                                                \
	{                                                                                                                  \
		gInAssert = true;                                                                                              \
		assert(exp);                                                                                                   \
		gInAssert = false;                                                                                             \
	}

#endif // NDEBUG

#endif