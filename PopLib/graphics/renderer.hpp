#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__
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


enum TextureFlags
{
    Flag_MinimizeNumSubdivisions = 0x0001,		// subdivide image into fewest possible textures (may use more memory)
    Flag_Use64By64Subdivisions = 0x0002,		// good to use with image strips so the entire texture isn't pulled in when drawing just a piece
    Flag_UseA4R4G4B4 = 0x0004,		// images with not too many color gradients work well in this format
    Flag_UseA8R8G8B8 = 0x0008,		// non-alpha images will be stored as R5G6B5 by default so use this option if you want a 32-bit non-alpha image
	Flag_NearestFiltering = 0x0016 //use the nearest filtering for texture scaling.
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

enum PixelFormat
{
	PixelFormat_Unknown				=			0x0000,
	PixelFormat_A8R8G8B8			=			0x0001,
	PixelFormat_A4R4G4B4			=			0x0002,
	PixelFormat_R5G6B5				=			0x0004,
	PixelFormat_Palette8			=			0x0008
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Texture
{
  public:
	virtual ~Texture() = default;
};

class Renderer
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

	GPUImage *mScreenImage;

  public:
	Renderer();
	virtual ~Renderer();
	virtual void Cleanup() = 0;

	virtual void AddImage(Image *theImage) = 0;
	virtual void RemoveImage(Image *theImage) = 0;
	virtual void Remove3DData(GPUImage *theImage) = 0; // for 3d texture cleanup

	virtual void GetOutputSize(int *outWidth, int *outHeight) = 0;

	virtual GPUImage *NewGPUImage() = 0;

	virtual GPUImage *GetScreenImage() = 0;
	virtual void UpdateViewport() = 0;
	virtual int Init() = 0;

	virtual bool Redraw(Rect *theClipRect) = 0;
	virtual void SetVideoOnlyDraw(bool videoOnly) = 0;

	virtual std::unique_ptr<ImageData> CaptureFrameBuffer() = 0;

	virtual bool UpdateWindowIcon(Image *theImage) = 0;

	virtual void PushTransform(const Matrix3 &theTransform, bool concatenate = true) = 0;
	virtual void PopTransform() = 0;

	virtual bool PreDraw() = 0;

	virtual bool CreateImageTexture(GPUImage *theImage) = 0;
	virtual bool RecoverBits(GPUImage *theImage) = 0;

	virtual BlendMode ChooseBlendMode(int theBlendMode) = 0;
	virtual void DrawText(int theY, int theX, const PopString &theText, const Color &theColor, TTF_Font *theFont) = 0;

	// Draw Funcs
	virtual void Blt(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor,
					 int theDrawMode, bool linearFilter = false) = 0;
	virtual void BltClipF(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Rect *theClipRect,
						  const Color &theColor, int theDrawMode) = 0;
	virtual void BltMirror(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Color &theColor,
						   int theDrawMode, bool linearFilter = false) = 0;
	virtual void StretchBlt(Image *theImage, const Rect &theDestRect, const Rect &theSrcRect, const Rect *theClipRect,
							const Color &theColor, int theDrawMode, bool fastStretch, bool mirror = false) = 0;
	virtual void BltRotated(Image *theImage, float theX, float theY, const Rect *theClipRect, const Color &theColor,
							int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY,
							const Rect &theSrcRect) = 0;
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
	virtual void DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor,
								  int theDrawMode, Image *theTexture, float tx = 0, float ty = 0,
								  bool blend = true) = 0;
	virtual void DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor,
									   int theDrawMode, Image *theTexture, float tx = 0, float ty = 0,
									   bool blend = true) = 0;
	virtual void FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor,
						  int theDrawMode, int tx, int ty) = 0;

	virtual void BltTexture(Texture *theTexture, const Rect &theSrcRect, const Rect &theDestRect, const Color &theColor,
							int theDrawMode) = 0;
};

}; // namespace PopLib

#endif
