//////////////////////////////////////////////////////////////////////////
//					OptionsDialog.h
//
//	Represents a standard options dialog box where the user can toggle
//	3D mode, full screen, custom cursors, adjust sound volumes, and
//	quit the game.
//////////////////////////////////////////////////////////////////////////

#ifndef __OPTIONS_DIALOG_HPP__
#define __OPTIONS_DIALOG_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "PopLib/widget/dialog.hpp"
#include "PopLib/widget/sliderlistener.hpp"
#include "PopLib/widget/checkboxlistener.hpp"

namespace PopLib
{

class Graphics;
class Slider;
class DialogButton;
class Checkbox;
class Board;

class OptionsDialog : public Dialog, public SliderListener, public CheckboxListener
{

  protected:
	Slider *mMusicVolumeSlider;
	Slider *mSfxVolumeSlider;
	DialogButton *mQuitBtn;
	Board *mBoard;

  public:
	enum
	{
		MUSIC_SLIDER_ID,
		SFX_SLIDER_ID,
		QUIT_BTN_ID,
		FS_CHECKBOX_ID,
		HARDWARE_CHECKBOX_ID,
		CUSTOM_CURSORS_CHECKBOX_ID,
		MESSAGE_BOX_ID,
		DIALOG_ID
	};

	Checkbox *m3DCheckbox;
	Checkbox *mFSCheckbox;
	Checkbox *mCustomCursorsCheckbox;

  public:
	//////////////////////////////////////////////////////////////////////////
	//	Function: OptionsDialog
	//	Parameters:
	//		b - A pointer to the board, used to unpause when dialog closes
	//
	//	Returns: none
	//////////////////////////////////////////////////////////////////////////
	OptionsDialog(Board *b);
	virtual ~OptionsDialog();

	//////////////////////////////////////////////////////////////////////////
	//	Draw
	//////////////////////////////////////////////////////////////////////////
	virtual void Draw(Graphics *g);

	//////////////////////////////////////////////////////////////////////////
	//	ButtonDepress
	//////////////////////////////////////////////////////////////////////////
	virtual void ButtonDepress(int theId);

	//////////////////////////////////////////////////////////////////////////
	//	AddedToManager
	//////////////////////////////////////////////////////////////////////////
	virtual void AddedToManager(WidgetManager *theWidgetManager);

	//////////////////////////////////////////////////////////////////////////
	//	RemovedFromManager
	//////////////////////////////////////////////////////////////////////////
	virtual void RemovedFromManager(WidgetManager *theWidgetManager);

	//////////////////////////////////////////////////////////////////////////
	//	Resize
	//////////////////////////////////////////////////////////////////////////
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);

	//////////////////////////////////////////////////////////////////////////
	//	SliderVal
	//////////////////////////////////////////////////////////////////////////
	virtual void SliderVal(int theId, double theVal);

	//////////////////////////////////////////////////////////////////////////
	//	CheckboxChecked
	//////////////////////////////////////////////////////////////////////////
	void CheckboxChecked(int theId, bool checked);
};

} // namespace PopLib

#endif