//////////////////////////////////////////////////////////////////////////
//						main.cpp
//
//	This is the starting point for all new projects. This file's purpose is
//	pretty small, but important. In here we create our application, initialize
//	it, and begin processing all the game code.
//
//	This demo will teach you:
//	* Using Hue, Saturation, Luminance values instead of RGB for color
//	* Custom cursors
//	* Handling loss/gain of application focus
//	* Basic particle effects
//	* Various programmatic effects/logic
//	* Putting most of the knowledge from the previous demos to work in a game
//
//	IMPORTANT: Comments for topics covered in the previous demos will
//	be kept to a minimum and removed in most cases. You should read through
//	the previous demos to familiarize yourself with anything you don't
//	understand. All functions, variables, and concepts are explained
//	either in this demo, or in previous ones if they've already been covered.
//////////////////////////////////////////////////////////////////////////

#include "gameapp.hpp"
#ifndef _CONSOLE_ON
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#endif

using namespace PopLib;

int main(int argc, char *argv[])
{
#ifndef _CONSOLE_ON
#ifdef _WIN32
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
#endif
	GameApp *anApp = new GameApp();
	anApp->Init();
	anApp->Start();

	delete anApp;

	return 0;
}
