#ifndef __XMLDEMOAPP_HPP__
#define __XMLDEMOAPP_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "PopLib/popapp.hpp"

namespace PopLib
{

class Board;

// This is the new class we're going to be learning about.
// It'll handle the loading and manipulation of XML files.
class XMLParser;

// These structs just represent the values from our properties/demo.xml
// file. In a real game, you'd have your own made up config file format.
// The names correspond to the attributes/sections from the XML file.
struct s_Section1
{
	PopString mItem1Text;

	PopString mBoolParamText;
	bool mBoolParamElement;
};

struct s_Section2
{
	PopString mSectionAttribText;

	PopString mIntParamText;
	int mIntParamElement;

	PopString mMultiAttrib1;
	PopString mMultiAttrib2;
	PopString mMultiAttrib3;

	PopString mSubsectionItem2;
};

class XMLDemoApp : public PopApp
{

  public:
	Board *mBoard;
	s_Section1 mSection1;
	s_Section2 mSection2;
	PopString mInstructionTag;
	PopString mInstruction;

  private:
	// With this class, we'll read in our sample XML file, properties/demo.xml
	XMLParser *mParser;

  private:
	//////////////////////////////////////////////////////////////////////////
	// Function:	ExtractXMLData
	// Parameters:	none
	// Returns:		false on error, otherwise true
	//
	// Purpose: After opening an XML file via the XMLParser::OpenFile() method,
	// this function then extracts out the data it needs from the sample XML file.
	// When complete, mSection1 and mSection2 will have their values initialized.
	//////////////////////////////////////////////////////////////////////////
	bool ExtractXMLData();

  public:
	XMLDemoApp();

	virtual ~XMLDemoApp();

	virtual void ShutdownHook();
	virtual void InitHook();
};

} // namespace PopLib
#endif //__XMLDEMOAPP_HPP__
