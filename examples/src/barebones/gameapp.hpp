#ifndef __GAMEAPP_HPP__
#define __GAMEAPP_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "PopLib/popapp.hpp"

namespace PopLib
{
class Board;

class GameApp : public PopApp
{
  public:
	GameApp();
	virtual ~GameApp();

	virtual void Init();

	virtual void LoadingThreadProc();

	virtual void LoadingThreadCompleted();

  private:
	Board *mBoard;
};

} // namespace PopLib

#endif // __GAMEAPP_H__