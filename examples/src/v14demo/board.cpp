#include "board.hpp"
#include "demowidget.hpp"
#include "res.hpp"
#include "v14demoapp.hpp"

#include "PopLib/debug/sehcatcher.hpp"
#include "PopLib/graphics/sysfont.hpp"
#include "PopLib/graphics/font.hpp"
#include "PopLib/graphics/graphics.hpp"
#include "PopLib/widget/buttonwidget.hpp"
#include "PopLib/graphics/HeaderFont/LiberationSansRegular.h"
#include "PopLib/widget/widgetmanager.hpp"
#include "PopLib/widget/dialog.hpp"
#include "PopLib/misc/flags.hpp"
#include "PopLib/imagelib/imagelib.hpp"
#include "PopLib/graphics/memoryimage.hpp"
#include "PopLib/math/matrix.hpp"
#include "PopLib/math/trivertex.hpp"
#include "PopLib/audio/soundmanager.hpp"
#include "PopLib/audio/soundinstance.hpp"

using namespace PopLib;

void crash()
{
#ifdef _WIN32
	__try {
        int a = 0;
        int b = 5 / a;
        (void)b;
    }
    __except(PopLib::SEHCatcher::UnhandledExceptionFilter(GetExceptionInformation())) {
    }
#else
    int a = 0;
    int b = 5 / a;
    (void)b; // this executes the catcher automatically
#endif
}

Board::Board(V14DemoApp* theApp)
{
	mApp = theApp;

	mWidgetFlagsMod.mAddFlags |= WIDGETFLAGS_MARK_DIRTY;

	SysFont* aFont = new SysFont(mApp, LiberationSans_Regular, LiberationSans_Regular_Size, 12, 0, false, false, false);

	mDemoButton = new ButtonWidget(0, this);
	mDemoButton->mLabel = "Demo Widget";
	mDemoButton->SetFont(aFont);
	mDemoButton->Resize(10, 10, 10 + aFont->StringWidth(mDemoButton->mLabel), 50);
	AddWidget(mDemoButton);

	mDialogButton = new ButtonWidget(1, this);
	mDialogButton->mLabel = "Do Dialog";
	mDialogButton->SetFont(FONT_DEFAULT);
	int w = FONT_DEFAULT->StringWidth(mDialogButton->mLabel);
	mDialogButton->Resize(mApp->mWidth - 20 - w, 10, w + 10, 50);
	AddWidget(mDialogButton);

	mMsgButton = new ButtonWidget(2, this);
	mMsgButton->mLabel = "Create Message Box";
	mMsgButton->SetFont(FONT_DEFAULT);
	w = FONT_DEFAULT->StringWidth(mMsgButton->mLabel);
	mMsgButton->Resize(mDemoButton->mX + 20 + mDemoButton->mWidth, 10, w + 10, 50);
	AddWidget(mMsgButton);

	mCrashButton = new ButtonWidget(3, this);
	mCrashButton->mLabel = "Crash the game";
	mCrashButton->SetFont(FONT_DEFAULT);
	w = FONT_DEFAULT->StringWidth(mCrashButton->mLabel);
	mCrashButton->Resize(mMsgButton->mX + 20 + mMsgButton->mWidth, 10, w + 10, 50);
	AddWidget(mCrashButton);

	mDemoWidget = NULL;

	mRect = Rect(mApp->mWidth / 2 - 1, mApp->mHeight / 2 - 1, 2, 2);
	mExpanding = true;
	mMsgX = Rand() % (mApp->mWidth - 100);
	mMsgY = Rand() % (mApp->mHeight - 100);
	mLostFocus = false;

	mCurtainWidth = 0;
	mCurtainMode = CURTAIN_INACTIVE;

	mDeferPriority = 0;
}

Board::~Board()
{
	RemoveAllWidgets();

	delete mDemoButton;
	delete mDialogButton;
	delete mMsgButton;
	delete mCrashButton;

	if (mDemoWidget != NULL)
		mApp->mWidgetManager->RemoveWidget(mDemoWidget);

	delete mDemoWidget;
}

void Board::Update()
{
	Widget::Update();

	if (mLostFocus)
	{
		if (mUpdateCnt % 100 == 0)
		{
			mMsgX = Rand() % (mApp->mWidth - 100);
			mMsgY = Rand() % (mApp->mHeight - 100);

			// Every second, we switch between drawing the overlay above or below
			// the mDemoWidget object (assuming of course you have the widget on screen
			//	when you lose focus).
			mDeferPriority = mDeferPriority == 0 ? 2 : 0;
		}
	}
	else
	{		
		// This just makes the rectangle in the middle grow/shrink. Nothing
		// too crazy.
		if (mExpanding)
		{
			mRect.mWidth += 2;
			mRect.mHeight += 2;

			if (mRect.mWidth >= mApp->mWidth)
			{
				mRect.mWidth = mApp->mWidth;
				mExpanding = false;
			}

			if (mRect.mHeight >= mApp->mHeight)
			{
				mRect.mHeight = mApp->mHeight;
				mExpanding = false;
			}
		}
		else
		{
			mRect.mWidth -= 2;
			mRect.mHeight -= 2;

			if (mRect.mHeight <= 0)
			{
				mRect.mHeight = 0;
				mExpanding = true;
			}

			if (mRect.mWidth <= 0)
			{
				mRect.mWidth = 0;
				mExpanding = true;
			}

		}

		mRect.mX = mApp->mWidth / 2 - mRect.mWidth / 2;
		mRect.mY = mApp->mHeight / 2 - mRect.mHeight / 2;
	}
}

void Board::Draw(Graphics* g)
{
	g->SetColor(Color::Black);
	g->FillRect(0, 0, mWidth, mHeight);

	g->SetColor(Color(255, 0, 0));
	g->FillRect(mRect);

	{
		GraphicsAutoState auto_state(g);
		g->SetScale(2.0f, 2.0f, 300.0f, 100.0f);
		g->DrawImage(IMAGE_TESTPIXEL, 300, 100);
	}


	if (!mLostFocus)
	{
		GraphicsAutoState auto_state(g);
		g->DrawImage(IMAGE_HUNGARR_LOGO, 10, 100);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(mUpdateCnt % 128, mUpdateCnt % 255, mUpdateCnt % 64));
		g->DrawImage(IMAGE_HUNGARR_LOGO, 10, 100);
	}

	if (mCurtainMode != CURTAIN_INACTIVE)
	{
		g->SetColor(Color(255, 255, 0));
		g->FillRect(0, 0, mCurtainWidth, mHeight);
		g->FillRect(mWidth - mCurtainWidth, 0, mCurtainWidth, mHeight);
	}

	if (mLostFocus)
		DeferOverlay(mDeferPriority);

}

void Board::DrawOverlay(Graphics* g)
{
	g->SetColor(Color(0, 0, 255, 175));
	g->FillRect(0, 0, mWidth, mHeight);

	g->SetFont(FONT_DEFAULT);
	g->SetColor(Color::White);
	g->DrawString("LOST FOCUS", mMsgX, mMsgY);	

	g->PushState();

	g->SetColorizeImages(true);
	g->SetColor(Color(0, 255, 255));
}

void Board::KeyChar(PopChar theChar)
{

}

void Board::KeyDown(KeyCode theKey)
{
	SDL_Log(GetKeyNameFromCode(theKey).c_str());
}


void Board::ButtonDepress(int id)
{
	if (id == mDemoButton->mId)
	{
		delete mDemoWidget;
		mDemoWidget = new DemoWidget();
		mApp->mWidgetManager->AddWidget(mDemoWidget);

		FlagsMod flags;
		flags.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
		mApp->mWidgetManager->AddBaseModal(mDemoWidget, flags);
	}
	else if (id == mDialogButton->mId)
	{

		Dialog* d = mApp->DoDialog(100, true, "Fun Dialog", "Line 1\nLine 2\nLine 3", "Close!", Dialog::BUTTONS_FOOTER);
		d->SetButtonFont(FONT_DEFAULT);

	}
	else if (id == mMsgButton->mId)
	{
		mApp->MsgBox("This is a SDL3 Message Box", "Test", MsgBoxFlags::MsgBox_ABORTRETRYIGNORE);

	}
	else if (id == mCrashButton->mId)
	{
		crash();
	}
}
