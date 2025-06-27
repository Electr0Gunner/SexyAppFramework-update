#include "sdlinterface.hpp"
#include "appbase.hpp"
#include "renderutils.hpp"
#include "misc/autocrit.hpp"

using namespace PopLib;

SDL_PixelFormat ToSDLPixelFormat(PixelFormat format)
{
    switch (format)
    {
        case PixelFormat_A8R8G8B8:
            return SDL_PIXELFORMAT_ARGB8888;

        case PixelFormat_A4R4G4B4:
            return SDL_PIXELFORMAT_ARGB4444;

        case PixelFormat_R5G6B5:
            return SDL_PIXELFORMAT_RGB565;

        case PixelFormat_Palette8:
            return SDL_PIXELFORMAT_INDEX8;

        case PixelFormat_Unknown:
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

SDLInterface::SDLInterface(AppBase* theApp) : Renderer(theApp)
{
	mAPI = API_SDL3;
}

SDLInterface::~SDLInterface()
{

}

void SDLInterface::UpdateViewport()
{

}

void SDLInterface::Cleanup()
{
	ImageSet::iterator anItr;
	for(anItr = mImageSet.begin(); anItr != mImageSet.end(); ++anItr)
	{
		MemoryImage *anImage = *anItr;
		delete (SDLTextureData*)anImage->mTextureData;
		anImage->mTextureData = NULL;
	}

	mImageSet.clear();

	if (mBackendRenderer)
		SDL_DestroyRenderer(mBackendRenderer);

	if (mBackendWindow)
		SDL_DestroyWindow(mBackendWindow);
}

void SDLInterface::Draw(RenderCommand command)
{
	SDL_SetRenderTarget(mBackendRenderer, mRenderBuffer);

	MemoryImage* aSrcMemoryImage = (MemoryImage*) command.image;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SDLTextureData *aData = (SDLTextureData*)aSrcMemoryImage->mTextureData;

	if (command.clip != Rect(-1, -1, -1, -1))
	{
		SDL_Rect clipRect;
		clipRect = {command.clip.mX, command.clip.mY, command.clip.mWidth, command.clip.mHeight};
		SDL_SetRenderClipRect(mBackendRenderer, &clipRect);
	}

	aData->Blt(mBackendRenderer, command.dst.mX, command.dst.mY, command.src, command.color);

	
	SDL_SetRenderClipRect(mBackendRenderer, nullptr);
	SDL_SetRenderTarget(mBackendRenderer, nullptr);
}

void SDLInterface::DrawTriangles(TriangleCommand command)
{

}

bool SDLInterface::CreateImageTexture(MemoryImage *theImage)
{
	bool wantPurge = false;

	if(theImage->mTextureData==NULL)
	{
		theImage->mTextureData = new SDLTextureData();
		
		// The actual purging was deferred
		wantPurge = theImage->mPurgeBits;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.insert(theImage);
	}

	SDLTextureData *aData = (SDLTextureData*)theImage->mTextureData;
	aData->CheckCreateTextures(theImage, mBackendRenderer);
	
	if (wantPurge)
		theImage->PurgeBits();

	return aData->mPixelFormat != PixelFormat_Unknown;
}

void SDLInterface::RemoveMemoryImage(MemoryImage *theImage)
{
	if (theImage->mTextureData != NULL)
	{
		delete (SDLTextureData*)theImage->mTextureData;
		theImage->mTextureData = NULL;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.erase(theImage);
	}
}

bool SDLInterface::RecoverBits(MemoryImage* theImage)
{
	if (theImage->mTextureData == NULL)
		return false;

	SDLTextureData* aData = (SDLTextureData*) theImage->mTextureData;
	if (aData->mBitsChangedCount != theImage->mBitsChangedCount) // bits have changed since texture was created
		return false;
	
	for (int aPieceRow = 0; aPieceRow < aData->mTexVecHeight; aPieceRow++)
	{
		for (int aPieceCol = 0; aPieceCol < aData->mTexVecWidth; aPieceCol++)
		{
			TextureDataPiece* aPiece = &aData->mTextures[aPieceRow*aData->mTexVecWidth + aPieceCol];

			if (SDL_LockSurface(aPiece->mSurface))
				return false;

			int offx = aPieceCol*aData->mTexPieceWidth;
			int offy = aPieceRow*aData->mTexPieceHeight;
			int aWidth = std::min(theImage->mWidth-offx, aPiece->mWidth);
			int aHeight = std::min(theImage->mHeight-offy, aPiece->mHeight);

			switch (aData->mPixelFormat)
			{
			case PixelFormat_A8R8G8B8:	CopyTexture8888ToImage(aPiece->mSurface, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_A4R4G4B4:	CopyTexture4444ToImage(aPiece->mSurface, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_R5G6B5: CopyTexture565ToImage(aPiece->mSurface,theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_Palette8:	CopyTexturePalette8ToImage(aPiece->mSurface,theImage, offx, offy, aWidth, aHeight, aData->mPalette); break;
			}

			SDL_UnlockSurface(aPiece->mSurface);

			if (aPiece->mTexture)
				SDL_DestroyTexture(aPiece->mTexture);
			
			aPiece->mTexture = SDL_CreateTextureFromSurface(mBackendRenderer, aPiece->mSurface);
		}
	}

	return true;
}

static void CopyImageToTexture8888(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad)
{
    uint8_t* dstRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 4;

	if (theImage->mColorTable == NULL)
	{
		ulong *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

        for (int y = 0; y < theHeight; ++y)
        {
            ulong* src = srcRow;
            ulong* dst = reinterpret_cast<ulong*>(dstRow);

            for (int x = 0; x < theWidth; ++x)
                *dst++ = *src++;

            if (rightPad)
                *dst = *(dst - 1);

            srcRow += theImage->GetWidth();
            dstRow += theSurface->pitch;
        }
	}
	else // palette
	{
        uint8_t* srcRow = theImage->mColorIndices + offy * theImage->GetWidth() + offx;
        ulong* palette = theImage->mColorTable;

        for (int y = 0; y < theHeight; ++y)
        {
            uint8_t* src = srcRow;
            ulong* dst = reinterpret_cast<ulong*>(dstRow);

            for (int x = 0; x < theWidth; ++x)
                *dst++ = palette[*src++];

            if (rightPad)
                *dst = *(dst - 1);

            srcRow += theImage->GetWidth();
            dstRow += theSurface->pitch;
        }
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyTexture8888ToImage(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight)
{		
    uint8_t* srcRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 4;
    ulong* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for(int y=0; y<theHeight; y++)
	{
		ulong *src = (ulong*)srcRow;
		ulong *dst = dstRow;
		
		for(int x=0; x<theWidth; x++)
			*dst++ = *src++;		

		dstRow += theImage->GetWidth();
		srcRow += theSurface->pitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexture4444(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad)
{
    uint8_t* dstRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 2;

	if (theImage->mColorTable == NULL)
	{
        ulong* srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

		for(int y=0; y<theHeight; y++)
		{
			ulong *src = srcRow;
			ushort *dst = (ushort*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				ulong aPixel = *src++;
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theSurface->pitch;
		}
	}
	else // palette
	{
		uint8_t *srcRow = (uchar*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		ulong *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uchar *src = srcRow;
			ushort *dst = (ushort*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				ulong aPixel = palette[*src++];
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theSurface->pitch;
		}

	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyTexture4444ToImage(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight)
{		
    uint8_t* srcRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 2;
    uint32_t* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for(int y=0; y<theHeight; y++)
	{
		uint16_t  *src = (uint16_t *)srcRow;
		ulong *dst = dstRow;
		
		for(int x=0; x<theWidth; x++)
		{
			uint16_t  aPixel = *src++;			
			*dst++ = 0xFF000000 | ((aPixel & 0xF000) << 16) | ((aPixel & 0x0F00) << 12) | ((aPixel & 0x00F0) << 8) | ((aPixel & 0x000F) << 4);
		}

		dstRow += theImage->GetWidth();
		srcRow += theSurface->pitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexture565(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad)
{
    uint8_t* dstRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 2;

	if (theImage->mColorTable == NULL)
	{
		ulong *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

		for(int y=0; y<theHeight; y++)
		{
			ulong *src = srcRow;
			uint16_t *dst = (uint16_t*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				ulong aPixel = *src++;
				*dst++ = ((aPixel>>8)&0xF800) | ((aPixel>>5)&0x07E0) | ((aPixel>>3)&0x001F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theSurface->pitch;
		}
	}
	else // palette
	{
        uint8_t* srcRow = theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		ulong *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint16_t *dst = (uint16_t*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				ulong aPixel = palette[*src++];
				*dst++ = ((aPixel>>8)&0xF800) | ((aPixel>>5)&0x07E0) | ((aPixel>>3)&0x001F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theSurface->pitch;
		}

	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyTexture565ToImage(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight)
{		
    uint8_t* srcRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx * 2;
    uint32_t* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for(int y=0; y<theHeight; y++)
	{
		uint16_t *src = (uint16_t*)srcRow;
		ulong *dst = dstRow;
		
		for(int x=0; x<theWidth; x++)
		{
			uint16_t aPixel = *src++;			
			*dst++ = 0xFF000000 | ((aPixel & 0xF800) << 8) | ((aPixel & 0x07E0) << 5) | ((aPixel & 0x001F) << 3);
		}

		dstRow += theImage->GetWidth();
		srcRow += theSurface->pitch;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexturePalette8(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad)
{
    uint8_t* srcRow = theImage->mColorIndices + offy * theImage->GetWidth() + offx;
    uint8_t* dstRow = static_cast<uint8_t*>(theSurface->pixels) + offy * theSurface->pitch + offx;

	for(int y=0; y<theHeight; y++)
	{
		uchar *src = srcRow;
		uchar *dst = dstRow;
		for(int x=0; x<theWidth; x++)
			*dst++ = *src++;

		if (rightPad) 
			*dst = *(dst-1);

		srcRow += theImage->GetWidth();
		dstRow += theSurface->pitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyTexturePalette8ToImage(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, SDL_Palette* thePalette)
{
    SDL_Color* paletteEntries = thePalette->colors;
    int pitch = theSurface->pitch;
    uint8_t* srcRow = static_cast<uint8_t*>(theSurface->pixels);
    ulong* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

    for (int y = 0; y < theHeight; ++y)
    {
        uint8_t* src = srcRow;
        ulong* dst = dstRow;

        for (int x = 0; x < theWidth; ++x)
        {
            SDL_Color color = paletteEntries[*src++];
            *dst++ = (color.a << 24) | (color.r << 16) | (color.g << 8) | (color.b);
        }

        srcRow += pitch;
        dstRow += theImage->GetWidth();
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexture(SDL_Surface* theSurface, MemoryImage *theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat)
{
	if (theSurface == NULL)
		return;

	if (SDL_LockSurface(theSurface))
		return;

	int aWidth = std::min(texWidth,(theImage->GetWidth()-offx));
	int aHeight = std::min(texHeight,(theImage->GetHeight()-offy));

	bool rightPad = aWidth<texWidth;
	bool bottomPad = aHeight<texHeight;


	if(aWidth>0 && aHeight>0)
	{
		switch (theFormat)
		{
			case PixelFormat_A8R8G8B8:	CopyImageToTexture8888(theSurface, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat_A4R4G4B4:	CopyImageToTexture4444(theSurface, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat_R5G6B5:	CopyImageToTexture565(theSurface, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat_Palette8:	CopyImageToTexturePalette8(theSurface, theImage, offx, offy, aWidth, aHeight, rightPad); break;
		}

		if (bottomPad)
		{
			uint8_t* base = static_cast<uint8_t*>(theSurface->pixels);
			int pitch = theSurface->pitch;

			uint8_t* lastRow = base + pitch * (offy + aHeight - 1);
			uint8_t* padRow  = base + pitch * (offy + aHeight);

			if ((offy + aHeight) < theSurface->h)
				memcpy(padRow, lastRow, pitch);


		}
	}

	SDL_UnlockSurface(theSurface);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SDLTextureData::SDLTextureData()
{
	mWidth = 0;
	mHeight = 0;
	mTexVecWidth = 0;
	mTexVecHeight = 0;
	mBitsChangedCount = 0;
	mTexMemSize = 0;
	mTexPieceWidth = 64;
	mTexPieceHeight = 64;

	mPalette = NULL;
	mPixelFormat = PixelFormat_Unknown;
	mImageFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SDLTextureData::~SDLTextureData()
{
	ReleaseTextures();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::ReleaseTextures()
{
	for(int i=0; i<(int)mTextures.size(); i++)
	{
		SDL_Surface *aSurface = mTextures[i].mSurface;
		SDL_Texture *aTexture = mTextures[i].mTexture;
		if (aSurface != nullptr)
			SDL_DestroySurface(aSurface);
		if (aTexture != nullptr)
			SDL_DestroyTexture(aTexture);
	}
	

	mTextures.clear();

	mTexMemSize = 0;

	if (mPalette!=NULL)
	{
        SDL_DestroyPalette(mPalette);
		mPalette = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::CreateTextureDimensions(MemoryImage *theImage)
{
	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();
	int i;
/**/
	// Calculate inner piece sizes
	mTexPieceWidth = aWidth;
	mTexPieceHeight = aHeight;
	bool usePow2 = true; //gTextureSizeMustBePow2 || mPixelFormat==PixelFormat_Palette8;
	GetBestTextureDimensions(mTexPieceWidth, mTexPieceHeight,false,usePow2,mImageFlags);

	// Calculate right boundary piece sizes
	int aRightWidth = aWidth%mTexPieceWidth;
	int aRightHeight = mTexPieceHeight;
	if (aRightWidth > 0)
		GetBestTextureDimensions(aRightWidth, aRightHeight,true,usePow2,mImageFlags);
	else
		aRightWidth = mTexPieceWidth;

	// Calculate bottom boundary piece sizes
	int aBottomWidth = mTexPieceWidth;
	int aBottomHeight = aHeight%mTexPieceHeight;
	if (aBottomHeight > 0)
		GetBestTextureDimensions(aBottomWidth, aBottomHeight,true,usePow2,mImageFlags);
	else
		aBottomHeight = mTexPieceHeight;

	// Calculate corner piece size
	int aCornerWidth = aRightWidth;
	int aCornerHeight = aBottomHeight;
	GetBestTextureDimensions(aCornerWidth, aCornerHeight,true,usePow2,mImageFlags);
/**/

//	mTexPieceWidth = 64;
//	mTexPieceHeight = 64;


	// Allocate texture array
	mTexVecWidth = (aWidth + mTexPieceWidth - 1)/mTexPieceWidth;
	mTexVecHeight = (aHeight + mTexPieceHeight - 1)/mTexPieceHeight;
	mTextures.resize(mTexVecWidth*mTexVecHeight);
	
	// Assign inner pieces
	for(i=0; i<(int)mTextures.size(); i++)
	{
		TextureDataPiece &aPiece = mTextures[i];
		aPiece.mSurface = NULL;
		aPiece.mWidth = mTexPieceWidth;
		aPiece.mHeight = mTexPieceHeight;
	}

	// Assign right pieces
/**/
	for(i=mTexVecWidth-1; i<(int)mTextures.size(); i+=mTexVecWidth)
	{
		TextureDataPiece &aPiece = mTextures[i];
		aPiece.mWidth = aRightWidth;
		aPiece.mHeight = aRightHeight;
	}

	// Assign bottom pieces
	for(i=mTexVecWidth*(mTexVecHeight-1); i<(int)mTextures.size(); i++)
	{
		TextureDataPiece &aPiece = mTextures[i];
		aPiece.mWidth = aBottomWidth;
		aPiece.mHeight = aBottomHeight;
	}

	// Assign corner piece
	mTextures.back().mWidth = aCornerWidth;
	mTextures.back().mHeight = aCornerHeight;
/**/

	mMaxTotalU = aWidth/(float)mTexPieceWidth;
	mMaxTotalV = aHeight/(float)mTexPieceHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::CreateTextures(MemoryImage *theImage, SDL_Renderer* theRenderer)
{
	theImage->DeleteSWBuffers(); // don't need these buffers for 3d drawing

	// Choose appropriate pixel format
	PixelFormat aFormat = PixelFormat_A8R8G8B8;
	//theImage->mD3DFlags = D3DImageFlag_UseA4R4G4B4;

	theImage->CommitBits();
	if (!theImage->mHasAlpha && !theImage->mHasTrans && (gSupportedPixelFormats & PixelFormat_R5G6B5))
	{
		if (!(theImage->mImageFlags & Flag_UseA8R8G8B8))
			aFormat = PixelFormat_R5G6B5;
	}

	SDL_Palette* aPalette = SDL_CreatePalette(256);
	if (theImage->mColorIndices != NULL && (gSupportedPixelFormats & PixelFormat_Palette8) && aPalette != nullptr)
	{
		SDL_Color colors[256];
		for (int i=0; i<256; i++)
		{
            ulong aPixel = theImage->mColorTable[i];
            colors[i].r = (aPixel >> 16) & 0xFF;
            colors[i].g = (aPixel >> 8) & 0xFF;
            colors[i].b = (aPixel >> 0) & 0xFF;
            colors[i].a = (aPixel >> 24) & 0xFF;
		}

        if (SDL_SetPaletteColors(aPalette, colors, 0, 256) == 0)
        {
            aFormat = PixelFormat_Palette8;
        }
        else
        {
            // Error setting palette colors
            gSupportedPixelFormats &= ~PixelFormat_Palette8;
            SDL_DestroyPalette(aPalette);
        }
	}

	if ((theImage->mImageFlags & Flag_UseA4R4G4B4) && aFormat==PixelFormat_A8R8G8B8 && (gSupportedPixelFormats & PixelFormat_A4R4G4B4))
		aFormat = PixelFormat_A4R4G4B4;

	if (aFormat==PixelFormat_A8R8G8B8 && !(gSupportedPixelFormats & PixelFormat_A8R8G8B8))
		aFormat = PixelFormat_A4R4G4B4;


	// Release texture if image size has changed
	bool createTextures = false;
	if (mWidth!=theImage->mWidth || mHeight!=theImage->mHeight || aFormat!=mPixelFormat || theImage->mImageFlags!=mImageFlags)
	{
		ReleaseTextures();

		mPixelFormat = aFormat;
		mImageFlags = theImage->mImageFlags;
		CreateTextureDimensions(theImage);
		createTextures = true;
	}

	mPalette =  aPalette;

	int i,x,y;

	int aHeight = theImage->GetHeight();
	int aWidth = theImage->GetWidth();

	if (mPalette!=NULL)
		mTexMemSize += 256*4;

	int aFormatSize = 4;
	if (aFormat==PixelFormat_Palette8)
		aFormatSize = 1;
	else if (aFormat==PixelFormat_R5G6B5)
		aFormatSize = 2;
	else if (aFormat==PixelFormat_A4R4G4B4)
		aFormatSize = 2;

	i=0;
	for(y=0; y<aHeight; y+=mTexPieceHeight)
	{
		for(x=0; x<aWidth; x+=mTexPieceWidth, i++)
		{
			TextureDataPiece &aPiece = mTextures[i];
			if (createTextures)
			{
				SDL_Surface* aSurface = SDL_CreateSurface(aPiece.mWidth, aPiece.mHeight, ToSDLPixelFormat(aFormat));
				SDL_SetSurfacePalette(aSurface, mPalette);
				aPiece.mSurface = aSurface;

				if (aPiece.mSurface == NULL) // create texture failure
				{
					mPixelFormat = PixelFormat_Unknown;
					return;
				}
					
				mTexMemSize += aPiece.mWidth*aPiece.mHeight*aFormatSize;
			}

			CopyImageToTexture(aPiece.mSurface ,theImage,x,y,aPiece.mWidth,aPiece.mHeight,aFormat);

			aPiece.mTexture = SDL_CreateTextureFromSurface(theRenderer, aPiece.mSurface);
		}
	}

	mWidth = theImage->mWidth;
	mHeight = theImage->mHeight;
	mBitsChangedCount = theImage->mBitsChangedCount;
	mPixelFormat = aFormat;
}
	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::CheckCreateTextures(MemoryImage *theImage, SDL_Renderer* theRenderer)
{
	if(mPixelFormat==PixelFormat_Unknown || theImage->mWidth != mWidth || theImage->mHeight != mHeight || theImage->mBitsChangedCount != mBitsChangedCount || theImage->mImageFlags != mImageFlags)
		CreateTextures(theImage, theRenderer);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int SDLTextureData::GetMemSize()
{
	int aSize = 0;

	aSize = (SDL_BYTESPERPIXEL(ToSDLPixelFormat(mPixelFormat)) / 8) * mWidth * mHeight;

	return aSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SDL_Texture* SDLTextureData::GetTexture(int x, int y, int &width, int &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	int left = x%mTexPieceWidth;
	int top = y%mTexPieceHeight;
	int right = left+width;
	int bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SDL_Texture* SDLTextureData::GetTextureF(float x, float y, float &width, float &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	float left = x - tx*mTexPieceWidth;
	float top = y - ty*mTexPieceHeight;
	float right = left+width;
	float bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::Blt(SDL_Renderer* theRenderer, float theX, float theY, const Rect& theSrcRect, const Color& theColor)
{
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth,aHeight;
	float u1,v1,u2,v2;

	srcY = srcTop;
	dstY = theY;

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	while(srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = theX;
		while(srcX < srcRight)
		{
			aWidth = srcRight-srcX;
			aHeight = srcBottom-srcY;
			SDL_Texture* aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			float x = dstX - 0.5f;
			float y = dstY - 0.5f;

			SDL_FColor color = SDL_FColor{(float)theColor.mRed, (float)theColor.mBlue, (float)theColor.mGreen, (float)theColor.mAlpha};

            SDL_Vertex verts[4] = {
                { SDL_FPoint{ x,     y },     				color, SDL_FPoint{ u1, v1 } },
                { SDL_FPoint{ x,     y + aHeight },	 		color, SDL_FPoint{ u1, v2 } },
                { SDL_FPoint{ x + aWidth, y }, 				color, SDL_FPoint{ u2, v1 } },
                { SDL_FPoint{ x + aWidth, y + aHeight }, 	color, SDL_FPoint{ u2, v2 } }
            };

			int triIndices[6] = { 0, 1, 2, 2, 1, 3 };
			
			SDL_RenderGeometry(theRenderer, aTexture, verts, 4, triIndices, 6);

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define GetColorFromTriVertex(theVertex, theColor) (theVertex.color?theVertex.color:theColor)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SDLTextureData::BltTriangles(SDL_Renderer* theRenderer, const TriVertex theVertices[][3], int theNumTriangles, ulong theColor, float tx, float ty)
{	
	if ((mMaxTotalU <= 1.0) && (mMaxTotalV <= 1.0))
	{
        std::vector<SDL_Vertex> vertexCache;
        vertexCache.reserve(3 * 100);

        for (int triNum = 0; triNum < theNumTriangles; ++triNum)
        {
            const TriVertex* triVerts = theVertices[triNum];

            for (int v = 0; v < 3; ++v)
            {
				SDL_FColor color;
				color.a = (GetColorFromTriVertex(triVerts[v], theColor) >> 24) & 0xFF;
				color.r = (GetColorFromTriVertex(triVerts[v], theColor) >> 16) & 0xFF;
				color.g = (GetColorFromTriVertex(triVerts[v], theColor) >> 8)  & 0xFF;
				color.b = (GetColorFromTriVertex(triVerts[v], theColor))       & 0xFF;

                SDL_Vertex vert;
                vert.position.x = triVerts[v].x + tx;
                vert.position.y = triVerts[v].y + ty;
                vert.color = color;
                vert.tex_coord.x = triVerts[v].u * mMaxTotalU;
                vert.tex_coord.y = triVerts[v].v * mMaxTotalV;

                vertexCache.push_back(vert);
            }

            // Flush if full or last triangle
            if (vertexCache.size() == 3 * 100 || triNum == theNumTriangles - 1)
            {
                SDL_Texture* texture = mTextures[0].mTexture;
                if (texture)
                {
                    // No index buffer needed; draws triangle list from consecutive vertices
                    SDL_RenderGeometry(theRenderer, texture, vertexCache.data(), (int)vertexCache.size(), nullptr, 0);
                }
                vertexCache.clear();
            }
        }
	}
	else
	{
		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			const TriVertex* triVerts = theVertices[aTriangleNum];

            SDL_Vertex sdlTri[3];
            float minU = mMaxTotalU, minV = mMaxTotalV;
            float maxU = 0, maxV = 0;

            for (int i = 0; i < 3; ++i)
            {
				SDL_FColor color;
				color.a = (GetColorFromTriVertex(triVerts[i], theColor) >> 24) & 0xFF;
				color.r = (GetColorFromTriVertex(triVerts[i], theColor) >> 16) & 0xFF;
				color.g = (GetColorFromTriVertex(triVerts[i], theColor) >> 8)  & 0xFF;
				color.b = (GetColorFromTriVertex(triVerts[i], theColor))       & 0xFF;

                sdlTri[i].position.x = triVerts[i].x + tx;
                sdlTri[i].position.y = triVerts[i].y + ty;
                sdlTri[i].color = color;
                sdlTri[i].tex_coord.x = triVerts[i].u * mMaxTotalU;
                sdlTri[i].tex_coord.y = triVerts[i].v * mMaxTotalV;

                if (sdlTri[i].tex_coord.x < minU) minU = sdlTri[i].tex_coord.x;
                if (sdlTri[i].tex_coord.y < minV) minV = sdlTri[i].tex_coord.y;
                if (sdlTri[i].tex_coord.x > maxU) maxU = sdlTri[i].tex_coord.x;
                if (sdlTri[i].tex_coord.y > maxV) maxV = sdlTri[i].tex_coord.y;
            }
			
            int left = (int)floorf(minU);
            int top = (int)floorf(minV);
            int right = (int)ceilf(maxU);
            int bottom = (int)ceilf(maxV);

            left = std::max(left, 0);
            top = std::max(top, 0);
            right = std::min(right, mTexVecWidth);
            bottom = std::min(bottom, mTexVecHeight);

			TextureDataPiece &aStandardPiece = mTextures[0];
			for (int tyIdx = top; tyIdx < bottom; ++tyIdx)
            {
                for (int txIdx = left; txIdx < right; ++txIdx)
                {
                    TextureDataPiece& piece = mTextures[tyIdx * mTexVecWidth + txIdx];

                    SDL_Vertex localVerts[3];
                    for (int k = 0; k < 3; ++k)
                    {
                        localVerts[k] = sdlTri[k];

                        localVerts[k].tex_coord.x -= float(txIdx);
                        localVerts[k].tex_coord.y -= float(tyIdx);

                        if (tyIdx == mTexVecHeight - 1)
                            localVerts[k].tex_coord.y *= float(mTextures[0].mHeight) / piece.mHeight;
                        if (txIdx == mTexVecWidth - 1)
                            localVerts[k].tex_coord.x *= float(mTextures[0].mWidth) / piece.mWidth;
                    }

                    if (piece.mTexture)
                    {
                        SDL_RenderGeometry(theRenderer, piece.mTexture, localVerts, 3, nullptr, 0);
                    }
                }
            }
		}
	}
}
