#include "sdlrenderer.hpp"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_render.h"
#include "graphics/graphics.hpp"
#include "graphics/renderer.hpp"
#include "sdlimage.hpp"
#include "appbase.hpp"
#include "misc/autocrit.hpp"
#include "misc/critsect.hpp"
#include "imgui/imguimanager.hpp"
#include <SDL3_ttf/SDL_ttf.h>

using namespace PopLib;

SDL_FPoint TransformToPoint(float x, float y, const Matrix3 &m, float aTransX = 0, float aTransY = 0)
{
	SDL_FPoint result;
	result.x = m.m00 * x + m.m01 * y + m.m02 + aTransX;
	result.y = m.m10 * x + m.m11 * y + m.m12 + aTransY;
	return result;
}

SDLRenderer::SDLRenderer(AppBase *theApp)
{
	mApp = theApp;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;
	mPresentationRect = Rect(0, 0, mWidth, mHeight);
	mScreenImage = nullptr;
	mHasInitiated = false;
	mCursorX = 0;
	mCursorY = 0;
	mIs3D = false;
	mMillisecondsPerFrame = 0;
	mNextCursorX = 0;
	mNextCursorY = 0;
	mRefreshRate = 0;
	mRenderer = nullptr;
	mScreenTexture = nullptr;
}

SDLRenderer::~SDLRenderer()
{
	Cleanup();
	SDL_Quit();
}

void SDLRenderer::DrawText(int theY, int theX, const PopString &theText, const Color &theColor, TTF_Font *theFont)
{
	SDL_Color aColor = {(Uint8)theColor.mRed, (Uint8)theColor.mGreen, (Uint8)theColor.mBlue, (Uint8)theColor.mAlpha};
	SDL_Surface *textSurface = TTF_RenderText_Blended(theFont, theText.c_str(), 0, aColor);
	if (!textSurface)
	{
		SDL_ShowSimpleMessageBox(static_cast<SDL_MessageBoxFlags>(MsgBox_OK), "Failed to render text: ", SDL_GetError(), mApp->mWindow);
		return;
	}
	SDL_Texture *textTexture = SDL_CreateTextureFromSurface(mRenderer, textSurface);
	SDL_FRect dstRect = {(float)theX, (float)theY, (float)textSurface->w, (float)textSurface->h};
	SDL_DestroySurface(textSurface);

	if (!textTexture)
	{
		SDL_ShowSimpleMessageBox(static_cast<SDL_MessageBoxFlags>(MsgBox_OK), "Failed to create texture from surface: ", SDL_GetError(), mApp->mWindow);
		return;
	}

	SDL_RenderTexture(mRenderer, textTexture, nullptr, &dstRect);
	SDL_DestroyTexture(textTexture);
}

void SDLRenderer::Cleanup()
{
	ImageSet::iterator anItr;
	for (anItr = mImageSet.begin(); anItr != mImageSet.end(); ++anItr)
	{
		SDLImage *anImage = *anItr;
		SDLTextureData *aData = (SDLTextureData *)anImage->mD3DData;
		delete aData;
		anImage->mD3DData = nullptr;
	}
	mImageSet.clear();

	SDL_DestroyRenderer(mRenderer);
	mHasInitiated = false;
}

void SDLRenderer::AddImage(Image *theImage)
{
	AutoCrit anAutoCrit(mCritSect);

	mSDLImageSet.insert((SDLImage *)theImage);
}

void SDLRenderer::RemoveImage(Image *theImage)
{
	AutoCrit anAutoCrit(mCritSect);

	GPUImageSet::iterator anItr = mSDLImageSet.find((SDLImage *)theImage);
	if (anItr != mSDLImageSet.end())
		mSDLImageSet.erase(anItr);
}

void SDLRenderer::Remove3DData(GPUImage *theImage)
{
	if (theImage->mD3DData != nullptr)
	{
		delete (SDLTextureData *)theImage->mD3DData;
		theImage->mD3DData = nullptr;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.erase(static_cast<SDLImage *>(theImage));
	}
}

void SDLRenderer::GetOutputSize(int *outWidth, int *outHeight)
{
	SDL_GetCurrentRenderOutputSize(mRenderer, outWidth, outHeight);
}

std::unique_ptr<ImageData> SDLRenderer::CaptureFrameBuffer()
{
	SDL_Surface *surface = SDL_RenderReadPixels(mRenderer, nullptr);
	if (!surface)
		return nullptr;

	auto image = std::make_unique<ImageData>();
	image->width = surface->w;
	image->height = surface->h;
	image->pixels.resize(surface->w * surface->h * 4);

	uint8_t *src = static_cast<uint8_t *>(surface->pixels);
	uint8_t *dst = image->pixels.data();

	for (int i = 0; i < surface->w * surface->h * 4; i += 4)
	{
		dst[i + 0] = src[i + 2]; // R
		dst[i + 1] = src[i + 1]; // G
		dst[i + 2] = src[i + 0]; // B
		dst[i + 3] = src[i + 3]; // A
	}

	SDL_DestroySurface(surface);
	return image;
}

GPUImage *SDLRenderer::GetScreenImage()
{
	return mScreenImage;
}

void SDLRenderer::UpdateViewport()
{
	if (SDL_GetCurrentThreadID() != SDL_GetThreadID(nullptr))
		return;

	int windowWidth, windowHeight;
	if (!SDL_GetWindowSize(mApp->mWindow, &windowWidth, &windowHeight))
		return;

	if (!SDL_SetRenderLogicalPresentation(mRenderer, windowWidth, windowHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX))
		return;

	mPresentationRect = Rect(0, 0, windowWidth, windowHeight);
}

int SDLRenderer::Init()
{
	int aResult = RESULT_OK;

	if (mHasInitiated)
		Cleanup();

	mRGBBits = 32;

	mRedBits = 8;
	mGreenBits = 8;
	mBlueBits = 8;

	mRedShift = 0;
	mGreenShift = 8;
	mBlueShift = 16;

	mRedMask = (0xFFU << mRedShift);
	mGreenMask = (0xFFU << mGreenShift);
	mBlueMask = (0xFFU << mBlueShift);

	aResult = InitSDLWindow() ? aResult : RESULT_FAIL;
	mHasInitiated = true;
	return aResult;
}

bool SDLRenderer::InitSDLWindow()
{
	UpdateWindowIcon(mApp->mTitleBarIcon);

	return InitSDLRenderer();
}

bool SDLRenderer::InitSDLRenderer()
{
	mRenderer = SDL_CreateRenderer(mApp->mWindow, nullptr);
	if (mRenderer == nullptr)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Creation Failed", SDL_GetError(), nullptr);
		return false;
	}

	mScreenTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mWidth, mHeight);
	if (mScreenTexture == nullptr)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Screen Texture-Buffer Creation Failed", SDL_GetError(),
								 nullptr);
		return false;
	}

	const SDL_DisplayMode *aMode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(mApp->mWindow));
	mRefreshRate = aMode->refresh_rate;
	if (!mRefreshRate)
		mRefreshRate = 60;
	mMillisecondsPerFrame = 1000 / mRefreshRate;

	SetVideoOnlyDraw(false);

	UpdateViewport();

	return true;
}

namespace PopLib
{
bool gRendererPreDrawError = false;
}

bool SDLRenderer::Redraw(Rect *theClipRect)
{
	// HACK: i dont know where to put this
	mApp->mIGUIManager->Frame();

	SDL_SetRenderTarget(mRenderer, nullptr);

	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 0);
	SDL_SetTextureBlendMode(mScreenTexture, SDL_BLENDMODE_BLEND);
	SDL_RenderClear(mRenderer);

	SDL_Rect clipRect =
		theClipRect
			? SDL_Rect{theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight}
			: SDL_Rect{mPresentationRect.mX, mPresentationRect.mY, mPresentationRect.mWidth, mPresentationRect.mHeight};

	SDL_SetRenderClipRect(mRenderer, &clipRect);

	PopLib::gRendererPreDrawError = !SDL_RenderTexture(mRenderer, mScreenTexture, nullptr, nullptr);

	if (ImGui::GetDrawData() != nullptr)
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);

	SDL_RenderPresent(mRenderer);

	return !PopLib::gRendererPreDrawError;
}

/// <summary>
/// Setup the mScreenImage
/// </summary>
/// <param name="videoOnly"></param>
void SDLRenderer::SetVideoOnlyDraw(bool videoOnly)
{
	if (mScreenImage)
		delete mScreenImage;
	mScreenImage = new SDLImage();
	mScreenImage->Create(mWidth, mHeight);
	// mScreenImage->SetSurface(useSecondary ? mSecondarySurface : mDrawSurface);
	// mScreenImage->mNoLock = mVideoOnlyDraw;
	// mScreenImage->mVideoMemory = mVideoOnlyDraw;
	mScreenImage->mWidth = mWidth;
	mScreenImage->mHeight = mHeight;
	mScreenImage->SetImageMode(false, false);
}

/// <summary>
/// Set the next cursor position
/// </summary>
/// <param name="theCursorX"></param>
/// <param name="theCursorY"></param>
void SDLRenderer::SetCursorPos(int theCursorX, int theCursorY)
{
	mNextCursorX = theCursorX;
	mNextCursorY = theCursorY;
}

/// <summary>
/// Set the cursor image to a Image* or nullptr to hide the cursor
/// </summary>
/// <param name="theImage"></param>
/// <returns></returns>
bool SDLRenderer::SetCursorImage(Image *theImage)
{
	AutoCrit anAutoCrit(mCritSect);

	if (mCursorImage != theImage)
	{
		mCursorImage = theImage;
		if (theImage == nullptr)
			return true;
		SDL_Surface *aSurface =
			SDL_CreateSurfaceFrom(theImage->mWidth, theImage->mHeight, SDL_PIXELFORMAT_ARGB8888,
								  ((SDLImage *)mCursorImage)->GetBits(), theImage->mWidth * sizeof(ulong));

		SDL_Cursor *aCursor = SDL_CreateColorCursor(aSurface, mCursorImage->mWidth / 2, mCursorImage->mHeight / 2);

		SDL_SetCursor(aCursor);

		SDL_DestroySurface(aSurface);

		return true;
	}

	return false;
}

bool SDLRenderer::UpdateWindowIcon(Image *theImage)
{
	if (theImage != nullptr)
	{
		SDL_Surface *aSurface =
			SDL_CreateSurfaceFrom(theImage->mWidth, theImage->mHeight, SDL_PIXELFORMAT_ARGB8888,
								  ((SDLImage *)theImage)->GetBits(), theImage->mWidth * sizeof(ulong));

		SDL_SetWindowIcon(mApp->mWindow, aSurface);

		SDL_DestroySurface(aSurface);
		return true;
	}
	return false;
}

void SDLRenderer::SetCursor(CursorType theCursorType)
{
	SDL_SystemCursor sdlCursorType;

	switch (theCursorType)
	{
	case CURSOR_POINTER:
	case CURSOR_HAND:
		sdlCursorType = SDL_SYSTEM_CURSOR_POINTER;
		break;
	case CURSOR_DRAGGING:
		sdlCursorType = SDL_SYSTEM_CURSOR_MOVE;
		break;
	case CURSOR_TEXT:
		sdlCursorType = SDL_SYSTEM_CURSOR_TEXT;
		break;
	case CURSOR_CIRCLE_SLASH:
		sdlCursorType = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
		break;
	case CURSOR_SIZEALL:
		sdlCursorType = SDL_SYSTEM_CURSOR_MOVE;
		break;
	case CURSOR_SIZENESW:
		sdlCursorType = SDL_SYSTEM_CURSOR_NESW_RESIZE;
		break;
	case CURSOR_SIZENS:
		sdlCursorType = SDL_SYSTEM_CURSOR_NS_RESIZE;
		break;
	case CURSOR_SIZENWSE:
		sdlCursorType = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
		break;
	case CURSOR_SIZEWE:
		sdlCursorType = SDL_SYSTEM_CURSOR_EW_RESIZE;
		break;
	case CURSOR_WAIT:
		sdlCursorType = SDL_SYSTEM_CURSOR_WAIT;
		break;
	case CURSOR_NONE:
		SDL_HideCursor();
		return;
	default:
		sdlCursorType = SDL_SYSTEM_CURSOR_DEFAULT;
		break;
	}

	SDL_Cursor *aCursor = SDL_CreateSystemCursor(sdlCursorType);
	SDL_SetCursor(aCursor);
	// SDL_DestroyCursor(aCursor);
}

void SDLRenderer::PushTransform(const Matrix3 &theTransform, bool concatenate)
{
	if (mTransformStack.empty() || !concatenate)
		mTransformStack.push_back(theTransform);
	else
	{
		Matrix3 &aTrans = mTransformStack.back();
		mTransformStack.push_back(theTransform * aTrans);
	}
}

void SDLRenderer::PopTransform()
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

bool SDLRenderer::PreDraw()
{
	return true;
}

bool SDLRenderer::CreateImageTexture(GPUImage *theImage)
{
	bool wantPurge = false;

	if (theImage->mD3DData == nullptr)
	{
		theImage->mD3DData = new SDLTextureData(mRenderer);

		// The actual purging was deferred
		wantPurge = theImage->mPurgeBits;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.insert(static_cast<SDLImage *>(theImage));
	}

	SDLTextureData *aData = static_cast<SDLTextureData *>(theImage->mD3DData);
	aData->CheckCreateTextures(static_cast<SDLImage *>(theImage));

	if (wantPurge)
		theImage->PurgeBits();

	return true;
}

bool SDLRenderer::RecoverBits(GPUImage *theImage)
{
	if (theImage->mD3DData == nullptr)
		return false;

	SDLTextureData *aData = (SDLTextureData *)theImage->mD3DData;
	if (aData->mBitsChangedCount != theImage->mBitsChangedCount) // bits have changed since texture was created
		return false;

	// Reverse the process: copy texture data to theImage
	float aWidth;
	float aHeight;
	void *pixels;
	int pitch;

	if (SDL_LockTexture(aData->mTexture, nullptr, &pixels, &pitch) &&
		SDL_GetTextureSize(aData->mTexture, &aWidth, &aHeight))
	{
		theImage->SetBits((ulong *)pixels, (int)aWidth, (int)aHeight);
		SDL_UnlockTexture(aData->mTexture);
	}

	return true;
}

BlendMode SDLRenderer::ChooseBlendMode(int theBlendMode)
{
	BlendMode theBBlendMode;
	switch (theBlendMode)
	{
	case Graphics::DRAWMODE_ADDITIVE:
		theBBlendMode = BLENDMODE_ADD;
		break;
	default:
	case Graphics::DRAWMODE_NORMAL:
		theBBlendMode = BLENDMODE_BLEND;
		break;
	}
	return theBBlendMode;
}

SDLTextureData::SDLTextureData(SDL_Renderer *theRenderer)
{
	mWidth = 0;
	mHeight = 0;
	mBitsChangedCount = 0;
	mRenderer = theRenderer;
	mTexture = nullptr;
}

SDLTextureData::~SDLTextureData()
{
	ReleaseTextures();
}

void SDLTextureData::ReleaseTextures()
{
	if (mTexture != nullptr)
		SDL_DestroyTexture(mTexture);
}

void SDLTextureData::CreateTextures(SDLImage *theImage)
{
	theImage->DeleteSWBuffers(); // we don't need the software buffers anymore
	theImage->CommitBits();

	bool createTexture = false;

	// only recreate the texture if the dimensions or image data have changed
	if (mWidth != theImage->mWidth || mHeight != theImage->mHeight || mBitsChangedCount != theImage->mBitsChangedCount)
	{
		ReleaseTextures();
		createTexture = true;
	}

	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();

	if (createTexture)
	{
		mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, aWidth, aHeight);

		if (mTexture)
		{
			const char *rendererName = SDL_GetRendererName(mRenderer);

			SDL_SetTextureScaleMode(mTexture, (rendererName && strcmp(rendererName, "software") == 0)
												  ? SDL_SCALEMODE_NEAREST
												  : SDL_SCALEMODE_LINEAR);

			void *bits = theImage->GetBits();
			if (bits)
			{
				SDL_UpdateTexture(mTexture, nullptr, bits, aWidth * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ARGB8888));
			}
			else
			{
				SDL_Log("Error: Image bits are nullptr, cannot update texture.");
			}
		}
		else
		{
			SDL_Log("Failed to create texture: %s", SDL_GetError());
		}
	}
	else if (mBitsChangedCount != theImage->mBitsChangedCount)
	{
		void *bits = theImage->GetBits();
		if (bits)
		{
			SDL_UpdateTexture(mTexture, nullptr, bits, aWidth * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ARGB8888));
		}
		else
		{
			SDL_Log("Error: Image bits are nullptr, cannot update texture.");
		}
	}

	mWidth = theImage->mWidth;
	mHeight = theImage->mHeight;
	mBitsChangedCount = theImage->mBitsChangedCount;
}

void SDLTextureData::CheckCreateTextures(SDLImage *theImage)
{
	if (mTexture != nullptr)
	{
		if (mWidth != theImage->mWidth || mHeight != theImage->mHeight ||
			mBitsChangedCount != theImage->mBitsChangedCount)
			CreateTextures(theImage);
		return;
	}
	CreateTextures(theImage);
}

int SDLTextureData::GetMemSize()
{
	int aSize = 0;

	aSize = (SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ARGB8888) / 8) * mWidth * mHeight;

	return aSize;
}

/////////////////////////////////////////////////////////////////
///				DRAWING/BLITTING FUNCTIONS		    	   //////
/////////////////////////////////////////////////////////////////

void SDLRenderer::Blt(Image *theImage, int theX, int theY, const Rect &theSrcRect, const Color &theColor,
					   int theDrawMode, bool linearFilter)
{
	SDLImage *memImg = static_cast<SDLImage *>(theImage);
	if (!CreateImageTexture(memImg))
		return;

	SDLTextureData *texData = static_cast<SDLTextureData *>(memImg->mD3DData);
	SDL_Texture *texture = texData->mTexture;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_SetTextureColorMod(texture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(texture, theColor.GetAlpha());
	SDL_SetTextureBlendMode(texture, ChooseBlendMode(theDrawMode));

	SDL_FRect srcF = {(float)theSrcRect.mX, (float)theSrcRect.mY, (float)theSrcRect.mWidth, (float)theSrcRect.mHeight};
	SDL_FRect dstF = {(float)theX, (float)theY, (float)theSrcRect.mWidth, (float)theSrcRect.mHeight};

	bool ok = SDL_RenderTexture(mRenderer, texture, &srcF, &dstF);
	if (ok)
		SDL_SetRenderTarget(mRenderer, nullptr);

	return;
}

void SDLRenderer::BltClipF(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Rect *theClipRect,
							const Color &theColor, int theDrawMode)
{
	SDLImage *aSrcSDLImage = (SDLImage *)theImage;

	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = (SDLTextureData *)aSrcSDLImage->mD3DData;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_Texture *aTexture = aData->mTexture;
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
	SDL_SetTextureScaleMode(aTexture, theDrawMode ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);

	SDL_FRect destRect = {theX, theY, (float)theSrcRect.mWidth, (float)theSrcRect.mHeight};
	if (theClipRect != nullptr)
	{
		SDL_Rect clipRect;
		clipRect = {theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight};
		SDL_SetRenderClipRect(mRenderer, &clipRect);
	}

	SDL_FRect srcRect = {(float)theSrcRect.mX, (float)theSrcRect.mY, (float)theSrcRect.mWidth,
						 (float)theSrcRect.mHeight};

	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));
	SDL_RenderTexture(mRenderer, aTexture, &srcRect, &destRect);
	SDL_SetRenderClipRect(mRenderer, nullptr);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::BltMirror(Image *theImage, float theX, float theY, const Rect &theSrcRect, const Color &theColor,
							 int theDrawMode, bool linearFilter)
{
	SDLImage *aSrcSDLImage = (SDLImage *)theImage;

	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = (SDLTextureData *)aSrcSDLImage->mD3DData;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_Texture *aTexture = aData->mTexture;
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());

	SDL_FRect destRect = {theX, theY, (float)theSrcRect.mWidth, (float)theSrcRect.mHeight};
	SDL_FRect srcRect = {(float)theSrcRect.mX, (float)theSrcRect.mY, (float)theSrcRect.mWidth,
						 (float)theSrcRect.mHeight};

	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));

	SDL_RenderTextureRotated(mRenderer, aTexture, &srcRect, &destRect, 0, nullptr, SDL_FLIP_HORIZONTAL);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::StretchBlt(Image *theImage, const Rect &theDestRect, const Rect &theSrcRect, const Rect *theClipRect,
							  const Color &theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	SDLImage *aSrcSDLImage = static_cast<SDLImage *>(theImage);
	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = static_cast<SDLTextureData *>(aSrcSDLImage->mD3DData);
	SDL_Texture *aTexture = aData->mTexture;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
	SDL_SetTextureScaleMode(aTexture, fastStretch ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);

	SDL_FRect destRect = {(float)theDestRect.mX, (float)theDestRect.mY, (float)theDestRect.mWidth,
						  (float)theDestRect.mHeight};

	if (theClipRect != nullptr)
	{
		SDL_Rect clipRect;
		clipRect = {theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight};
		SDL_SetRenderClipRect(mRenderer, &clipRect);
	}

	SDL_FRect srcRect = {(float)theSrcRect.mX, (float)theSrcRect.mY, (float)theSrcRect.mWidth,
						 (float)theSrcRect.mHeight};

	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));
	SDL_RenderTextureRotated(mRenderer, aTexture, &srcRect, &destRect, 0.0, nullptr,
							 mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
	SDL_SetRenderClipRect(mRenderer, nullptr);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::BltRotated(Image *theImage, float theX, float theY, const Rect *theClipRect, const Color &theColor,
							  int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY,
							  const Rect &theSrcRect)
{
	SDLImage *aSrcSDLImage = static_cast<SDLImage *>(theImage);
	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = static_cast<SDLTextureData *>(aSrcSDLImage->mD3DData);
	SDL_Texture *aTexture = aData ? aData->mTexture : nullptr;
	if (!aTexture)
		return;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());

	SDL_FRect destRect = {theX, theY, static_cast<float>(theSrcRect.mWidth), static_cast<float>(theSrcRect.mHeight)};
	SDL_FRect srcRect = {static_cast<float>(theSrcRect.mX), static_cast<float>(theSrcRect.mY),
						 static_cast<float>(theSrcRect.mWidth), static_cast<float>(theSrcRect.mHeight)};

	if (theClipRect != nullptr)
	{
		SDL_Rect clipRect;
		clipRect = {theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight};
		SDL_SetRenderClipRect(mRenderer, &clipRect);
	}

	SDL_FPoint rotationCenter = {theRotCenterX, theRotCenterY};

	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));
	SDL_RenderTextureRotated(mRenderer, aTexture, &srcRect, &destRect, theRot, &rotationCenter, SDL_FLIP_NONE);
	SDL_SetRenderClipRect(mRenderer, nullptr);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::BltTransformed(Image *theImage, const Rect *theClipRect, const Color &theColor, int theDrawMode,
								  const Rect &theSrcRect, const Matrix3 &theTransform, bool linearFilter, float theX,
								  float theY, bool center)
{
	SDLImage *aSrcSDLImage = static_cast<SDLImage *>(theImage);

	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = static_cast<SDLTextureData *>(aSrcSDLImage->mD3DData);
	if (!aData || !aData->mTexture)
		return;

	SDL_Texture *aTexture = aData->mTexture;
	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());

	if (theClipRect != nullptr)
	{
		SDL_Rect clipRect;
		clipRect = {theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight};
		SDL_SetRenderClipRect(mRenderer, &clipRect);
	}

	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));

	float halfWidth = theSrcRect.mWidth * 0.5f;
	float halfHeight = theSrcRect.mHeight * 0.5f;

	float x1 = center ? -halfWidth : 0;
	float y1 = center ? -halfHeight : 0;
	float x2 = x1 + theSrcRect.mWidth;
	float y2 = y1;
	float x3 = x1;
	float y3 = y1 + theSrcRect.mHeight;
	float x4 = x2;
	float y4 = y3;

	float u1 = static_cast<float>(theSrcRect.mX) / theImage->mWidth;
	float v1 = static_cast<float>(theSrcRect.mY) / theImage->mHeight;
	float u2 = static_cast<float>(theSrcRect.mX + theSrcRect.mWidth) / theImage->mWidth;
	float v2 = static_cast<float>(theSrcRect.mY + theSrcRect.mHeight) / theImage->mHeight;

	SDL_FColor aColor = {theColor.GetRed() / 255.0f, theColor.GetGreen() / 255.0f, theColor.GetBlue() / 255.0f,
						 theColor.GetAlpha() / 255.0f};

	SDL_Vertex vertices[4] = {
		{TransformToPoint(x1, y1, theTransform, theX, theY), aColor, {u1, v1}}, // TL
		{TransformToPoint(x2, y2, theTransform, theX, theY), aColor, {u2, v1}}, // TR
		{TransformToPoint(x3, y3, theTransform, theX, theY), aColor, {u1, v2}}, // BL
		{TransformToPoint(x4, y4, theTransform, theX, theY), aColor, {u2, v2}}	// BR
	};

	int indices[] = {0, 1, 2, 1, 3, 2};

	SDL_RenderGeometry(mRenderer, aTexture, vertices, 4, indices, 6);
	SDL_SetRenderClipRect(mRenderer, nullptr);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor,
							int theDrawMode)
{
	if (!mRenderer)
		return;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(theDrawMode));
	SDL_SetRenderDrawColor(mRenderer, theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	SDL_RenderLine(mRenderer, theStartX, theStartY, theEndX, theEndY);

	SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(Graphics::DRAWMODE_NORMAL));
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::FillRect(const Rect &theRect, const Color &theColor, int theDrawMode)
{
	if (!mRenderer)
		return;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_FRect theSDLRect = {(float)theRect.mX, (float)theRect.mY, (float)theRect.mWidth, (float)theRect.mHeight};

	SDL_SetRenderDrawColor(mRenderer, theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(theDrawMode));
	SDL_RenderFillRect(mRenderer, &theSDLRect);

	SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(Graphics::DRAWMODE_NORMAL));
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::DrawTriangle(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
								int theDrawMode)
{
	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_FColor aColor = {(float)theColor.GetRed(), (float)theColor.GetGreen(), (float)theColor.GetBlue(),
						 (float)theColor.GetAlpha()};

	int indices[] = {0, 1, 2};

	SDL_Vertex vertices[3] = {{SDL_FPoint{p1.x, p1.y}, aColor, {p1.u, p1.v}},
							  {SDL_FPoint{p2.x, p2.y}, aColor, {p2.u, p2.v}},
							  {SDL_FPoint{p3.x, p3.y}, aColor, {p3.u, p3.v}}};

	SDL_RenderGeometry(mRenderer, nullptr, vertices, 3, indices, 3);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::DrawTriangleTex(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor,
								   int theDrawMode, Image *theTexture, bool blend)
{
	SDLImage *aSrcSDLImage = (SDLImage *)theTexture;

	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = (SDLTextureData *)aSrcSDLImage->mD3DData;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_Texture *aTexture = aData->mTexture;
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));

	SDL_FColor aColor = {(float)theColor.GetRed(), (float)theColor.GetGreen(), (float)theColor.GetBlue(),
						 (float)theColor.GetAlpha()};

	int indices[] = {0, 1, 2};

	SDL_Vertex vertices[3] = {{SDL_FPoint{p1.x, p1.y}, aColor, {p1.u, p1.v}},
							  {SDL_FPoint{p2.x, p2.y}, aColor, {p2.u, p2.v}},
							  {SDL_FPoint{p3.x, p3.y}, aColor, {p3.u, p3.v}}};

	SDL_RenderGeometry(mRenderer, aTexture, vertices, 3, indices, 3);
	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor,
									int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	SDLImage *aSrcSDLImage = (SDLImage *)theTexture;

	if (!CreateImageTexture(aSrcSDLImage))
		return;

	SDLTextureData *aData = (SDLTextureData *)aSrcSDLImage->mD3DData;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_Texture *aTexture = aData->mTexture;
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));

	for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
	{
		TriVertex theCurrentVertex[3];
		theCurrentVertex[0] = theVertices[aTriangleNum][0];
		theCurrentVertex[1] = theVertices[aTriangleNum][1];
		theCurrentVertex[2] = theVertices[aTriangleNum][2];

		SDL_Vertex vertices[3];

		SDL_FColor aColor[3];
		aColor[0].r = (theCurrentVertex[0].color >> 16) & 0xFF;
		aColor[0].g = (theCurrentVertex[0].color >> 8) & 0xFF;
		aColor[0].b = theCurrentVertex[0].color & 0xFF;
		aColor[0].a = (theCurrentVertex[0].color >> 24) & 0xFF; // alpha extraction

		aColor[1].r = (theCurrentVertex[1].color >> 16) & 0xFF;
		aColor[1].g = (theCurrentVertex[1].color >> 8) & 0xFF;
		aColor[1].b = theCurrentVertex[1].color & 0xFF;
		aColor[1].a = (theCurrentVertex[1].color >> 24) & 0xFF;

		aColor[2].r = (theCurrentVertex[2].color >> 16) & 0xFF;
		aColor[2].g = (theCurrentVertex[2].color >> 8) & 0xFF;
		aColor[2].b = theCurrentVertex[2].color & 0xFF;
		aColor[2].a = (theCurrentVertex[2].color >> 24) & 0xFF;

		vertices[0].position.x = theCurrentVertex[0].x + tx;
		vertices[0].position.y = theCurrentVertex[0].y + ty;
		vertices[1].position.x = theCurrentVertex[1].x + tx;
		vertices[1].position.y = theCurrentVertex[1].y + ty;
		vertices[2].position.x = theCurrentVertex[2].x + tx;
		vertices[2].position.y = theCurrentVertex[2].y + ty;
		vertices[0].color = aColor[0];
		vertices[1].color = aColor[1];
		vertices[2].color = aColor[2];

		SDL_RenderGeometry(mRenderer, aTexture, vertices, 3, nullptr, 3);
	}

	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor,
										 int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	if (theNumTriangles < 3)
		return;

	SDLImage *aSrcSDLImage = (SDLImage *)theTexture;
	if (!CreateImageTexture(aSrcSDLImage))
		return;
	SDLTextureData *aData = (SDLTextureData *)aSrcSDLImage->mD3DData;
	SDL_Texture *aTexture = aData->mTexture;

	std::vector<float> positions;
	std::vector<SDL_FColor> colors;
	std::vector<float> uvs;

	for (int i = 0; i < theNumTriangles; ++i)
	{
		const TriVertex &v = theVertices[i];
		positions.push_back(v.x + tx);
		positions.push_back(v.y + ty);
		uvs.push_back(v.u);
		uvs.push_back(v.v);

		SDL_FColor color = {(float)((v.color >> 16) & 0xFF), (float)((v.color >> 8) & 0xFF), (float)(v.color & 0xFF),
							(float)((v.color >> 24) & 0xFF)};
		colors.push_back(color);
	}

	SDL_SetRenderTarget(mRenderer, mScreenTexture);
	SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(theDrawMode));
	SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());

	SDL_RenderGeometryRaw(mRenderer, aTexture, positions.data(), sizeof(float) * 2, colors.data(), sizeof(SDL_FColor),
						  uvs.data(), sizeof(float) * 2, positions.size() / 2, nullptr, 0, 0);

	SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLRenderer::FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect,
							const Color &theColor, int theDrawMode, int tx, int ty)
{
	if (theNumVertices < 3)
		if (theNumVertices == 2)
		{
			DrawLine(theVertices[0].mX + tx, theVertices[0].mY + ty, theVertices[1].mX + tx, theVertices[1].mY + ty,
					 theColor, theDrawMode);
			return;
		}

	if (theClipRect != nullptr)
	{
		SDL_Rect clipRect;
		clipRect = {theClipRect->mX, theClipRect->mY, theClipRect->mWidth, theClipRect->mHeight};
		SDL_SetRenderClipRect(mRenderer, &clipRect);
	}

	for (int i = 0; i < theNumVertices - 1; i++)
		DrawLine(theVertices[i].mX + tx, theVertices[i].mY + ty, theVertices[i + 1].mX + tx, theVertices[i + 1].mY + ty,
				 theColor, theDrawMode);

	DrawLine(theVertices[theNumVertices - 1].mX + tx, theVertices[theNumVertices - 1].mY + ty, theVertices[0].mX + tx,
			 theVertices[0].mY + ty, theColor, theDrawMode);

	SDL_SetRenderClipRect(mRenderer, nullptr);
}

void SDLRenderer::BltTexture(Texture *theTexture, const Rect &theSrcRect, const Rect &theDestRect,
							  const Color &theColor, int theDrawMode)
{
	SDLTexture *sdlTex = dynamic_cast<SDLTexture *>(theTexture);
	if (!sdlTex || !sdlTex->GetSDLTexture())
		return;

	SDL_SetRenderTarget(mRenderer, mScreenTexture);

	SDL_SetTextureColorMod(sdlTex->GetSDLTexture(), theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
	SDL_SetTextureAlphaMod(sdlTex->GetSDLTexture(), theColor.GetAlpha());
	SDL_SetTextureBlendMode(sdlTex->GetSDLTexture(), ChooseBlendMode(theDrawMode));

	SDL_FRect srcRect = {
		static_cast<float>(theSrcRect.mX),
		static_cast<float>(theSrcRect.mY),
		static_cast<float>(theSrcRect.mWidth),
		static_cast<float>(theSrcRect.mHeight),
	};

	SDL_FRect destRect = {
		static_cast<float>(theDestRect.mX),
		static_cast<float>(theDestRect.mY),
		static_cast<float>(theDestRect.mWidth),
		static_cast<float>(theDestRect.mHeight),
	};

	SDL_RenderTexture(mRenderer, sdlTex->GetSDLTexture(), &srcRect, &destRect);

	SDL_SetTextureBlendMode(sdlTex->GetSDLTexture(), SDL_BLENDMODE_NONE);
	SDL_SetRenderTarget(mRenderer, nullptr);
}
