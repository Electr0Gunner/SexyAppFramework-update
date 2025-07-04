#ifndef __BASSMUSICINTERFACE_HPP__
#define __BASSMUSICINTERFACE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "musicinterface.hpp"
#include "bass.h"

namespace PopLib
{

class AppBase;

struct StreamData
{
	/// @brief data that is streamed to mHStream
	uchar* mStreamData;
	/// @brief stream object
	HSTREAM mHStream;
};

/**
 * @brief bass music info
 *
 * why is this not a struct?
 */
class BassMusicInfo
{
  public:
	/// @brief music object
	HMUSIC mHMusic;
	/// @brief stream object
	StreamData mStream;
	/// @brief current volume
	double mVolume;
	/// @brief TBA
	double mVolumeAdd;
	/// @brief TBA
	double mVolumeCap;
	/// @brief true if going to stop on fade
	bool mStopOnFade;

  public:
	/// @brief constructor
	BassMusicInfo();
	/// @brief destructor
	virtual ~BassMusicInfo() = default;

	/// @brief gets the current handle
	/// @return mHMusic if no mHStream and vice verse
	uint32_t GetHandle()
	{
		return mHMusic ? mHMusic : mStream.mHStream;
	}
};

/// @brief list
typedef std::map<int, BassMusicInfo> BassMusicMap;

/**
 * @brief bass music interface
 *
 * parents MusicInterface
 */
class BassMusicInterface : public MusicInterface
{
  public:
	/// @brief list
	BassMusicMap mMusicMap;
	/// @brief maximum music volume
	int mMaxMusicVolume;
	/// @brief music loading flags
	int mMusicLoadFlags;

  public:
	/// @brief constructor
	BassMusicInterface();
	/// @brief destructor
	virtual ~BassMusicInterface();

	/// @brief loads music by id
	/// @param theSongId 
	/// @param theFileName 
	/// @return true if success
	virtual bool LoadMusic(int theSongId, const std::string &theFileName);
	/// @brief plays music by id
	/// @param theSongId 
	/// @param theOffset 
	/// @param noLoop 
	virtual void PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);
	/// @brief stops music by id
	/// @param theSongId 
	virtual void StopMusic(int theSongId);
	/// @brief stops all music
	virtual void StopAllMusic();
	/// @brief unloads music by id
	/// @param theSongId 
	virtual void UnloadMusic(int theSongId);
	/// @brief unloads all music
	virtual void UnloadAllMusic();
	/// @brief pause all music
	virtual void PauseAllMusic();
	/// @brief resume all music
	virtual void ResumeAllMusic();
	/// @brief pauses music by id
	/// @param theSongId 
	virtual void PauseMusic(int theSongId);
	/// @brief resumes music by id
	/// @param theSongId 
	virtual void ResumeMusic(int theSongId);
	/// @brief fades in music by id
	/// @param theSongId 
	/// @param theOffset 
	/// @param theSpeed 
	/// @param noLoop 
	virtual void FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	/// @brief fades out music by id
	/// @param theSongId 
	/// @param stopSong 
	/// @param theSpeed 
	virtual void FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	/// @brief fades out all music
	/// @param stopSong 
	/// @param theSpeed 
	virtual void FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	/// @brief sets song volume by id
	/// @param theSongId 
	/// @param theVolume 
	virtual void SetSongVolume(int theSongId, double theVolume);
	/// @brief sets song maximum volume by id
	/// @param theSongId 
	/// @param theMaxVolume 
	virtual void SetSongMaxVolume(int theSongId, double theMaxVolume);
	/// @brief is song by id playing?
	/// @param theSongId 
	/// @return true if yes 
	virtual bool IsPlaying(int theSongId);

	/// @brief sets global volume
	/// @param theVolume 
	virtual void SetVolume(double theVolume);
	/// @brief sets music amplify, default is 0.50
	/// @param theSongId 
	/// @param theAmp 
	virtual void SetMusicAmplify(int theSongId, double theAmp);
	/// @brief music update
	virtual void Update();

	/// @brief functions for dealing with MODs
	/// @param theSongId 
	/// @return music order
	int GetMusicOrder(int theSongId);
};

} // namespace PopLib

#endif