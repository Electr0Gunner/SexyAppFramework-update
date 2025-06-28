#include "apitester.hpp"

using namespace PopLib;

bool APITester::IsSDLRendererAvailable(SDL_Window* window)
{
    return SDL_GetNumRenderDrivers() > 0;
}

bool APITester::IsOpenGLAvailable(SDL_Window* window)
{
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr)
        return false;
    
    SDL_GL_DestroyContext(context);

    return true;
}

bool APITester::IsDirect3DAvailable(SDL_Window* window)
{
#ifdef _WIN32
    return false; //TODO: ADD Direct3D renderer
#else
    return false; //Not using windows / aka Direct3D is not available
#endif
}