#ifndef __APITESTER_HPP__
#define __APITESTER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include <SDL3/SDL.h>

namespace PopLib
{
    class APITester
    {
    public:
        static bool IsSDLRendererAvailable(SDL_Window* window);

        static bool IsOpenGLAvailable(SDL_Window* window);

        static bool IsDirect3DAvailable(SDL_Window* window);
    };

} // namespace PopLib

#endif // __APITESTER_HPP__