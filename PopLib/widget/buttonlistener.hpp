#ifndef __BUTTONLISTENER_HPP__
#define __BUTTONLISTENER_HPP__
#ifdef _WIN32
#pragma once
#endif

namespace PopLib
{

class ButtonListener
{
  public:
	virtual void ButtonPress(int theId)
	{
	}
	virtual void ButtonPress(int theId, int theClickCount)
	{
		ButtonPress(theId);
	}
	virtual void ButtonDepress(int theId)
	{
	}
	virtual void ButtonDownTick(int theId)
	{
	}
	virtual void ButtonMouseEnter(int theId)
	{
	}
	virtual void ButtonMouseLeave(int theId)
	{
	}
	virtual void ButtonMouseMove(int theId, int theX, int theY)
	{
	}
};

} // namespace PopLib

#endif