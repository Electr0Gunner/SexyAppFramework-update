#ifndef __SDLRENDERER_HPP__
#define __SDLRENDERER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "misc/critsect.hpp"
#include "../renderer.hpp"
#include "math/rect.hpp"
#include "math/ratio.hpp"
#include "math/matrix.hpp"
#include "sdlimage.hpp"

#include <SDL3/SDL.h>

namespace PopLib
{

class AppBase;
class SDLImage;
class Matrix3;
class TriVertex;

typedef std::set<SDLImage *> SDLImageSet;
typedef std::set<SDLImage *> ImageSet;
typedef std::list<Matrix3> TransformStack;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct SDLTextureData
{
  public:
	SDL_Texture *mTexture;
	int mWidth;
	int mHeight;
	int mBitsChangedCount;
	SDL_Renderer *mRenderer;

	SDLTextureData(SDL_Renderer *theRenderer);
	~SDLTextureData();

	void ReleaseTextures();

	void CreateTextures(SDLImage *theImage);
	void CheckCreateTextures(SDLImage *theImage);

	int GetMemSize();
};

class SDLTexture : public Texture
{
  public:
	SDLTexture(SDL_Texture *texture) : mTexture(texture)
	{
	}
	~SDLTexture() override
	{
		if (mTexture)
			SDL_DestroyTexture(mTexture);
	}

	SDL_Texture *GetSDLTexture() const
	{
		return mTexture;
	}

  private:
	SDL_Texture *mTexture;
};

class SDLRenderer : public Renderer
{
  public:
	ImageSet mImageSet;
	GPUImageSet mSDLImageSet;
	TransformStack mTransformStack;

  public:
	SDL_Renderer *mRenderer;
	SDL_Texture *mScreenTexture;

  public:
	void AddImage(Image *theImage);
	void RemoveImage(Image *theImage);
	void Remove3DData(GPUImage *theImage); // for 3d texture cleanup

  public:
	SDLRenderer(AppBase *theApp);
	virtual ~SDLRenderer();
	virtual void Cleanup();

	virtual GPUImage *NewGPUImage()
	{
		return new SDLImage();
	}
	virtual GPUImage *GetScreenImage();
	virtual void UpdateViewport();
	virtual int Init();

	bool InitSDLWindow();
	bool InitSDLRenderer();

	virtual void GetOutputSize(int *outWidth, int *outHeight);

	virtual std::unique_ptr<ImageData> CaptureFrameBuffer();

	virtual bool Redraw(Rect *theClipRect);
	virtual void SetVideoOnlyDraw(bool videoOnly);

	virtual void SetCursorPos(int theCursorX, int theCursorY);
	virtual bool SetCursorImage(Image *theImage);

	virtual bool UpdateWindowIcon(Image *theImage);

	virtual void SetCursor(CursorType theCursorType);
	virtual void DrawText(int theY, int theX, const PopString &theText, const Color &theColor, TTF_Font *theFont);

  public:
	virtual void PushTransform(const Matrix3 &theTransform, bool concatenate = true);
	virtual void PopTransform();

	virtual bool PreDraw();

	virtual bool CreateImageTexture(GPUImage *theImage);
	virtual bool RecoverBits(GPUImage *theImage);

	virtual BlendMode ChooseBlendMode(int theBlendMode);

	// Draw Funcs
	virtual void Blt(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor,
					 int theDrawMode, bool linearFilter = false);
	virtual void BltClipF(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Rect *theClipRect,
						  const Color &theColor, int theDrawMode);
	virtual void BltMirror(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Color &theColor,
						   int theDrawMode, bool linearFilter = false);
	virtual void StretchBlt(Image *theImage, const Rect &theDestRect, const Rect &theSrcRect, const Rect *theClipRect,
							const Color &theColor, int theDrawMode, bool fastStretch, bool mirror = false);
	virtual void BltRotated(Image *theImage, float theX, float theY, const Rect *theClipRect, const Color &theColor,
							int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY,
							const Rect &theSrcRect);
	virtual void BltTransformed(Image *theImage, const Rect *theClipRect, const Color &theColor, int theDrawMode,
								const Rect &theSrcRect, const Matrix3 &theTransform, bool linearFilter, float theX = 0,
								float theY = 0, bool center = false);
	virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor,
						  int theDrawMode);
	virtual void FillRect(const Rect &theRect, const Color &theColor, int theDrawMode);
	virtual void DrawTriangle(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
							  int theDrawMode);
	virtual void DrawTriangleTex(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
								 int theDrawMode, Image *theTexture, bool blend = true);
	virtual void DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor,
								  int theDrawMode, Image *theTexture, float tx = 0, float ty = 0, bool blend = true);
	virtual void DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor,
									   int theDrawMode, Image *theTexture, float tx = 0, float ty = 0,
									   bool blend = true);
	virtual void FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor,
						  int theDrawMode, int tx, int ty);

	virtual void BltTexture(Texture *theTexture, const Rect &theSrcRect, const Rect &theDestRect, const Color &theColor,
							int theDrawMode);
};
} // namespace PopLib

#endif // __SDLRENDERER_HPP__