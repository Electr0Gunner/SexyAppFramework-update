#include "sharedimage.hpp"
#include "gpuimage.hpp"
#include "appbase.hpp"

using namespace PopLib;

SharedImage::SharedImage()
{
	mImage = nullptr;
	mRefCount = 0;
}

SharedImageRef::SharedImageRef(const SharedImageRef &theSharedImageRef)
{
	mSharedImage = theSharedImageRef.mSharedImage;
	if (mSharedImage != nullptr)
		mSharedImage->mRefCount++;
	mUnsharedImage = theSharedImageRef.mUnsharedImage;
	mOwnsUnshared = false;
}

SharedImageRef::SharedImageRef()
{
	mSharedImage = nullptr;
	mUnsharedImage = nullptr;
	mOwnsUnshared = false;
}

SharedImageRef::SharedImageRef(SharedImage *theSharedImage)
{
	mSharedImage = theSharedImage;
	if (theSharedImage != nullptr)
		mSharedImage->mRefCount++;

	mUnsharedImage = nullptr;
	mOwnsUnshared = false;
}

SharedImageRef::~SharedImageRef()
{
	Release();
}

void SharedImageRef::Release()
{
	if (mOwnsUnshared)
		delete mUnsharedImage;
	mUnsharedImage = nullptr;
	if (mSharedImage != nullptr)
	{
		if (--mSharedImage->mRefCount == 0)
			gAppBase->mCleanupSharedImages = true;
	}
	mSharedImage = nullptr;
}

SharedImageRef &SharedImageRef::operator=(const SharedImageRef &theSharedImageRef)
{
	Release();
	mSharedImage = theSharedImageRef.mSharedImage;
	if (mSharedImage != nullptr)
		mSharedImage->mRefCount++;
	return *this;
}

SharedImageRef &SharedImageRef::operator=(SharedImage *theSharedImage)
{
	Release();
	mSharedImage = theSharedImage;
	mSharedImage->mRefCount++;
	return *this;
}

SharedImageRef &SharedImageRef::operator=(MemoryImage *theUnsharedImage)
{
	Release();
	mUnsharedImage = theUnsharedImage;
	return *this;
}

MemoryImage *SharedImageRef::operator->()
{
	return (MemoryImage *)*this;
}

SharedImageRef::operator Image *()
{
	return (MemoryImage *)*this;
}

SharedImageRef::operator MemoryImage *()
{
	if (mUnsharedImage != nullptr)
		return mUnsharedImage;
	else
		return (GPUImage *)*this;
}

SharedImageRef::operator GPUImage *()
{
	if (mSharedImage != nullptr)
		return mSharedImage->mImage;
	else
		return nullptr;
}