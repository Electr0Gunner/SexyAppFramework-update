#ifndef __MATH_HPP__
#define __MATH_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include <cmath>

// VC6 workaround
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace PopLib
{

class Math
{
  public:
	inline static double Fabs(double x)
	{
		return std::fabs(x);
	}

	inline static float Fabs(float x)
	{
		return (float)std::fabs(x);
	}

	inline static float DegToRad(float degrees)
	{
		return degrees * static_cast<float>(M_PI / 180.0);
	}

	inline static float RadToDeg(float radians)
	{
		return radians * static_cast<float>(180.0 / M_PI);
	}

	inline static bool IsPowerOfTwo(uint32_t x)
	{
		return x != 0 && (x & (x - 1)) == 0;
	}

	inline static float Lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	inline static float SmoothStep(float edge0, float edge1, float x)
	{
		x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return x * x * (3 - 2 * x);
	}

	inline static bool NearlyEqual(float a, float b, float epsilon = 1e-5f)
	{
		return std::fabs(a - b) <= epsilon;
	}

	inline static float Sin(float angleRadians) { return (float)std::sin(angleRadians); }
	inline static float Cos(float angleRadians) { return (float)std::cos(angleRadians); }
	inline static float Tan(float angleRadians) { return (float)std::tan(angleRadians); }
	inline static float Asin(float x) { return (float)std::asin(x); }
	inline static float Acos(float x) { return (float)std::acos(x); }
	inline static float Atan2(float y, float x) { return (float)std::atan2(y, x); }

	template <typename T> int Sign(T val) {
    return (T(0) < val) - (val < T(0));
	}
};

};

#endif