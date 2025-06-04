#ifndef __LISTLISTENER_HPP__
#define __LISTLISTENER_HPP__
#ifdef _WIN32
#pragma once
#endif

namespace PopLib
{

class ListListener
{
  public:
	virtual void ListClicked(int theId, int theIdx, int theClickCount)
	{
	}
	virtual void ListClosed(int theId)
	{
	}
	virtual void ListHiliteChanged(int theId, int theOldIdx, int theNewIdx)
	{
	}
};

} // namespace PopLib

#endif