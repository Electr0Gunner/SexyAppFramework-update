#include "imagelib.hpp"
#include "paklib/pakinterface.hpp"

#include <stb_image.h>
#include <stb_image_write.h>

#include <cmath>
#include <vector>

using namespace ImageLib;

Image::Image()
{
	mWidth = 0;
	mHeight = 0;
	mNumChannels = 4; // Default to have R, G, B and A channels.
	mBits = nullptr;
}

Image::~Image()
{
	delete mBits;
}

int Image::GetWidth()
{
	return mWidth;
}

int Image::GetHeight()
{
	return mHeight;
}

ulong *Image::GetBits()
{
	return mBits;
}

//////////////////////////////////////////////////////////////////////////
// PNG Pak Support

Image *GetImageSTB(const std::string &theFileName)
{
	PFILE *fp;

	if ((fp = p_fopen(theFileName.c_str(), "rb")) == nullptr)
		return nullptr;

	p_fseek(fp, 0, SEEK_END);
	size_t fileSize = p_ftell(fp);
	p_fseek(fp, 0, SEEK_SET);
	std::vector<uint8_t> data(fileSize);
	p_fread(data.data(), 1, fileSize, fp);
	p_fclose(fp);

	int width, height, num_channels;
	unsigned char *stb_image = stbi_load_from_memory(data.data(), fileSize, &width, &height, &num_channels, NULL);

	ulong *aBits = new ulong[width * height];
	for (int i = 0; i < width * height; ++i)
	{
		unsigned char *pixel = &stb_image[i * num_channels];

		unsigned char r = pixel[0];
		unsigned char g = pixel[1];
		unsigned char b = pixel[2];
		unsigned char a = (num_channels == 4) ? pixel[3] : 0xFF;

		aBits[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	Image *anImage = new Image();
	anImage->mWidth = width;
	anImage->mHeight = height;
	anImage->mBits = aBits;
	anImage->mNumChannels = num_channels;

	delete stb_image;

	return anImage;
}

int ReadBlobBlock(PFILE *fp, char *data)
{
	unsigned char aCount = 0;
	p_fread(&aCount, sizeof(char), 1, fp);
	p_fread(data, sizeof(char), aCount, fp);
	return aCount;
}

Image *GetGIFImage(const std::string &theFileName)
{
#define BitSet(byte, bit) (((byte) & (bit)) == (bit))
#define LSBFirstOrder(x, y) (((y) << 8) | (x))

	int opacity, status;

	int i;

	unsigned char *p;

	unsigned char background, c, flag, *global_colormap, header[1664], magick[12];

	unsigned int delay, dispose, global_colors, image_count, iterations;

	/*
	Open image file.
	*/

	PFILE *fp;

	if ((fp = p_fopen(theFileName.c_str(), "rb")) == nullptr)
		return nullptr;
	/*
	Determine if this is a GIF file.
	*/
	status = p_fread(magick, sizeof(char), 6, fp);

	if (((strncmp((char *)magick, "GIF87", 5) != 0) && (strncmp((char *)magick, "GIF89", 5) != 0)))
		return nullptr;

	global_colors = 0;
	global_colormap = (unsigned char *)nullptr;

	short pw;
	short ph;

	p_fread(&pw, sizeof(short), 1, fp);
	p_fread(&ph, sizeof(short), 1, fp);
	p_fread(&flag, sizeof(char), 1, fp);
	p_fread(&background, sizeof(char), 1, fp);
	p_fread(&c, sizeof(char), 1, fp);

	if (BitSet(flag, 0x80))
	{
		/*
		opacity global colormap.
		*/
		global_colors = 1 << ((flag & 0x07) + 1);
		global_colormap = new unsigned char[3 * global_colors];
		if (global_colormap == (unsigned char *)nullptr)
			return nullptr;

		p_fread(global_colormap, sizeof(char), 3 * global_colors, fp);
	}

	delay = 0;
	dispose = 0;
	iterations = 1;
	opacity = (-1);
	image_count = 0;

	for (;;)
	{
		if (p_fread(&c, sizeof(char), 1, fp) == 0)
			break;

		if (c == ';')
			break; /* terminator */
		if (c == '!')
		{
			/*
			GIF Extension block.
			*/
			p_fread(&c, sizeof(char), 1, fp);

			switch (c)
			{
			case 0xf9: {
				/*
				Read Graphics Control extension.
				*/
				while (ReadBlobBlock(fp, (char *)header) > 0)
					;

				dispose = header[0] >> 2;
				delay = (header[2] << 8) | header[1];
				if ((header[0] & 0x01) == 1)
					opacity = header[3];
				break;
			}
			case 0xfe: {
				char *comments;
				int length;

				/*
				Read Comment extension.
				*/
				comments = (char *)nullptr;
				for (;;)
				{
					length = ReadBlobBlock(fp, (char *)header);
					if (length <= 0)
						break;
					if (comments == nullptr)
					{
						comments = new char[length + 1];
						if (comments != (char *)nullptr)
							*comments = '\0';
					}

					header[length] = '\0';
					strcat(comments, (char *)header);
				}
				if (comments == (char *)nullptr)
					break;

				delete comments;
				break;
			}
			case 0xff: {
				int loop;

				/*
				Read Netscape Loop extension.
				*/
				loop = false;
				if (ReadBlobBlock(fp, (char *)header) > 0)
					loop = !strncmp((char *)header, "NETSCAPE2.0", 11);
				while (ReadBlobBlock(fp, (char *)header) > 0)
					if (loop)
						iterations = (header[2] << 8) | header[1];
				break;
			}
			default: {
				while (ReadBlobBlock(fp, (char *)header) > 0)
					;
				break;
			}
			}
		}

		if (c != ',')
			continue;

		if (image_count != 0)
		{
			/*
			Allocate next image structure.
			*/

			/*AllocateNextImage(image_info,image);
			if (image->next == (Image *) nullptr)
			{
			DestroyImages(image);
			return((Image *) nullptr);
			}
			image=image->next;
			MagickMonitor(LoadImagesText,TellBlob(image),image->filesize);*/
		}
		image_count++;

		short pagex;
		short pagey;
		short width;
		short height;
		int colors;
		bool interlaced;

		p_fread(&pagex, sizeof(short), 1, fp);
		p_fread(&pagey, sizeof(short), 1, fp);
		p_fread(&width, sizeof(short), 1, fp);
		p_fread(&height, sizeof(short), 1, fp);
		p_fread(&flag, sizeof(char), 1, fp);

		colors = !BitSet(flag, 0x80) ? global_colors : 1 << ((flag & 0x07) + 1);
		ulong *colortable = new ulong[colors];

		interlaced = BitSet(flag, 0x40);

		delay = 0;
		dispose = 0;
		iterations = 1;
		/*if (image_info->ping)
		{
		f (opacity >= 0)
		/image->matte=true;

		CloseBlob(image);
		return(image);
		}*/
		if ((width == 0) || (height == 0))
			return nullptr;
		/*
		Inititialize colormap.
		*/
		/*if (!AllocateImageColormap(image,image->colors))
		ThrowReaderException(ResourceLimitWarning,"Memory allocation failed",
		image);*/
		if (!BitSet(flag, 0x80))
		{
			/*
			Use global colormap.
			*/
			p = global_colormap;
			for (i = 0; i < (int)colors; i++)
			{
				int r = *p++;
				int g = *p++;
				int b = *p++;

				colortable[i] = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			// image->background_color=
			// image->colormap[Min(background,image->colors-1)];
		}
		else
		{
			unsigned char *colormap;

			/*
			Read local colormap.
			*/
			colormap = new unsigned char[3 * colors];

			int pos = p_ftell(fp);
			p_fread(colormap, sizeof(char), 3 * colors, fp);

			p = colormap;
			for (i = 0; i < (int)colors; i++)
			{
				int r = *p++;
				int g = *p++;
				int b = *p++;

				colortable[i] = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}
			delete colormap;
		}

		/*if (opacity >= (int) colors)
		{
		for (i=colors; i < (opacity+1); i++)
		{
		image->colormap[i].red=0;
		image->colormap[i].green=0;
		image->colormap[i].blue=0;
		}
		image->colors=opacity+1;
		}*/
		/*
		Decode image.
		*/
		// status=DecodeImage(image,opacity,exception);

		// if (global_colormap != (unsigned char *) nullptr)
		//  LiberateMemory((void **) &global_colormap);
		if (global_colormap != nullptr)
		{
			delete[] global_colormap;
			global_colormap = nullptr;
		}

		// while (image->previous != (Image *) nullptr)
		//     image=image->previous;

#define MaxStackSize 4096
#define NullCode (-1)

		int available, bits, code, clear, code_mask, code_size, count, end_of_information, in_code, offset, old_code,
			pass, y;

		int x;

		unsigned int datum;

		short *prefix;

		unsigned char data_size, first, *packet, *pixel_stack, *suffix, *top_stack;

		/*
		Allocate decoder tables.
		*/

		packet = new unsigned char[256];
		prefix = new short[MaxStackSize];
		suffix = new unsigned char[MaxStackSize];
		pixel_stack = new unsigned char[MaxStackSize + 1];

		/*
		Initialize GIF data stream decoder.
		*/
		p_fread(&data_size, sizeof(char), 1, fp);
		clear = 1 << data_size;
		end_of_information = clear + 1;
		available = clear + 2;
		old_code = NullCode;
		code_size = data_size + 1;
		code_mask = (1 << code_size) - 1;
		for (code = 0; code < clear; code++)
		{
			prefix[code] = 0;
			suffix[code] = (unsigned char)code;
		}
		/*
		Decode GIF pixel stream.
		*/
		datum = 0;
		bits = 0;
		c = 0;
		count = 0;
		first = 0;
		offset = 0;
		pass = 0;
		top_stack = pixel_stack;

		ulong *aBits = new ulong[width * height];

		unsigned char *c = nullptr;

		for (y = 0; y < (int)height; y++)
		{
			// q=SetImagePixels(image,0,offset,width,1);
			// if (q == (PixelPacket *) nullptr)
			// break;
			// indexes=GetIndexes(image);

			ulong *q = aBits + offset * width;

			for (x = 0; x < (int)width;)
			{
				if (top_stack == pixel_stack)
				{
					if (bits < code_size)
					{
						/*
						Load bytes until there is enough bits for a code.
						*/
						if (count == 0)
						{
							/*
							Read a new data block.
							*/
							int pos = p_ftell(fp);

							count = ReadBlobBlock(fp, (char *)packet);
							if (count <= 0)
								break;
							c = packet;
						}
						datum += (*c) << bits;
						bits += 8;
						c++;
						count--;
						continue;
					}
					/*
					Get the next code.
					*/
					code = datum & code_mask;
					datum >>= code_size;
					bits -= code_size;
					/*
					Interpret the code
					*/
					if ((code > available) || (code == end_of_information))
						break;
					if (code == clear)
					{
						/*
						Reset decoder.
						*/
						code_size = data_size + 1;
						code_mask = (1 << code_size) - 1;
						available = clear + 2;
						old_code = NullCode;
						continue;
					}
					if (old_code == NullCode)
					{
						*top_stack++ = suffix[code];
						old_code = code;
						first = (unsigned char)code;
						continue;
					}
					in_code = code;
					if (code >= available)
					{
						*top_stack++ = first;
						code = old_code;
					}
					while (code >= clear)
					{
						*top_stack++ = suffix[code];
						code = prefix[code];
					}
					first = suffix[code];
					/*
					Add a new string to the string table,
					*/
					if (available >= MaxStackSize)
						break;
					*top_stack++ = first;
					prefix[available] = old_code;
					suffix[available] = first;
					available++;
					if (((available & code_mask) == 0) && (available < MaxStackSize))
					{
						code_size++;
						code_mask += available;
					}
					old_code = in_code;
				}
				/*
				Pop a pixel off the pixel stack.
				*/
				top_stack--;

				int index = (*top_stack);

				*q = colortable[index];

				if (index == opacity)
					*q = 0;

				x++;
				q++;
			}

			if (!interlaced)
				offset++;
			else
			{
				switch (pass)
				{
				case 0:
				default: {
					offset += 8;
					if (offset >= height)
					{
						pass++;
						offset = 4;
					}
					break;
				}
				case 1: {
					offset += 8;
					if (offset >= height)
					{
						pass++;
						offset = 2;
					}
					break;
				}
				case 2: {
					offset += 4;
					if (offset >= height)
					{
						pass++;
						offset = 1;
					}
					break;
				}
				case 3: {
					offset += 2;
					break;
				}
				}
			}

			if (x < width)
				break;

			/*if (image->previous == (Image *) nullptr)
			if (QuantumTick(y,image->rows))
			MagickMonitor(LoadImageText,y,image->rows);*/
		}
		delete pixel_stack;
		delete suffix;
		delete prefix;
		delete packet;

		delete colortable;

		// if (y < image->rows)
		// failed = true;

		Image *anImage = new Image();

		anImage->mWidth = width;
		anImage->mHeight = height;
		anImage->mBits = aBits;

		// TODO: Change for animation crap
		p_fclose(fp);
		return anImage;
	}

	p_fclose(fp);

	return nullptr;
}

bool ImageLib::WriteImage(const std::string &theFileName, const std::string &theExtension, Image *theImage)
{
	uint8_t *data = new unsigned char[theImage->mWidth * theImage->mHeight * theImage->mNumChannels];

	int index = 0;
	for (int j = theImage->mHeight - 1; j >= 0; --j)
	{
		for (int i = 0; i < theImage->mWidth; ++i)
		{
			float r = (float)i / (float)theImage->mWidth;
			float g = (float)j / (float)theImage->mHeight;
			float b = 0.2f;
			int ir = int(255.99 * r);
			int ig = int(255.99 * g);
			int ib = int(255.99 * b);

			data[index++] = ir;
			data[index++] = ig;
			data[index++] = ib;
		}
	}

	if (stricmp(theExtension.c_str(), ".jpg") == 0)
		stbi_write_jpg((theFileName + theExtension).c_str(), theImage->mWidth, theImage->mHeight, 3, data, 100);
	else if (stricmp(theExtension.c_str(), ".jpeg") == 0)
		stbi_write_jpg((theFileName + ".jpeg").c_str(), theImage->mWidth, theImage->mHeight, 3, data, 100);
	else if (stricmp(theExtension.c_str(), ".png") == 0)
		stbi_write_png((theFileName + theExtension).c_str(), theImage->mWidth, theImage->mHeight,
					   theImage->mNumChannels, data, theImage->mWidth * theImage->mNumChannels);
	else if (stricmp(theExtension.c_str(), ".bmp") == 0)
		stbi_write_bmp((theFileName + theExtension).c_str(), theImage->mWidth, theImage->mHeight,
					   theImage->mNumChannels, data);
	else if (stricmp(theExtension.c_str(), ".tga") == 0)
		stbi_write_tga((theFileName + theExtension).c_str(), theImage->mWidth, theImage->mHeight,
					   theImage->mNumChannels, data);
	else
	{
		delete[] data;
		return false;
	}

	delete[] data;

	return true;
}

bool ImageLib::WriteImageRaw(const std::string &theFileName, const std::string &theExtension, unsigned char *theData,
							 int theWidth, int theHeight)
{

	if (stricmp(theExtension.c_str(), ".jpg") == 0)
		stbi_write_jpg((theFileName + theExtension).c_str(), theWidth, theHeight, 4, theData, 100);
	else if (stricmp(theExtension.c_str(), ".jpeg") == 0)
		stbi_write_jpg((theFileName + ".jpeg").c_str(), theWidth, theHeight, 4, theData, 100);
	else if (stricmp(theExtension.c_str(), ".png") == 0)
		stbi_write_png((theFileName + theExtension).c_str(), theWidth, theHeight, 4, theData, theWidth * 4);
	else if (stricmp(theExtension.c_str(), ".bmp") == 0)
		stbi_write_bmp((theFileName + theExtension).c_str(), theWidth, theHeight, 4, theData);
	else if (stricmp(theExtension.c_str(), ".tga") == 0)
		stbi_write_tga((theFileName + theExtension).c_str(), theWidth, theHeight, 4, theData);
	else
	{
		return false;
	}

	return true;
}

int ImageLib::gAlphaComposeColor = 0xFFFFFF;
bool ImageLib::gAutoLoadAlpha = true;

Image *ImageLib::GetImage(const std::string &theFilename, bool lookForAlphaImage)
{
	if (!gAutoLoadAlpha)
		lookForAlphaImage = false;

	int aLastDotPos = theFilename.rfind('.');
	int aLastSlashPos = std::max((int)theFilename.rfind('\\'), (int)theFilename.rfind('/'));

	std::string anExt;
	std::string aFilename;

	if (aLastDotPos > aLastSlashPos)
	{
		anExt = theFilename.substr(aLastDotPos, theFilename.length() - aLastDotPos);
		aFilename = theFilename.substr(0, aLastDotPos);
	}
	else
		aFilename = theFilename;

	Image *anImage = nullptr;

	if ((anImage == nullptr) && ((stricmp(anExt.c_str(), ".tga") == 0) || (anExt.length() == 0)))
		anImage = GetImageSTB(aFilename + ".tga");

	if ((anImage == nullptr) && ((stricmp(anExt.c_str(), ".jpg") == 0) || (anExt.length() == 0)))
		anImage = GetImageSTB(aFilename + ".jpg");

	if ((anImage == nullptr) && ((stricmp(anExt.c_str(), ".png") == 0) || (anExt.length() == 0)))
		anImage = GetImageSTB(aFilename + ".png");

	if ((anImage == nullptr) && ((stricmp(anExt.c_str(), ".gif") == 0) || (anExt.length() == 0)))
		anImage = GetGIFImage(aFilename + ".gif");

	// Check for alpha images
	Image *anAlphaImage = nullptr;
	if (lookForAlphaImage)
	{
		// Check _ImageName
		anAlphaImage = GetImage(theFilename.substr(0, aLastSlashPos + 1) + "_" +
									theFilename.substr(aLastSlashPos + 1, theFilename.length() - aLastSlashPos - 1),
								false);

		// Check ImageName_
		if (anAlphaImage == nullptr)
			anAlphaImage = GetImage(theFilename + "_", false);
	}

	// Compose alpha channel with image
	if (anAlphaImage != nullptr)
	{
		if (anImage != nullptr)
		{
			if ((anImage->mWidth == anAlphaImage->mWidth) && (anImage->mHeight == anAlphaImage->mHeight))
			{
				ulong *aBits1 = anImage->mBits;
				ulong *aBits2 = anAlphaImage->mBits;
				int aSize = anImage->mWidth * anImage->mHeight;

				for (int i = 0; i < aSize; i++)
				{
					*aBits1 = (*aBits1 & 0x00FFFFFF) | ((*aBits2 & 0xFF) << 24);
					++aBits1;
					++aBits2;
				}
			}

			delete anAlphaImage;
		}
		else if (gAlphaComposeColor == 0xFFFFFF)
		{
			anImage = anAlphaImage;

			ulong *aBits1 = anImage->mBits;

			int aSize = anImage->mWidth * anImage->mHeight;
			for (int i = 0; i < aSize; i++)
			{
				*aBits1 = (0x00FFFFFF) | ((*aBits1 & 0xFF) << 24);
				++aBits1;
			}
		}
		else
		{
			const int aColor = gAlphaComposeColor;
			anImage = anAlphaImage;

			ulong *aBits1 = anImage->mBits;

			int aSize = anImage->mWidth * anImage->mHeight;
			for (int i = 0; i < aSize; i++)
			{
				*aBits1 = aColor | ((*aBits1 & 0xFF) << 24);
				++aBits1;
			}
		}
	}

	return anImage;
}