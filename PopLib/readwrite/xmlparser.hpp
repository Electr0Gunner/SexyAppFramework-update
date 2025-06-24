#ifndef __XMLPARSER_HPP__
#define __XMLPARSER_HPP__
#ifdef _WIN32
#pragma once
#endif

#include "common.hpp"
#include "debug/perftimer.hpp"

struct PFILE;

namespace PopLib
{

class XMLParam
{
public:
	std::string				mKey;
	std::string				mValue;
};

typedef std::map<PopString, PopString>	XMLParamMap;
typedef std::list<XMLParamMap::iterator>	XMLParamMapIteratorList;

typedef std::vector<wchar_t> XMLParserBuffer;

class XMLElement
{
public:
	enum
	{
		TYPE_NONE,
		TYPE_START,
		TYPE_END,
		TYPE_ELEMENT,
		TYPE_INSTRUCTION,
		TYPE_COMMENT
	};
public:
	
	int						mType;
	PopString				mSection;
	PopString				mValue;
	PopString				mInstruction;
	XMLParamMap				mAttributes;
	XMLParamMapIteratorList	mAttributeIteratorList; // stores attribute iterators in their original order
};

class XMLParser
{
protected:
	std::string				mFileName;
	PopString				mErrorText;
	int						mLineNum;
	PFILE*					mFile;
	bool					mHasFailed;
	bool					mAllowComments;
	XMLParserBuffer			mBufferedText;
	PopString				mSection;
	bool					(XMLParser::*mGetCharFunc)(wchar_t* theChar, bool* error);
	bool					mForcedEncodingType;
	bool					mFirstChar;
	bool					mByteSwap;

protected:
	void					Fail(const PopString& theErrorText);
	void					Init();

	bool					AddAttribute(XMLElement* theElement, const PopString& aAttributeKey, const PopString& aAttributeValue);

	bool					GetAsciiChar(wchar_t* theChar, bool* error);
	bool					GetUTF8Char(wchar_t* theChar, bool* error);
	bool					GetUTF16Char(wchar_t* theChar, bool* error);
	bool					GetUTF16LEChar(wchar_t* theChar, bool* error);
	bool					GetUTF16BEChar(wchar_t* theChar, bool* error);

public:
	enum XMLEncodingType
	{
		ASCII,
		UTF_8,
		UTF_16,
		UTF_16_LE,
		UTF_16_BE
	};

public:
	XMLParser();
	virtual ~XMLParser();

	void					SetEncodingType(XMLEncodingType theEncoding);
	bool					OpenFile(const std::string& theFilename);
	void					SetStringSource(const std::wstring& theString);
	void					SetStringSource(const std::string& theString);
	bool					NextElement(XMLElement* theElement);
	PopString				GetErrorText();
	int						GetCurrentLineNum();
	std::string				GetFileName();

	inline void				AllowComments(bool doAllow) { mAllowComments = doAllow; }

	bool					HasFailed();
	bool					EndOfFile();
};

}; // namespace PopLib

#endif
