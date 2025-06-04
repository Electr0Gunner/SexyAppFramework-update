#ifndef __EDITWIDGET_HPP__
#define __EDITWIDGET_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "widget.hpp"

namespace PopLib
{

class Font;
class EditListener;

class EditWidget : public Widget
{
  public:
	enum
	{
		COLOR_BKG,
		COLOR_OUTLINE,
		COLOR_TEXT,
		COLOR_HILITE,
		COLOR_HILITE_TEXT,
		NUM_COLORS
	};

	int mId;
	PopString mString;
	PopString mPasswordDisplayString;
	Font *mFont;

	struct WidthCheck
	{
		Font *mFont;
		int mWidth;
	};
	typedef std::list<WidthCheck> WidthCheckList;
	WidthCheckList mWidthCheckList;

	EditListener *mEditListener;
	bool mShowingCursor;
	bool mDrawSelOverride; // set this to true to draw selected text even when not in focus
	bool mHadDoubleClick;  // Used to fix a bug with double clicking to hilite a word after the widget manager started
						   // calling mouse drag before mouse down/up events
	int mCursorPos;
	int mHilitePos;
	int mBlinkAcc;
	int mBlinkDelay;
	int mLeftPos;
	int mMaxChars;
	int mMaxPixels;
	PopChar mPasswordChar;

	PopString mUndoString;
	int mUndoCursor;
	int mUndoHilitePos;
	int mLastModifyIdx;

  protected:
	virtual void ProcessKey(KeyCode theKey, PopChar theChar);
	PopString &GetDisplayString();
	virtual void HiliteWord();
	void UpdateCaretPos();

  public:
	virtual void SetFont(Font *theFont, Font *theWidthCheckFont = NULL);
	virtual void SetText(const PopString &theText, bool leftPosToZero = true);
	virtual bool IsPartOfWord(PopChar theChar);
	virtual int GetCharAt(int x, int y);

	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void Draw(Graphics *g); // Already translated;

	virtual void Update();
	virtual void MarkDirty();

	virtual bool WantsFocus();
	virtual void GotFocus();
	virtual void LostFocus();
	virtual void FocusCursor(bool bigJump);

	virtual void KeyDown(KeyCode theKey);
	virtual void KeyChar(PopChar theChar);

	virtual void MouseDown(int x, int y, int theClickCount)
	{
		Widget::MouseDown(x, y, theClickCount);
	}
	virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount);
	virtual void MouseUp(int x, int y)
	{
		Widget::MouseUp(x, y);
	}
	virtual void MouseUp(int x, int y, int theClickCount)
	{
		Widget::MouseUp(x, y, theClickCount);
	}
	virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount);
	virtual void MouseDrag(int x, int y);
	virtual void MouseEnter();
	virtual void MouseLeave();
	void ClearWidthCheckFonts();
	void AddWidthCheckFont(Font *theFont, int theMaxPixels = -1); // defaults to mMaxPixels
	void EnforceMaxPixels();

  public:
	EditWidget(int theId, EditListener *theEditListener);
	virtual ~EditWidget();
};

} // namespace PopLib

#endif