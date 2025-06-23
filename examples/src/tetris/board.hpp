#ifndef __BOARD_HPP__
#define __BOARD_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "PopLib/widget/widget.hpp"
#include "PopLib/graphics/graphics.hpp"
#include "PopLib/widget/buttonlistener.hpp"
#include "PopLib/graphics/color.hpp"
#include "PopLib/math/point.hpp"
#include "PopLib/math/point.hpp"

namespace PopLib
{

class GameApp;

const int BoardWidth = 10;
const int BoardHeight = 20;
const int DAS = 10;
const int ARR = 2;

class Board : public Widget, public ButtonListener
{
  public:
	Board(GameApp *theApp);
	virtual ~Board();

	virtual void Update();
	virtual void Draw(Graphics *g);
	virtual void KeyDown(KeyCode key);
	virtual void KeyUp(KeyCode key);

  private:
	GameApp *mApp;

	int mGrid[BoardHeight][BoardWidth]; // -1 empty, else block type 0..6
	int mPieceType, mPieceRotation;
	int mPieceX, mPieceY;
	int mNextType;
	int mDropCounter, mDropSpeed;
	int mScore, mLevel, mLinesCleared;
	bool mLeftKey, mRightKey, mDownKey, mRotateKey;

	static int sShapes[7][4][4][4]; // [piece][rot][row][col] occupancy
	void SpawnPiece();
	bool CheckCollision(int newX, int newY, int newRot);
	void MergePieceToGrid();
	void ClearLines();

	bool mLeftHeld = false;
	bool mRightHeld = false;
	int mLeftTimer = -1;
	int mRightTimer = -1;
};

} // namespace PopLib

#endif // __BOARD_H__