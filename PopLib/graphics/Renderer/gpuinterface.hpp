#ifndef __GPUINTERFACE_HPP__
#define __GPUINTERFACE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "renderer.hpp"

#include <SDL3/SDL_gpu.h>

namespace PopLib
{
class GPUInterface : public Renderer
{

};
} // namespace PopLib

#endif // __GPUINTERFACE_HPP__