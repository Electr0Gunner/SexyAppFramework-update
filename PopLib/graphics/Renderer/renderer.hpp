#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "graphics/graphics.hpp"
#include "math/rect.hpp"
#include "graphics/color.hpp"
#include "math/point.hpp"
#include "math/trivertex.hpp"
#include "misc/critsect.hpp"
#include <vector>
#include <string>

namespace PopLib
{
  class Image;

  enum FlipMode
  {
    FLIP_NONE,
    FLIP_HORIZONTAL,
    FLIP_VERTICAL,
    FLIP_BOTH,
  };

  struct RendererInitData
  {
    int windowWidth;
    int windowHeight;
    std::string windowTitle;
    bool windowed;
  };

  struct RenderCommand
  {
    Image* image;
    Rect dst;
    Rect src;
    Color color;
    int drawMode;
    Rect clip;
    double rotation;
    Point center;
    FlipMode flipMode;
  };

  struct TriangleCommand
  {
    Image* image;
    std::vector<TriVertex> vertices;
    std::vector<int> indices;
    float transX;
    float transY;
    Color color;
    int drawMode;
    Rect clip;
  };

  enum RendererError
  {
    ERR_NONE = 0,
    ERR_CREATE_WINDOW,
    ERR_CREATE_RENDERER,
    ERR_CREATE_TEXTURE,
  };

  enum TextureFlags
  {
    Flag_MinimizeNumSubdivisions = 0x0001,		// subdivide image into fewest possible textures (may use more memory)
    Flag_Use64By64Subdivisions = 0x0002,		// good to use with image strips so the entire texture isn't pulled in when drawing just a piece
    Flag_UseA4R4G4B4 = 0x0004,		// images with not too many color gradients work well in this format
    Flag_UseA8R8G8B8 = 0x0008,		// non-alpha images will be stored as R5G6B5 by default so use this option if you want a 32-bit non-alpha image
    Flag_NearestFiltering = 0x0016 //use the nearest filtering for texture scaling.
  };
  

  class Renderer
  {
    public:
      RendererError mError = ERR_NONE;
      CritSect mCritSect;
  
    public:
      Renderer();
      virtual ~Renderer();

        virtual bool Init(const RendererInitData& initData);
        virtual void Cleanup();

        virtual void Render();

        virtual void UpdateViewport();

        virtual Image* GetScreenImage();

        virtual void Draw(RenderCommand command);
        virtual void DrawTriangles(TriangleCommand command);

        inline virtual RendererError GetError() {return mError;}
  };

} // namespace PopLib

#endif // __RENDERER_HPP__