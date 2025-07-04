#ifndef __SOUNDINSTANCE_HPP__
#define __SOUNDINSTANCE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"

namespace PopLib
{
class SoundInstance
{
  public:
	virtual ~SoundInstance() = default;
	virtual void Release() = 0;

	virtual void SetBaseVolume(double theBaseVolume) = 0;
	virtual void SetBasePan(int theBasePan) = 0;

	virtual void AdjustPitch(double theNumSteps) = 0;

	virtual void SetVolume(double theVolume) = 0;
	virtual void SetPan(int thePosition) = 0; // -hundredth db to +hundredth db = left to right

	virtual bool Play(bool looping, bool autoRelease) = 0;
	virtual void Stop() = 0;
	virtual bool IsPlaying() = 0;
	virtual bool IsReleased() = 0;
	virtual double GetVolume() = 0;
};

} // namespace PopLib

#endif