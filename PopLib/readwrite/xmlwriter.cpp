// xmlwriter.cpp: implementation of the XMLWriter class.
//
//////////////////////////////////////////////////////////////////////

#include "xmlwriter.hpp"

#pragma warning(disable : 4018)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
using namespace PopLib;

XMLWriter::XMLWriter()
{
	mFile = NULL;
	mLineNum = 0;
	mOpenAttributes = false;
}

XMLWriter::~XMLWriter()
{
	if (mFile != NULL)
		fclose(mFile);
}

void XMLWriter::Fail(const PopString &theErrorText)
{
	mHasFailed = true;
	mErrorText = theErrorText;
}

void XMLWriter::Warn(const PopString &theWarning)
{
	mWarningStack.push("WARNING: " + theWarning);
}

void XMLWriter::Comment(const PopString &theComment)
{
	mWarningStack.push(theComment);
}

void XMLWriter::Init()
{
	mLineNum = 1;
	mHasFailed = false;
}

bool XMLWriter::OpenFile(const PopString &theFileName)
{
	mFile = fopen(theFileName.c_str(), "w+t");

	if (mFile == NULL)
	{
		mLineNum = 0;
		Fail("Unable to open file " + theFileName);
		return false;
	}

	mFileName = theFileName.c_str();
	Init();
	return true;
}

bool XMLWriter::HasFailed()
{
	return mHasFailed;
}

PopString XMLWriter::GetErrorText()
{
	return mErrorText;
}

int XMLWriter::GetCurrentLineNum()
{
	return mLineNum;
}

PopString XMLWriter::GetFileName()
{
	return mFileName;
}

/*
	Used as a tool to add attributes to XMLElement Nodes.
*/
bool XMLWriter::AddAttribute(XMLElement *theElement, const PopString &theAttributeKey,
							 const PopString &theAttributeValue)
{
	std::pair<XMLParamMap::iterator, bool> aRet;

	aRet = theElement->mAttributes.insert(XMLParamMap::value_type(theAttributeKey, theAttributeValue));
	if (!aRet.second)
		aRet.first->second = theAttributeValue;

	if (theAttributeKey != "/")
		theElement->mAttributeIteratorList.push_back(aRet.first);

	return aRet.second;
}

/*
	Pushes the Element onto the section stack and creates
	a new Node with an Attributes section
*/
bool XMLWriter::StartElement(const PopString &theElementName)
{
	CheckFileOpen();
	if (mHasFailed)
		return false;

	if (mOpenAttributes)
	{
		// Close Previous Element Attribute section
		fprintf(mFile, ">\n");
		mLineNum++;
	}

	if (!ValidateElementNodeName(theElementName))
	{
		Warn(theElementName + " is an Invalid Node Name.");
	}

	while (!mWarningStack.empty())
	{
		fprintf(mFile, "<!--  %s -->\n", mWarningStack.top().c_str());
		mWarningStack.pop();
	}

	mSectionStack.push(theElementName);

	for (int i = 1; i < mSectionStack.size(); i++)
	{
		fprintf(mFile, "\t");
	}

	fprintf(mFile, "<%s", theElementName.c_str());

	mOpenAttributes = true;
	return true;
}

// Warning: This Stops the Element!
bool XMLWriter::WriteElementText(PopString theText)
{
	CheckFileOpen();
	if (mHasFailed)
		return false;

	if (mOpenAttributes)
	{
		// Close Previous Element Attribute section
		fprintf(mFile, ">");
		mOpenAttributes = false;
	}

	const PopString aNodeName = mSectionStack.top();
	mSectionStack.pop();

	try
	{
		// I fixed this error, but just in case, I handle it by re-encoding it
		fprintf(mFile, "%s", theText.c_str());
	}
	catch (...)
	{
		fprintf(mFile, "%s", XMLEncodeString(theText).c_str());
	}
	fprintf(mFile, "</%s>\n", aNodeName.c_str());

	mLineNum++;

	return true;
}

/*
	Pushes theElement->Value onto the stack and adds all of
	theElements->mAttributes to the Attribute section
*/
bool XMLWriter::StartElement(XMLElement *theElement)
{
	if (StartElement(theElement->mValue.c_str()) == false)
		return false;

	std::map<PopString, PopString>::iterator map_itr;
	map_itr = theElement->mAttributes.begin();

	for (; map_itr != theElement->mAttributes.end(); map_itr++)
	{
		if (!WriteAttribute(map_itr->first, map_itr->second))
			return false;
	}

	return true;
}

/*
	Closes the previously open Node and pops it from the stack.
	Also Closes the Attributes Writing until StartElement is called.
*/
bool XMLWriter::StopElement()
{
	CheckFileOpen();
	if (mHasFailed)
		return false;

	if (mSectionStack.empty())
	{
		Fail("Stop Element Calls do not match StartElement Calls.");
		return false;
	}

	const PopString aNodeName = mSectionStack.top();
	mSectionStack.pop();

	if (mOpenAttributes)
	{
		// Close Previous Element Attribute section
		fprintf(mFile, "/>\n");
		mLineNum++;
	}
	else
	{
		// Otherwise close element section
		for (int i = 0; i < mSectionStack.size(); i++)
		{
			fprintf(mFile, "\t");
		}

		fprintf(mFile, "</%s>\n", aNodeName.c_str());
	}

	mOpenAttributes = false;

	while (!mWarningStack.empty())
	{
		fprintf(mFile, "<!--  %s -->\n", mWarningStack.top().c_str());
		mWarningStack.pop();
	}

	return true;
}

/*
	Adds an attribute to the Current Element.  If No element is open, then it returns
	false.
*/
bool XMLWriter::WriteAttribute(const PopString &aAttributeKey, const PopString &aAttributeValue)
{
	CheckFileOpen();
	if (mHasFailed)
		return false;

	if (mOpenAttributes)
	{
		if (!ValidateElementNodeName(aAttributeKey))
		{
			Warn(aAttributeKey + " is an invalid Attribute Name.");
		}

		fprintf(mFile, " %s=\"%s\"", aAttributeKey.c_str(), XMLEncodeString(aAttributeValue).c_str());
		return true;
	}

	if (mSectionStack.size())
		Fail("Attributes Section already closed for " + mSectionStack.top());
	else
		Fail("No Element Nodes Open for Writing Attributes.");

	return false;
}

bool XMLWriter::WriteAttribute(const PopString &aAttributeKey, const int &aAttributeValue)
{
	return WriteAttribute(aAttributeKey, StrFormat("%d", aAttributeValue));
}

bool XMLWriter::WriteAttribute(const PopString &aAttributeKey, const float &aAttributeValue)
{
	return WriteAttribute(aAttributeKey, StrFormat("%f", aAttributeValue));
}

bool XMLWriter::CloseFile()
{
	while (!mSectionStack.empty())
	{
		StopElement();
	}

	if (mFile != NULL)
	{
		fclose(mFile);
		return true;
	}

	Fail("File not Open");
	return false;
}

bool XMLWriter::ValidateElementNodeName(const PopString &theNodeName)
{
	const PopChar *aNodeName = theNodeName.c_str();

	for (int i = 0; i < theNodeName.size(); i++)
	{
		if (aNodeName[i] < '0' || (aNodeName[i] > '9' && aNodeName[i] < 'A') ||
			(aNodeName[i] > 'Z' && aNodeName[i] < '_') || (aNodeName[i] > '_' && aNodeName[i] < 'a') ||
			aNodeName[i] > 'z')
		{
			return false;
		}
	}
	return true;
}

bool XMLWriter::CheckFileOpen()
{
	if (mFile != NULL)
	{
		return true;
	}

	Fail("No File Opened for writing");
	return false;
}

bool PopLib::XMLWriter::WriteAttribute(const PopString &aAttributeKey)
{
	CheckFileOpen();
	if (mHasFailed)
		return false;

	if (mOpenAttributes)
	{
		if (!ValidateElementNodeName(aAttributeKey))
		{
			Warn(aAttributeKey + " is an invalid Attribute Name.");
		}

		fprintf(mFile, " %s", aAttributeKey.c_str());
		return true;
	}

	if (mSectionStack.size())
		Fail("Attributes Section already closed for " + mSectionStack.top());
	else
		Fail("No Element Nodes Open for Writing Attributes.");

	return false;
}