#ifndef __OPENALSOUNDMANAGER_HPP__
#define __OPENALSOUNDMANAGER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "soundmanager.hpp"
#include "bass.h"

#ifndef AL_LIBTYPE_STATIC
#define AL_LIBTYPE_STATIC
#endif
#include <AL/al.h>
#include <AL/alc.h>

namespace PopLib
{
class OpenALSoundInstance;

class OpenALSoundManager : public SoundManager
{
	friend class OpenALSoundInstance;
	friend class BassMusicInterface;

  public:
	ALuint mSourceSounds[MAX_SOURCE_SOUNDS];
	std::string mSourceFileNames[MAX_SOURCE_SOUNDS];
	ulong mSourceDataSizes[MAX_SOURCE_SOUNDS];
	double mBaseVolumes[MAX_SOURCE_SOUNDS];
	int mBasePans[MAX_SOURCE_SOUNDS];
	OpenALSoundInstance *mPlayingSounds[MAX_CHANNELS];
	double mMasterVolume;
	uint32_t mLastReleaseTick;

	// hack
	ALCdevice *mALDeviceD;

	int FindFreeChannel();
	int VolumeToDB(double theVolume);
	void ReleaseFreeChannels();

	OpenALSoundManager();
	virtual ~OpenALSoundManager();

	virtual bool Initialized();

	virtual bool LoadSound(unsigned int theSfxID, const std::string &theFilename);
	virtual int LoadSound(const std::string &theFilename);
	virtual bool LoadOGGSound(unsigned int theSfxID, const std::string &theFilename);
	virtual bool LoadAUSound(unsigned int theSfxID, const std::string &theFilename);
	virtual void ReleaseSound(unsigned int theSfxID);

	virtual void SetVolume(double theVolume);
	virtual bool SetBaseVolume(unsigned int theSfxID, double theBaseVolume);
	virtual bool SetBasePan(unsigned int theSfxID, int theBasePan);

	virtual SoundInstance *GetSoundInstance(unsigned int theSfxID);

	virtual void ReleaseSounds();
	virtual void ReleaseChannels();

	virtual double GetMasterVolume();
	virtual void SetMasterVolume(double theVolume);

	virtual void Flush();

	virtual void SetCooperativeWindow(bool isWindowed);
	virtual void StopAllSounds();
	virtual int GetFreeSoundId();
	virtual int GetNumSounds();
	virtual void ForceReleaseSources(ALuint theBuffer);
};

} // namespace PopLib

#endif