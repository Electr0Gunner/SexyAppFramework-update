#include "xmlparser.hpp"
#include "debug/debug.hpp"
#include "paklib/pakinterface.hpp"

using namespace PopLib;

XMLParser::XMLParser()
{
	mFile = NULL;
	mLineNum = 0;
	mCurrentNode = nullptr;
	mFirstStart = true;
}

XMLParser::~XMLParser()
{
	if (mFile != NULL)
		p_fclose(mFile);
}

void XMLParser::Fail(const PopString &theErrorText)
{
	mHasFailed = true;
	mErrorText = theErrorText;
}

void XMLParser::Init()
{
    mLineNum = 1;
    mHasFailed  = false;
    mCurrentNode = nullptr;
    mSectionStack.clear();
}

bool XMLParser::AddAttribute(XMLElement *theElement, const PopString &theAttributeKey, const PopString &theAttributeValue)
{
	std::pair<XMLParamMap::iterator, bool> aRet;

	aRet = theElement->mAttributes.insert(XMLParamMap::value_type(theAttributeKey, theAttributeValue));
	if (!aRet.second)
		aRet.first->second = theAttributeValue;

	if (theAttributeKey != "/")
		theElement->mAttributeIteratorList.push_back(aRet.first);

	return aRet.second;
}

bool XMLParser::OpenBuffer(const std::string &theBuffer, const std::string &custom_root)
{
	mCurrentNode = nullptr;
	mSectionStack.clear();
	mEndPending.clear();
	mHasFailed = false;


    // UTF-8 stuff
    std::string input = theBuffer;
    if (input.size() >= 3 &&
        (unsigned char)input[0] == 0xEF &&
        (unsigned char)input[1] == 0xBB &&
        (unsigned char)input[2] == 0xBF)
    {
        input = input.substr(3);
    }

    std::string aNewWrappedBuffer;

    if (custom_root != "")
    {
        // get the declaration so we dont add the custom root before it
        std::string xmlDecl;
        if (input.find("<?xml") == 0)
        {
            size_t declEnd = input.find("?>");
            if (declEnd != std::string::npos)
            {
                declEnd += 2;
                xmlDecl = input.substr(0, declEnd);
                input = input.substr(declEnd);  // remove declaration from the main content
            }
        }

        // Wrap with custom root
        aNewWrappedBuffer = xmlDecl + "<" + custom_root + ">" + input + "</" + custom_root + ">";
    }
    else
        aNewWrappedBuffer = input;

    const char* data = aNewWrappedBuffer.c_str();
    size_t size = aNewWrappedBuffer.size();

    mDocument = new XMLDocument();
	XMLError err = mDocument->Parse(data, size);
	if (err != XML_SUCCESS) { 
		std::string anError = mDocument->ErrorStr();
		Fail("Parse error: " + anError); 
		return false;
	}
	return true;
}

bool XMLParser::OpenFile(const std::string &theFileName, const std::string &custom_root)
{
	mFile = p_fopen(theFileName.c_str(), "r");

	if (mFile == NULL)
	{
		mLineNum = 0;
		Fail("Unable to open file " + theFileName);
		return false;
	}

	p_fseek(mFile, 0, SEEK_END);
	long size = p_ftell(mFile);
	p_fseek(mFile, 0, SEEK_SET);

	if (size <= 0)
	{
		p_fclose(mFile);
		Fail("Empty or unreadable file: " + theFileName);
		return false;
	}

	std::string content(size, '\0');
	p_fread(content.data(), size, 1, mFile);

    OpenBuffer(content, custom_root);
	return true;
}

bool XMLParser::NextElement(XMLElement *theElement)
{

    theElement->mAttributes.clear();
    theElement->mInstruction.clear();
    theElement->mValue.clear();
    theElement->mSection.clear();


    if (!mEndPending.empty())
    {
        theElement->mType = XMLElement::TYPE_END;
        theElement->mValue = mEndPending.back();
        theElement->mSection = mSectionStack.back();
        mEndPending.pop_back();
        mSectionStack.pop_back();
        return true;
    }

	while (!mNodeStack.empty() && mCurrentNode == nullptr)
    {
        XMLNode* parent = mNodeStack.back();
        mNodeStack.pop_back();

        theElement->mType = XMLElement::TYPE_END;
        theElement->mValue = parent->ToElement()->Name();
        theElement->mSection = mSectionStack.back();
        mSectionStack.pop_back();

        if (mSectionStack.empty())
            return false; //We left the root?

        mCurrentNode = parent->NextSibling();
        while (mCurrentNode &&
            !mCurrentNode->ToElement() &&
            !mCurrentNode->ToComment() &&
            !mCurrentNode->ToDeclaration())
        {
            mCurrentNode = mCurrentNode->NextSibling();
        }
		if (mCurrentNode)
			mLineNum = mCurrentNode->GetLineNum();
        return true;
    }

	if (!mCurrentNode && mFirstStart)
    {
        mCurrentNode = mDocument->FirstChild();
        mFirstStart = false;
    }
    else if (!mCurrentNode && !mFirstStart)
    {
        // No more nodes to process, end of document reached
        return false;
    }

    while (mCurrentNode &&
           !mCurrentNode->ToElement() &&
           !mCurrentNode->ToComment() &&
           !mCurrentNode->ToDeclaration())
    {
        mCurrentNode = mCurrentNode->NextSibling();
    }

  	if (!mCurrentNode)
        return false;

    if (auto com = mCurrentNode->ToComment()) // Comments
    {
        theElement->mType = XMLElement::TYPE_COMMENT;
        theElement->mInstruction = com->Value();
        mCurrentNode = mCurrentNode->NextSibling();
        return true;
    }
    else if (auto decl = mCurrentNode->ToDeclaration()) // XML declaration / PI
    {
        theElement->mType        = XMLElement::TYPE_INSTRUCTION;
        theElement->mInstruction = decl->Value();
		mCurrentNode = mCurrentNode->NextSibling();
        return true;
    }
    else if (auto elem = mCurrentNode->ToElement()) // Element start
    {
        auto closeType = elem->ClosingType();

        if (closeType == tinyxml2::XMLElement::OPEN)
        {
            // Start tag <tag>
            theElement->mType = XMLElement::TYPE_START;
            theElement->mValue = elem->Name();

            if (mSectionStack.empty())
                mSectionStack.push_back(elem->Name());
            else
                mSectionStack.push_back(mSectionStack.back() + "/" + elem->Name());
            theElement->mSection = mSectionStack.back();

            for (auto attr = elem->FirstAttribute(); attr; attr = attr->Next())
                AddAttribute(theElement, attr->Name(), attr->Value());

            mCurrentNode = elem->FirstChild();
            mNodeStack.push_back(elem);

            return true;
        }
        else if (closeType == tinyxml2::XMLElement::CLOSED)
        {
            // Self-closing tag <tag />
            // Emit START event now
            theElement->mType = XMLElement::TYPE_START;
            theElement->mValue = elem->Name();

            if (mSectionStack.empty())
                mSectionStack.push_back(elem->Name());
            else
                mSectionStack.push_back(mSectionStack.back() + "/" + elem->Name());
            theElement->mSection = mSectionStack.back();

            for (auto attr = elem->FirstAttribute(); attr; attr = attr->Next())
                AddAttribute(theElement, attr->Name(), attr->Value());

            // Immediately emit the END event for the same element on next call
            mEndPending.push_back(elem->Name());

            mCurrentNode = mCurrentNode->NextSibling();

            return true;
        }
        else if (closeType == tinyxml2::XMLElement::CLOSING)
        {
            // This means an end tag node (</tag>) - normally tinyxml2 skips these as separate nodes,
            // but if you get here, emit END event.

            theElement->mType = XMLElement::TYPE_END;
            theElement->mValue = elem->Name();

            if (!mSectionStack.empty())
                mSectionStack.pop_back();

            theElement->mSection = mSectionStack.empty() ? "" : mSectionStack.back();

            mCurrentNode = mCurrentNode->NextSibling();

            return true;
        }
    }

	return false;
}

bool XMLParser::HasFailed()
{
	return mHasFailed;
}

PopString XMLParser::GetErrorText()
{
	return mErrorText;
}

int XMLParser::GetCurrentLineNum()
{
	return mLineNum;
}

std::string XMLParser::GetFileName()
{
	return mFileName;
}