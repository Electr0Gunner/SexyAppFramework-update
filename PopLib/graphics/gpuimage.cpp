#include "gpuimage.hpp"
#include "misc/critsect.hpp"
#include "debug/debug.hpp"
#include "sdlinterface.hpp"
#include "appbase.hpp"
#include "image.hpp"

using namespace PopLib;

GPUImage::GPUImage() : Image()
{
    mApp = gAppBase;
	mRenderer = gAppBase->mRenderer;
	mRenderer->AddGPUImage(this);
}

GPUImage::GPUImage(Renderer *theRenderer)
{
    mApp = theRenderer->mApp;
	mRenderer = theRenderer;
	mRenderer->AddGPUImage(this);
}

GPUImage::GPUImage(const GPUImage &theImage):  mHasAlpha(theImage.mHasAlpha),
	  mHasTrans(theImage.mHasTrans),mIsVolatile(theImage.mIsVolatile), 
      mPurgeBits(theImage.mPurgeBits), mWantPal(theImage.mWantPal),
      mApp(theImage.mApp), mRenderer(theImage.mRenderer), mTextureData(theImage.mTextureData)
{
	mRenderer->AddGPUImage(this);
}

GPUImage::~GPUImage()
{
	mRenderer->RemoveGPUImage(this);
}

void GPUImage::Create(int theWidth, int theHeight)
{
	mWidth = theWidth;
	mHeight = theHeight;

	mHasTrans = true;
	mHasAlpha = true;
}

bool GPUImage::PolyFill3D(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor,
						  int theDrawMode, int tx, int ty)
{
	//mInterface->FillPoly(theVertices, theNumVertices, theClipRect, theColor, theDrawMode, tx, ty);
	return true;
}

void GPUImage::FillRect(const Rect &theRect, const Color &theColor, int theDrawMode)
{
	//mInterface->FillRect(theRect, theColor, theDrawMode);
}

void GPUImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
								 const Color &theColor, int theDrawMode)
{
	//mInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void GPUImage::DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY,
								   const Color &theColor, int theDrawMode)
{
	//mInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void GPUImage::Blt(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor,
							int theDrawMode)
{
	theImage->mDrawn = true;

	//CommitBits();

	//mInterface->Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void GPUImage::BltF(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Rect &theClipRect,
							 const Color &theColor, int theDrawMode)
{
	theImage->mDrawn = true;

	FRect aClipRect(theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight);
	FRect aDestRect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight);

	FRect anIntersect = aDestRect.Intersection(aClipRect);
	if (anIntersect.mWidth != aDestRect.mWidth || anIntersect.mHeight != aDestRect.mHeight)
	{
	//	if (anIntersect.mWidth != 0 && anIntersect.mHeight != 0)
		//	mInterface->BltClipF(theImage, theX, theY, theSrcRect, &theClipRect, theColor, theDrawMode);
	}
	//else
	//	mInterface->Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode, true);
}

void GPUImage::BltRotated(Image *theImage, float theX, float theY, const Rect &theSrcRect,
								   const Rect &theClipRect, const Color &theColor, int theDrawMode, double theRot,
								   float theRotCenterX, float theRotCenterY)
{
	theImage->mDrawn = true;

	//CommitBits();

	//mInterface->BltRotated(theImage, theX, theY, &theClipRect, theColor, theDrawMode, theRot, theRotCenterX,
						//   theRotCenterY, theSrcRect);
}

void GPUImage::StretchBlt(Image *theImage, const Rect &theDestRect, const Rect &theSrcRect,
								   const Rect &theClipRect, const Color &theColor, int theDrawMode, bool fastStretch)
{
	theImage->mDrawn = true;

	//CommitBits();

	//mInterface->StretchBlt(theImage, theDestRect, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch);
}

void GPUImage::BltMatrix(Image *theImage, float x, float y, const Matrix3 &theMatrix,
								  const Rect &theClipRect, const Color &theColor, int theDrawMode,
								  const Rect &theSrcRect, bool blend)
{
	theImage->mDrawn = true;

	//mInterface->BltTransformed(theImage, &theClipRect, theColor, theDrawMode, theSrcRect, theMatrix, blend, x, y, true);
}

void GPUImage::BltTrianglesTex(Image *theTexture, const TriVertex theVertices[][3], int theNumTriangles,
										const Rect &theClipRect, const Color &theColor, int theDrawMode, float tx,
										float ty, bool blend)
{
	theTexture->mDrawn = true;

	//mInterface->DrawTrianglesTex(theVertices, theNumTriangles, theColor, theDrawMode, theTexture, tx, ty, blend);
}

void GPUImage::BltMirror(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor,
								  int theDrawMode)
{
	theImage->mDrawn = true;

//	CommitBits();

	//mInterface->BltMirror(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void GPUImage::StretchBltMirror(Image *theImage, const Rect &theDestRectOrig, const Rect &theSrcRect,
										 const Rect &theClipRect, const Color &theColor, int theDrawMode,
										 bool fastStretch)
{
	theImage->mDrawn = true;

	//CommitBits();

	//mInterface->StretchBlt(theImage, theDestRectOrig, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch,
						 //  true);
}

void GPUImage::FillScanLinesWithCoverage(Span *theSpans, int theSpanCount, const Color &theColor, int theDrawMode,
										 const BYTE *theCoverage, int theCoverX, int theCoverY, int theCoverWidth,
										 int theCoverHeight)
{
	if (theSpanCount == 0)
		return;

	int l = theSpans[0].mX, t = theSpans[0].mY;
	int r = l + theSpans[0].mWidth, b = t;
	for (int i = 1; i < theSpanCount; ++i)
	{
		l = std::min(theSpans[i].mX, l);
		r = std::max(theSpans[i].mX + theSpans[i].mWidth - 1, r);
		t = std::min(theSpans[i].mY, t);
		b = std::max(theSpans[i].mY + theSpans[i].mWidth - 1, b);
	}
	for (int i = 0; i < theSpanCount; ++i)
	{
		theSpans[i].mX -= l;
		theSpans[i].mY -= t;
	}

	MemoryImage aTempImage;
	aTempImage.Create(r - l + 1, b - t + 1);
	aTempImage.FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX - l,
										 theCoverY - t, theCoverWidth, theCoverHeight);
	Blt(&aTempImage, l, t, Rect(0, 0, r - l + 1, b - t + 1), Color::White, theDrawMode);
	return;
}

bool GPUImage::Check3D(GPUImage *theImage)
{
	return true;
}

bool GPUImage::Check3D(Image *theImage)
{
	GPUImage *anImage = dynamic_cast<GPUImage *>(theImage);
	return anImage != nullptr;
}

void GPUImage::PurgeBits()
{
	mPurgeBits = true;

	//CommitBits();
	//GetBits();

	//MemoryImage::PurgeBits();
}
