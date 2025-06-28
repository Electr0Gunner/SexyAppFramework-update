#ifndef __SHAREDIMAGE_HPP__
#define __SHAREDIMAGE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"

namespace PopLib
{

class Image;
class GPUImage;
class MemoryImage;

class SharedImage
{
  public:
	GPUImage *mImage;
	int mRefCount;

	SharedImage();
};

typedef std::map<std::pair<std::string, std::string>, SharedImage> SharedImageMap;

class SharedImageRef
{
  public:
	SharedImage *mSharedImage;
	MemoryImage *mUnsharedImage;
	bool mOwnsUnshared;

  public:
	SharedImageRef();
	SharedImageRef(const SharedImageRef &theSharedImageRef);
	SharedImageRef(SharedImage *theSharedImage);
	~SharedImageRef();

	void Release();

	SharedImageRef &operator=(const SharedImageRef &theSharedImageRef);
	SharedImageRef &operator=(SharedImage *theSharedImage);
	SharedImageRef &operator=(MemoryImage *theUnsharedImage);
	MemoryImage *operator->();
	operator Image *();
	operator MemoryImage *();
	operator GPUImage *();
};

} // namespace PopLib

#endif