#ifndef __SDLINTERFACE_HPP__
#define __SDLINTERFACE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "renderer.hpp"
#include "graphics/memoryimage.hpp"
#include "math/matrix.hpp"

#include <SDL3/SDL.h>

namespace PopLib
{
class SDLInterface : public Renderer
{
    public:
        SDL_Window* mBackendWindow;
        SDL_Renderer* mBackendRenderer;
        SDL_Texture*  mRenderBuffer;

		typedef std::set<MemoryImage*> ImageSet;
		ImageSet mImageSet;

    public:
		virtual void UpdateViewport();
		virtual void Cleanup();
        virtual void Draw(RenderCommand command);
        virtual void DrawTriangles(TriangleCommand command);
		virtual bool CreateImageTexture(MemoryImage *theImage);
		virtual void RemoveMemoryImage(MemoryImage *theImage);
};

enum PixelFormat
{
	PixelFormat_Unknown				=			0x0000,
	PixelFormat_A8R8G8B8			=			0x0001,
	PixelFormat_A4R4G4B4			=			0x0002,
	PixelFormat_R5G6B5				=			0x0004,
	PixelFormat_Palette8			=			0x0008
};

struct TextureDataPiece
{
	SDL_Surface* mSurface;
	SDL_Texture* mTexture;
	int mWidth,mHeight;
};

struct SDLTextureData
{
public:
	typedef std::vector<TextureDataPiece> SurfaceVector;

	SurfaceVector mTextures;
	SDL_Palette* mPalette;
	
	int mWidth,mHeight;
	int mTexVecWidth, mTexVecHeight;
	int mTexPieceWidth, mTexPieceHeight;
	int mBitsChangedCount;
	int mTexMemSize;
	float mMaxTotalU, mMaxTotalV;
	PixelFormat mPixelFormat;
	DWORD mImageFlags;

	SDLTextureData();
	~SDLTextureData();

	void ReleaseTextures();

	void CreateTextureDimensions(MemoryImage *theImage);
	void CreateTextures(MemoryImage *theImage, SDL_Renderer* theRenderer);
	void CheckCreateTextures(MemoryImage *theImage, SDL_Renderer* theRenderer);
	SDL_Texture* GetTexture(int x, int y, int &width, int &height, float &u1, float &v1, float &u2, float &v2);
	SDL_Texture* GetTextureF(float x, float y, float &width, float &height, float &u1, float &v1, float &u2, float &v2);

	void Blt(SDL_Renderer* theRenderer, float theX, float theY, const Rect& theSrcRect, const Color& theColor);
	void BltTransformed(SDL_Renderer* theRenderer, const Matrix3 &theTrans, const Rect& theSrcRect, const Color& theColor, const Rect *theClipRect = NULL, float theX = 0, float theY = 0, bool center = false);	
	void BltTriangles(SDL_Renderer* theRenderer, const TriVertex theVertices[][3], int theNumTriangles, ulong theColor, float tx = 0, float ty = 0);
};
} // namespace PopLib

#endif // __SDLINTERFACE_HPP__