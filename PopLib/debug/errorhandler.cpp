#include "errorhandler.hpp"
#include "appbase.hpp"

using namespace PopLib;

ErrorHandler *PopLib::gErrorHandler = nullptr; // global error handler!!! :P

ErrorHandler::ErrorHandler(AppBase *theApp)
{
	gErrorHandler = this;

	mApp = theApp;
}

ErrorHandler::~ErrorHandler()
{
}

void ErrorHandler::OnException(const std::exception& e)
{
	SDL_Window* window = SDL_CreateWindow(
        "Error",
        600, 400,
        SDL_WINDOWPOS_CENTERED
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // Wait until closed
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }
        SDL_Delay(16);
    }

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}