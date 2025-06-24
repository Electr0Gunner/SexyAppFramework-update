#include "board.hpp"
#include "gameapp.hpp"

#include "PopLib/graphics/graphics.hpp"
#include "PopLib/graphics/color.hpp"
#include "PopLib/math/point.hpp"
#include "PopLib/graphics/sysfont.hpp"
#include "PopLib/misc/keycodes.hpp"
#include "res.hpp"
#include <cstdlib>
#include <ctime>
#include <string>

using namespace PopLib;

void DrawStringWithOutline(PopLib::Graphics* g, const std::string& text, int x, int y,
                           const PopLib::Color& outlineColor, const PopLib::Color& fillColor)
{
    g->SetFont(FONT_DEFAULT);

    g->SetColor(outlineColor);
    g->DrawString(text, x - 1, y);
    g->DrawString(text, x + 1, y);
    g->DrawString(text, x, y - 1);
    g->DrawString(text, x, y + 1);
    g->DrawString(text, x - 1, y - 1);
    g->DrawString(text, x + 1, y - 1);
    g->DrawString(text, x - 1, y + 1);
    g->DrawString(text, x + 1, y + 1);

    g->SetColor(fillColor);
    g->DrawString(text, x, y);
}

int Board::sShapes[7][4][4][4] = {
	// I
	{{{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
	 {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}}},
	// J
	{{{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}},
	// L
	{{{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
	 {{0, 0, 0, 0}, {1, 1, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}},
	 {{1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}},
	// O
	{{{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
	// S
	{{{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
	 {{0, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
	 {{1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}},
	// T
	{{{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}},
	// Z
	{{{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
	 {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
	 {{0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}}}};

Board::Board(GameApp *theApp)
{
	mApp = theApp;
	std::srand((unsigned)std::time(nullptr));
	// clear grid (-1 = empty)
	for (int r = 0; r < BoardHeight; r++)
		for (int c = 0; c < BoardWidth; c++)
			mGrid[r][c] = -1;
	mScore = 0;
	mLevel = 1;
	mLinesCleared = 0;
	mDropCounter = 0;
	mDropSpeed = 30; // initial fall speed (higher = slower)
	mLeftKey = mRightKey = mDownKey = mRotateKey = false;
	mNextType = std::rand() % 7;
	SpawnPiece();
}

Board::~Board()
{
}

void Board::SpawnPiece()
{
	mPieceType = mNextType;
	mNextType = std::rand() % 7;
	mPieceRotation = 0;
	mPieceX = BoardWidth / 2 - 2;
	mPieceY = 0;
	if (CheckCollision(mPieceX, mPieceY, mPieceRotation))
	{
		// reset board
		for (int r = 0; r < BoardHeight; r++)
			for (int c = 0; c < BoardWidth; c++)
				mGrid[r][c] = -1;
		mScore = mLinesCleared = 0;
		mLevel = 1;
		mDropSpeed = 30;
	}
}

bool Board::CheckCollision(int newX, int newY, int newRot)
{
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (sShapes[mPieceType][newRot][r][c])
			{
				int br = newY + r, bc = newX + c;
				if (br < 0 || br >= BoardHeight || bc < 0 || bc >= BoardWidth)
					return true;
				if (mGrid[br][bc] != -1)
					return true;
			}
		}
	}
	return false;
}

void Board::MergePieceToGrid()
{
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 4; c++)
			if (sShapes[mPieceType][mPieceRotation][r][c])
			{
				int br = mPieceY + r, bc = mPieceX + c;
				if (br >= 0 && br < BoardHeight && bc >= 0 && bc < BoardWidth)
					mGrid[br][bc] = mPieceType;
			}
	ClearLines();
}

void Board::ClearLines()
{
	int lines = 0;
	for (int r = 0; r < BoardHeight; r++)
	{
		bool full = true;
		for (int c = 0; c < BoardWidth; c++)
		{
			if (mGrid[r][c] == -1)
			{
				full = false;
				break;
			}
		}
		if (full)
		{
			lines++;
			// shift everything above down
			for (int rr = r; rr > 0; rr--)
				for (int cc = 0; cc < BoardWidth; cc++)
					mGrid[rr][cc] = mGrid[rr - 1][cc];
			for (int cc = 0; cc < BoardWidth; cc++)
				mGrid[0][cc] = -1;
		}
	}
	if (lines > 0)
	{
		// update score
		mScore += 100 * lines * mLevel;
		mLinesCleared += lines;
		int newLevel = mLinesCleared / 10 + 1;
		if (newLevel > mLevel)
		{
			mLevel = newLevel;
			mDropSpeed = std::max(5, mDropSpeed - 5);
		}
	}
}

void Board::Update()
{
	Widget::Update();

	// LEFT key repeat handling
	if (mLeftHeld)
	{
		mLeftTimer++;
		if (mLeftTimer == 0 || (mLeftTimer > DAS && (mLeftTimer - DAS) % ARR == 0))
		{
			if (!CheckCollision(mPieceX - 1, mPieceY, mPieceRotation))
			{
				mPieceX--;
			}
		}
	}

	// RIGHT key repeat handling
	if (mRightHeld)
	{
		mRightTimer++;
		if (mRightTimer == 0 || (mRightTimer > DAS && (mRightTimer - DAS) % ARR == 0))
		{
			if (!CheckCollision(mPieceX + 1, mPieceY, mPieceRotation))
			{
				mPieceX++;
			}
		}
	}

	// soft drop
	if (mDownKey)
	{
		if (!CheckCollision(mPieceX, mPieceY + 1, mPieceRotation))
		{
			mPieceY++;
		}
	}

	// rotation
	if (mRotateKey)
	{
		int newRot = (mPieceRotation + 1) % 4;
		if (!CheckCollision(mPieceX, mPieceY, newRot))
		{
			mPieceRotation = newRot;
		}
		mRotateKey = false;
	}

	// auto drop
	if (++mDropCounter >= mDropSpeed)
	{
		mDropCounter = 0;
		if (!CheckCollision(mPieceX, mPieceY + 1, mPieceRotation))
		{
			mPieceY++;
		}
		else
		{
			MergePieceToGrid();
			SpawnPiece();
		}
	}

	MarkDirty(); // redraw
}

void Board::Draw(Graphics *g)
{
	const int brickSize = 20;
	const int gridX = 20 * 15;
	const int gridY = 20 * 2;
	const int gridWidthPx = BoardWidth * brickSize;
	const int gridHeightPx = BoardHeight * brickSize;

	// black background slop
	g->SetColor(Color(0, 0, 0)); // black
	g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);

	g->SetColor(Color(255, 255, 255));
	g->SetColorizeImages(false);

	for (int y = 0; y < mHeight; y += brickSize)
	{
		for (int x = 0; x < mWidth; x += brickSize)
		{
			// skip drawing bricks in the grid area
			bool inGridArea = (x >= gridX && x < gridX + gridWidthPx && y >= gridY && y < gridY + gridHeightPx);
			bool isUiArea = (x >= gridX + gridWidthPx + brickSize * 2 && x < gridX + gridWidthPx + brickSize * 12 && y >= gridY && y < gridY + gridHeightPx);
			if (!inGridArea && !isUiArea)
			{
				g->DrawImage(IMAGE_BRICK, x, y);
			}
		}
	}

	g->SetColor(Color(50, 50, 50)); // light gray grid lines
	for (int r = 0; r <= BoardHeight; r++)
	{
		g->DrawLine(gridX, gridY + r * brickSize, gridX + gridWidthPx, gridY + r * brickSize);
	}
	for (int c = 0; c <= BoardWidth; c++)
	{
		g->DrawLine(gridX + c * brickSize, gridY, gridX + c * brickSize, gridY + gridHeightPx);
	}

	static Color blockColors[7] = {Color(0, 255, 255), Color(0, 0, 255),   Color(255, 165, 0), Color(255, 255, 0),
								   Color(0, 255, 0),   Color(255, 0, 255), Color(255, 0, 0)};

	// now, we draw the ghost piece
	int ghostY = mPieceY;
	while (!CheckCollision(mPieceX, ghostY + 1, mPieceRotation))
	{
		ghostY++;
	}
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (sShapes[mPieceType][mPieceRotation][r][c])
			{
				int x = gridX + (mPieceX + c) * brickSize;
				int y = gridY + (ghostY + r) * brickSize;
				Color ghostColor = blockColors[mPieceType];
				ghostColor.mAlpha = 100; // semi-transparent

				g->SetColor(ghostColor);
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_BRICK, x, y);
				g->SetColorizeImages(false);
			}
		}
	}

	// Draw grid blocks on top of black background
	for (int r = 0; r < BoardHeight; r++)
	{
		for (int c = 0; c < BoardWidth; c++)
		{
			int type = mGrid[r][c];
			if (type != -1)
			{
				int x = gridX + c * brickSize;
				int y = gridY + r * brickSize;
				g->SetColor(blockColors[type]);
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_BRICK, x, y);
				g->SetColorizeImages(false);
			}
		}
	}

	// Draw current piece
	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (sShapes[mPieceType][mPieceRotation][r][c])
			{
				int x = gridX + (mPieceX + c) * brickSize;
				int y = gridY + (mPieceY + r) * brickSize;
				g->SetColor(blockColors[mPieceType]);
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_BRICK, x, y);
				g->SetColorizeImages(false);
			}
		}
	}

	for (int r = 0; r < 4; r++)
	{
		for (int c = 0; c < 4; c++)
		{
			if (sShapes[mNextType][0][r][c]) // Always show rotation 0
			{
				int x = 600 + c * brickSize;
				int y = 200 + r * brickSize;
				g->SetColor(blockColors[mNextType]);
				g->SetColorizeImages(true);
				g->DrawImage(IMAGE_BRICK, x, y);
				g->SetColorizeImages(false);
			}
		}
	}

	// draw text
	DrawStringWithOutline(g, "Score: " + std::to_string(mScore), 600, 80, Color(0, 0, 0), Color(255, 255, 255));
	DrawStringWithOutline(g, "Level: " + std::to_string(mLevel), 600, 100, Color(0, 0, 0), Color(255, 255, 255));
	DrawStringWithOutline(g, "Lines: " + std::to_string(mLinesCleared), 600, 120, Color(0, 0, 0), Color(255, 255, 255));
	DrawStringWithOutline(g, "Next piece:", 600, 140, Color(0, 0, 0), Color(255, 255, 255));

	g->SetColor(Color(255, 255, 255));
#ifdef _DEBUG
	g->SetColor(Color(0, 0, 0, 140));
	std::string watermark = StrFormat("Made using PopLib v%s", POPLIB_VERSION);
	g->FillRect(Rect(0, (mY + mHeight) - FONT_DEFAULT->mHeight - 4, FONT_DEFAULT->StringWidth(watermark) + 8, FONT_DEFAULT->mHeight));
	g->SetColor(Color(255, 255, 255));
	g->DrawString(watermark, 4, (mY + mHeight) - (FONT_DEFAULT->mHeight - 12));
#endif
}

void Board::KeyDown(KeyCode key)
{
	if (key == KEYCODE_LEFT)
	{
		if (!mLeftHeld)
		{
			mLeftHeld = true;
			mLeftTimer = 0;
			if (!CheckCollision(mPieceX - 1, mPieceY, mPieceRotation))
				mPieceX--;
		}
	}
	else if (key == KEYCODE_RIGHT)
	{
		if (!mRightHeld)
		{
			mRightHeld = true;
			mRightTimer = 0;
			if (!CheckCollision(mPieceX + 1, mPieceY, mPieceRotation))
				mPieceX++;
		}
	}
	else if (key == KEYCODE_DOWN)
	{
		mDownKey = true;
	}
	else if (key == KEYCODE_UP)
	{
		mRotateKey = true;
	}
}

void Board::KeyUp(KeyCode key)
{
	if (key == KEYCODE_LEFT)
	{
		mLeftHeld = false;
		mLeftTimer = -1;
	}
	else if (key == KEYCODE_RIGHT)
	{
		mRightHeld = false;
		mRightTimer = -1;
	}
	else if (key == KEYCODE_DOWN)
	{
		mDownKey = false;
	}
}
