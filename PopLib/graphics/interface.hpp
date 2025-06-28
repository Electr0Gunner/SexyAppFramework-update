#ifndef __INTERFACE_HPP__
#define __INTERFACE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "image.hpp"
#include "gpuimage.hpp"
#include "gpuimage.hpp"
#include "appbase.hpp"
#include <memory>

namespace PopLib
{

enum BlendMode
{
	BLENDMODE_NONE = 0,
	BLENDMODE_BLEND,
	BLENDMODE_BLEND_PREMULTIPLIED,
	BLENDMODE_ADD,
	BLENDMODE_ADD_PREMULTIPLIED,
	BLENDMODE_MOD,
	BLENDMODE_MUL,
	BLENDMODE_LAST,
};

struct MsgBoxData
{
	MsgBoxFlags mFlags;
	const char *mTitle;
	const char *mMessage;
};

struct ImageData
{
	int width;
	int height;
	std::vector<uint8_t> pixels; // RGBA8
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Texture
{
public:
	virtual ~Texture() = default;
};

class Interface
{
  public:
	int mRGBBits;
	ulong mRedMask;
	ulong mGreenMask;
	ulong mBlueMask;
	int mRedBits;
	int mGreenBits;
	int mBlueBits;
	int mRedShift;
	int mGreenShift;
	int mBlueShift;

	enum
	{
		RESULT_OK = 0,
		RESULT_FAIL = 1,
		RESULT_DD_CREATE_FAIL = 2,
		RESULT_SURFACE_FAIL = 3,
		RESULT_EXCLUSIVE_FAIL = 4,
		RESULT_DISPCHANGE_FAIL = 5,
		RESULT_INVALID_COLORDEPTH = 6,
		RESULT_3D_FAIL = 7
	};
	
	AppBase *mApp;
	CritSect mCritSect;
	int mWidth;
	int mHeight;
	int mDisplayWidth;
	int mDisplayHeight;
	int mVideoOnlyDraw;

	bool mIs3D;
	bool mHasInitiated;

	Rect mPresentationRect;
	int mRefreshRate;
	int mMillisecondsPerFrame;

	Image *mCursorImage;
	GPUImage *mScreenImage;

	int mNextCursorX;
	int mNextCursorY;
	int mCursorX;
	int mCursorY;

  public:
	Interface();
	virtual ~Interface();
	virtual void Cleanup() = 0;

	virtual void AddImage(Image *theImage) = 0;
	virtual void RemoveImage(Image *theImage) = 0;
	virtual void Remove3DData(GPUImage *theImage) = 0; // for 3d texture cleanup

	virtual void GetOutputSize(int *outWidth, int *outHeight) = 0;

	virtual GPUImage *NewGPUImage() = 0;

	virtual GPUImage *GetScreenImage() = 0;
	virtual void UpdateViewport() = 0;
	virtual int Init(bool IsWindowed) = 0;

	virtual bool Redraw(Rect *theClipRect) = 0;
	virtual void SetVideoOnlyDraw(bool videoOnly) = 0;

	virtual std::unique_ptr<ImageData> CaptureFrameBuffer() = 0;

	virtual void SetCursorPos(int theCursorX, int theCursorY) = 0;

	virtual bool SetCursorImage(Image *theImage) = 0;
	virtual bool UpdateWindowIcon(Image *theImage) = 0;

	virtual void SetCursor(CursorType theCursorType) = 0;

	virtual void MakeSimpleMessageBox(const char *theTitle, const char *theMessage, MsgBoxFlags flags) = 0;
	virtual int MakeResultMessageBox(MsgBoxData data) = 0;

	virtual void PushTransform(const Matrix3 &theTransform, bool concatenate = true) = 0;
	virtual void PopTransform() = 0;

	virtual bool PreDraw() = 0;

	virtual bool CreateImageTexture(GPUImage *theImage) = 0;
	virtual bool RecoverBits(GPUImage *theImage) = 0;

	virtual BlendMode ChooseBlendMode(int theBlendMode) = 0;
	virtual void DrawText(int theY, int theX, const PopString &theText, const Color &theColor, TTF_Font *theFont) = 0;

	// Draw Funcs
	virtual void Blt(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor, int theDrawMode,
			 bool linearFilter = false) = 0;
	virtual void BltClipF(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Rect *theClipRect,
				  const Color &theColor, int theDrawMode) = 0;
	virtual void BltMirror(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Color &theColor,
				   int theDrawMode, bool linearFilter = false) = 0;
	virtual void StretchBlt(Image *theImage, const Rect &theDestRect, const Rect &theSrcRect, const Rect *theClipRect,
					const Color &theColor, int theDrawMode, bool fastStretch, bool mirror = false) = 0;
	virtual void BltRotated(Image *theImage, float theX, float theY, const Rect *theClipRect, const Color &theColor,
					int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY, const Rect &theSrcRect) = 0;
	virtual void BltTransformed(Image *theImage, const Rect *theClipRect, const Color &theColor, int theDrawMode,
						const Rect &theSrcRect, const Matrix3 &theTransform, bool linearFilter, float theX = 0,
						float theY = 0, bool center = false) = 0;
	virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor,
				  int theDrawMode) = 0;
	virtual void FillRect(const Rect &theRect, const Color &theColor, int theDrawMode) = 0;
	virtual void DrawTriangle(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
					  int theDrawMode) = 0;
	virtual void DrawTriangleTex(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
						 int theDrawMode, Image *theTexture, bool blend = true) = 0;
	virtual void DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor, int theDrawMode,
						  Image *theTexture, float tx = 0, float ty = 0, bool blend = true) = 0;
	virtual void DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor,
							   int theDrawMode, Image *theTexture, float tx = 0, float ty = 0, bool blend = true) = 0;
	virtual void FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor,
				  int theDrawMode, int tx, int ty) = 0;

	virtual void BltTexture(Texture *theTexture, const Rect &theSrcRect, const Rect &theDestRect,
					const Color &theColor, int theDrawMode) = 0;
};

}; // namespace PopLib

#endif
