#include "common.hpp"
#include "math/mtrand.hpp"
#include "debug/debug.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <filesystem>
#include <cstdarg>
#include <wchar.h>

#include "debug/debug.hpp"

bool PopLib::gDebug = false;
static PopLib::MTRand gMTRand;
namespace PopLib
{
#ifdef _WIN32
std::string gAppDataFolder = std::filesystem::path(std::getenv("LOCALAPPDATA")).string() + "/";
#else
std::string gAppDataFolder = std::filesystem::path(std::getenv("HOME")).string() + "/.config/";
#endif
}

int PopLib::Rand()
{
	return gMTRand.Next();
}

int PopLib::Rand(int range)
{
	return gMTRand.Next((ulong)range);
}

float PopLib::Rand(float range)
{
	return gMTRand.Next(range);
}

void PopLib::SRand(ulong theSeed)
{
	gMTRand.SRand(theSeed);
}

bool PopLib::CheckFor98Mill()
{
	return false;
}

bool PopLib::CheckForVista()
{
	return false;
}

std::string PopLib::GetAppDataFolder()
{
	return PopLib::gAppDataFolder;
}

void PopLib::SetAppDataFolder(const std::string &thePath)
{
	/*
	std::string aPath = thePath;
	if (!aPath.empty())
	{
		if (aPath[aPath.length() - 1] != '\\' && aPath[aPath.length() - 1] != '/')
			aPath += '\\';
	}

	PopLib::gAppDataFolder = aPath;
	*/
}

std::string PopLib::URLEncode(const std::string &theString)
{
	const char *aHexChars = "0123456789ABCDEF";

	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
	{
		switch (theString[i])
		{
		case ' ':
			aString.insert(aString.end(), '+');
			break;
		case '?':
		case '&':
		case '%':
		case '+':
		case '\r':
		case '\n':
		case '\t':
			aString.insert(aString.end(), '%');
			aString.insert(aString.end(), aHexChars[(theString[i] >> 4) & 0xF]);
			aString.insert(aString.end(), aHexChars[(theString[i]) & 0xF]);
			break;
		default:
			aString.insert(aString.end(), theString[i]);
		}
	}

	return aString;
}

std::string PopLib::StringToUpper(const std::string &theString)
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += toupper(theString[i]);

	return aString;
}

std::wstring PopLib::StringToUpper(const std::wstring &theString)
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += towupper(theString[i]);

	return aString;
}

std::string PopLib::StringToLower(const std::string &theString)
{
	std::string aString;

	for (unsigned i = 0; i < theString.length(); i++)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring PopLib::StringToLower(const std::wstring &theString)
{
	std::wstring aString;

	for (unsigned i = 0; i < theString.length(); ++i)
		aString += tolower(theString[i]);

	return aString;
}

std::wstring PopLib::StringToWString(const std::string &theString)
{
	std::wstring aString;
	aString.reserve(theString.length());
	for (size_t i = 0; i < theString.length(); ++i)
		aString += (unsigned char)theString[i];
	return aString;
}

std::string PopLib::WStringToString(const std::wstring &theString)
{
	size_t aRequiredLength = wcstombs(NULL, theString.c_str(), 0);
	if (aRequiredLength < 16384)
	{
		char aBuffer[16384];
		wcstombs(aBuffer, theString.c_str(), 16384);
		return std::string(aBuffer);
	}
	else
	{
		DBG_ASSERTE(aRequiredLength != (size_t)-1);
		if (aRequiredLength == (size_t)-1)
			return "";

		char *aBuffer = new char[aRequiredLength + 1];
		wcstombs(aBuffer, theString.c_str(), aRequiredLength + 1);
		std::string aStr = aBuffer;
		delete[] aBuffer;
		return aStr;
	}
}

std::string PopLib::Trim(const std::string &theString)
{
	int aStartPos = 0;
	while (aStartPos < (int)theString.length() && isspace((unsigned char)theString[aStartPos]))
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while (anEndPos >= 0 && isspace((unsigned char)theString[anEndPos]))
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

std::wstring PopLib::Trim(const std::wstring &theString)
{
	int aStartPos = 0;
	while (aStartPos < (int)theString.length() && iswspace(theString[aStartPos]))
		aStartPos++;

	int anEndPos = theString.length() - 1;
	while (anEndPos >= 0 && iswspace(theString[anEndPos]))
		anEndPos--;

	return theString.substr(aStartPos, anEndPos - aStartPos + 1);
}

bool PopLib::StringToInt(const std::string theString, int *theIntVal)
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((theRadix == 10) && (aChar >= '0') && (aChar <= '9'))
			*theIntVal = (*theIntVal * 10) + (aChar - '0');
		else if ((theRadix == 0x10) && (((aChar >= '0') && (aChar <= '9')) || ((aChar >= 'A') && (aChar <= 'F')) ||
										((aChar >= 'a') && (aChar <= 'f'))))
		{
			if (aChar <= '9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - '0');
			else if (aChar <= 'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - 'a') + 0x0A;
		}
		else if (((aChar == 'x') || (aChar == 'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool PopLib::StringToInt(const std::wstring theString, int *theIntVal)
{
	*theIntVal = 0;

	if (theString.length() == 0)
		return false;

	int theRadix = 10;
	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((theRadix == 10) && (aChar >= L'0') && (aChar <= L'9'))
			*theIntVal = (*theIntVal * 10) + (aChar - L'0');
		else if ((theRadix == 0x10) && (((aChar >= L'0') && (aChar <= L'9')) || ((aChar >= L'A') && (aChar <= L'F')) ||
										((aChar >= L'a') && (aChar <= L'f'))))
		{
			if (aChar <= L'9')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'0');
			else if (aChar <= L'F')
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'A') + 0x0A;
			else
				*theIntVal = (*theIntVal * 0x10) + (aChar - L'a') + 0x0A;
		}
		else if (((aChar == L'x') || (aChar == L'X')) && (i == 1) && (*theIntVal == 0))
		{
			theRadix = 0x10;
		}
		else
		{
			*theIntVal = 0;
			return false;
		}
	}

	if (isNeg)
		*theIntVal = -*theIntVal;

	return true;
}

bool PopLib::StringToDouble(const std::string theString, double *theDoubleVal)
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - '0');
		else if (aChar == '.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		char aChar = theString[i];

		if ((aChar >= '0') && (aChar <= '9'))
		{
			*theDoubleVal += (aChar - '0') * aMult;
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

bool PopLib::StringToDouble(const std::wstring theString, double *theDoubleVal)
{
	*theDoubleVal = 0.0;

	if (theString.length() == 0)
		return false;

	bool isNeg = false;

	unsigned i = 0;
	if (theString[i] == '-')
	{
		isNeg = true;
		i++;
	}

	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
			*theDoubleVal = (*theDoubleVal * 10) + (aChar - L'0');
		else if (aChar == L'.')
		{
			i++;
			break;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	double aMult = 0.1;
	for (; i < theString.length(); i++)
	{
		wchar_t aChar = theString[i];

		if ((aChar >= L'0') && (aChar <= L'9'))
		{
			*theDoubleVal += (aChar - L'0') * aMult;
			aMult /= 10.0;
		}
		else
		{
			*theDoubleVal = 0.0;
			return false;
		}
	}

	if (isNeg)
		*theDoubleVal = -*theDoubleVal;

	return true;
}

// TODO: Use <locale> for localization of number output?
PopString PopLib::CommaSeperate(int theValue)
{
	if (theValue == 0)
		return "0";

	PopString aCurString;

	int aPlace = 0;
	int aCurValue = theValue;

	while (aCurValue > 0)
	{
		if ((aPlace != 0) && (aPlace % 3 == 0))
			aCurString = ',' + aCurString;
		aCurString = (PopChar)('0' + (aCurValue % 10)) + aCurString;
		aCurValue /= 10;
		aPlace++;
	}

	return aCurString;
}

std::string PopLib::GetCurDir()
{
    return std::filesystem::current_path().string();
}

std::string PopLib::GetFullPath(const std::string &theRelPath)
{
	return GetPathFrom(theRelPath, GetCurDir());
}

std::string PopLib::GetPathFrom(const std::string &theRelPath, const std::string &theDir)
{
	std::string aDriveString;
	std::string aNewPath = theDir;

	if ((theRelPath.length() >= 2) && (theRelPath[1] == ':'))
		return theRelPath;

	char aSlashChar = '/';

	if ((theRelPath.find('\\') != -1) || (theDir.find('\\') != -1))
		aSlashChar = '\\';

	if ((aNewPath.length() >= 2) && (aNewPath[1] == ':'))
	{
		aDriveString = aNewPath.substr(0, 2);
		aNewPath.erase(aNewPath.begin(), aNewPath.begin() + 2);
	}

	// Append a trailing slash if necessary
	if ((aNewPath.length() > 0) && (aNewPath[aNewPath.length() - 1] != '\\') &&
		(aNewPath[aNewPath.length() - 1] != '/'))
		aNewPath += aSlashChar;

	std::string aTempRelPath = theRelPath;

	for (;;)
	{
		if (aNewPath.length() == 0)
			break;

		int aFirstSlash = aTempRelPath.find('\\');
		int aFirstForwardSlash = aTempRelPath.find('/');

		if ((aFirstSlash == -1) || ((aFirstForwardSlash != -1) && (aFirstForwardSlash < aFirstSlash)))
			aFirstSlash = aFirstForwardSlash;

		if (aFirstSlash == -1)
			break;

		std::string aChDir = aTempRelPath.substr(0, aFirstSlash);

		aTempRelPath.erase(aTempRelPath.begin(), aTempRelPath.begin() + aFirstSlash + 1);

		if (aChDir.compare("..") == 0)
		{
			int aLastDirStart = aNewPath.length() - 1;
			while ((aLastDirStart > 0) && (aNewPath[aLastDirStart - 1] != '\\') && (aNewPath[aLastDirStart - 1] != '/'))
				aLastDirStart--;

			std::string aLastDir = aNewPath.substr(aLastDirStart, aNewPath.length() - aLastDirStart - 1);
			if (aLastDir.compare("..") == 0)
			{
				aNewPath += "..";
				aNewPath += aSlashChar;
			}
			else
			{
				aNewPath.erase(aNewPath.begin() + aLastDirStart, aNewPath.end());
			}
		}
		else if (aChDir.compare("") == 0)
		{
			aNewPath = aSlashChar;
			break;
		}
		else if (aChDir.compare(".") != 0)
		{
			aNewPath += aChDir + aSlashChar;
			break;
		}
	}

	aNewPath = aDriveString + aNewPath + aTempRelPath;

	if (aSlashChar == '/')
	{
		for (;;)
		{
			int aSlashPos = aNewPath.find('\\');
			if (aSlashPos == -1)
				break;
			aNewPath[aSlashPos] = '/';
		}
	}
	else
	{
		for (;;)
		{
			int aSlashPos = aNewPath.find('/');
			if (aSlashPos == -1)
				break;
			aNewPath[aSlashPos] = '\\';
		}
	}

	return aNewPath;
}

bool PopLib::AllowAllAccess(const std::string &theFileName)
{
	return true;
}

bool PopLib::Deltree(const std::string &thePath)
{
	return false;
}

bool PopLib::FileExists(const std::string &theFileName)
{
    return std::filesystem::exists(theFileName);
}

void PopLib::MkDir(const std::string &theDir)
{
	std::filesystem::create_directories(theDir);
}

std::string PopLib::GetFileName(const std::string &thePath, bool noExtension)
{
	int aLastSlash = std::max((int)thePath.rfind('\\'), (int)thePath.rfind('/'));

	if (noExtension)
	{
		int aLastDot = (int)thePath.rfind('.');
		if (aLastDot > aLastSlash)
			return thePath.substr(aLastSlash + 1, aLastDot - aLastSlash - 1);
	}

	if (aLastSlash == -1)
		return thePath;
	else
		return thePath.substr(aLastSlash + 1);
}

std::string PopLib::GetFileDir(const std::string &thePath, bool withSlash)
{
	int aLastSlash = std::max((int)thePath.rfind('\\'), (int)thePath.rfind('/'));

	if (aLastSlash == -1)
		return "";
	else
	{
		if (withSlash)
			return thePath.substr(0, aLastSlash + 1);
		else
			return thePath.substr(0, aLastSlash);
	}
}

std::string PopLib::RemoveTrailingSlash(const std::string &theDirectory)
{
	int aLen = theDirectory.length();

	if ((aLen > 0) && ((theDirectory[aLen - 1] == '\\') || (theDirectory[aLen - 1] == '/')))
		return theDirectory.substr(0, aLen - 1);
	else
		return theDirectory;
}

std::string PopLib::AddTrailingSlash(const std::string &theDirectory, bool backSlash)
{
	if (!theDirectory.empty())
	{
		char aChar = theDirectory[theDirectory.length() - 1];
		if (aChar != '\\' && aChar != '/')
			return theDirectory + (backSlash ? '\\' : '/');
		else
			return theDirectory;
	}
	else
		return "";
}

time_t PopLib::GetFileDate(const std::string &theFileName)
{
	namespace fs = std::filesystem;
    try {
        auto ftime = fs::last_write_time(theFileName);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return std::chrono::system_clock::to_time_t(sctp);
    } catch (...) {
        return 0; // file doesn't exist
    }
}

std::string PopLib::vformat(const char *fmt, va_list argPtr)
{
	// We draw the line at a 1MB string.
	const int maxSize = 1000000;

	// If the string is less than 161 characters,
	// allocate it on the stack because this saves
	// the malloc/free time.
	const int bufSize = 161;
	char stackBuffer[bufSize];

	int attemptedSize = bufSize - 1;

	int numChars = 0;
#ifdef _WIN32
	numChars = _vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = vsnprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	// cout << "NumChars: " << numChars << endl;

	if ((numChars >= 0) && (numChars <= attemptedSize))
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

		// Got it on the first try.
		return std::string(stackBuffer);
	}

	// Now use the heap.
	char *heapBuffer = NULL;

	while (((numChars == -1) || (numChars > attemptedSize)) && (attemptedSize < maxSize))
	{
		// Try a bigger size
		attemptedSize *= 2;
		heapBuffer = (char *)realloc(heapBuffer, (attemptedSize + 1));
#ifdef _WIN32
		numChars = _vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = vsnprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
	}

	heapBuffer[numChars] = 0;

	std::string result = std::string(heapBuffer);

	free(heapBuffer);
	return result;
}

// overloaded StrFormat: should only be used by the xml strings
std::string PopLib::StrFormat(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list argsCopy;
    va_copy(argsCopy, args);

    int size = std::vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (size <= 0) {
        va_end(args);
        return {};
    }

    std::vector<char> buffer(size + 1);
    std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);

    return std::string(buffer.data());
}

std::wstring PopLib::vformat(const wchar_t *fmt, va_list argPtr)
{
	// We draw the line at a 1MB string.
	const int maxSize = 1000000;

	// If the string is less than 161 characters,
	// allocate it on the stack because this saves
	// the malloc/free time.
	const int bufSize = 161;
	wchar_t stackBuffer[bufSize];

	int attemptedSize = bufSize - 1;

	int numChars = 0;
#ifdef _WIN32
	numChars = _vsnwprintf(stackBuffer, attemptedSize, fmt, argPtr);
#else
	numChars = vswprintf(stackBuffer, attemptedSize, fmt, argPtr);
#endif

	// cout << "NumChars: " << numChars << endl;

	if ((numChars >= 0) && (numChars <= attemptedSize))
	{
		// Needed for case of 160-character printf thing
		stackBuffer[numChars] = '\0';

		// Got it on the first try.
		return std::wstring(stackBuffer);
	}

	// Now use the heap.
	wchar_t *heapBuffer = NULL;

	while (((numChars == -1) || (numChars > attemptedSize)) && (attemptedSize < maxSize))
	{
		// Try a bigger size
		attemptedSize *= 2;
		heapBuffer = (wchar_t *)realloc(heapBuffer, (attemptedSize + 1) * sizeof(wchar_t));
#ifdef _WIN32
		numChars = _vsnwprintf(heapBuffer, attemptedSize, fmt, argPtr);
#else
		numChars = vswprintf(heapBuffer, attemptedSize, fmt, argPtr);
#endif
	}

	heapBuffer[numChars] = 0;

	std::wstring result(heapBuffer);

	free(heapBuffer);

	return result;
}

// overloaded StrFormat: should only be used by the xml strings
std::wstring PopLib::StrFormat(const wchar_t *fmt...)
{
	va_list argList;
	va_start(argList, fmt);
	std::wstring result = vformat(fmt, argList);
	va_end(argList);

	return result;
}

std::string PopLib::Evaluate(const std::string &theString, const DefinesMap &theDefinesMap)
{
	std::string anEvaluatedString = theString;

	for (;;)
	{
		int aPercentPos = anEvaluatedString.find('%');

		if (aPercentPos == std::string::npos)
			break;

		int aSecondPercentPos = anEvaluatedString.find('%', aPercentPos + 1);
		if (aSecondPercentPos == std::string::npos)
			break;

		std::string aName = anEvaluatedString.substr(aPercentPos + 1, aSecondPercentPos - aPercentPos - 1);

		std::string aValue;
		DefinesMap::const_iterator anItr = theDefinesMap.find(aName);
		if (anItr != theDefinesMap.end())
			aValue = anItr->second;
		else
			aValue = "";

		anEvaluatedString.erase(anEvaluatedString.begin() + aPercentPos,
								anEvaluatedString.begin() + aSecondPercentPos + 1);
		anEvaluatedString.insert(anEvaluatedString.begin() + aPercentPos, aValue.begin(),
								 aValue.begin() + aValue.length());
	}

	return anEvaluatedString;
}

std::string PopLib::XMLDecodeString(const std::string &theString)
{
	std::string aNewString;

	int aUTF8Len = 0;
	int aUTF8CurVal = 0;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == '&')
		{
			int aSemiPos = theString.find(';', i);

			if (aSemiPos != -1)
			{
				std::string anEntName = theString.substr(i + 1, aSemiPos - i - 1);
				i = aSemiPos;

				if (anEntName == "lt")
					c = '<';
				else if (anEntName == "amp")
					c = '&';
				else if (anEntName == "gt")
					c = '>';
				else if (anEntName == "quot")
					c = '"';
				else if (anEntName == "apos")
					c = '\'';
				else if (anEntName == "nbsp")
					c = ' ';
				else if (anEntName == "cr")
					c = '\n';
			}
		}

		aNewString += c;
	}

	return aNewString;
}

std::wstring PopLib::XMLDecodeString(const std::wstring &theString)
{
	std::wstring aNewString;

	int aUTF8Len = 0;
	int aUTF8CurVal = 0;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == L'&')
		{
			int aSemiPos = theString.find(L';', i);

			if (aSemiPos != -1)
			{
				std::wstring anEntName = theString.substr(i + 1, aSemiPos - i - 1);
				i = aSemiPos;

				if (anEntName == L"lt")
					c = L'<';
				else if (anEntName == L"amp")
					c = L'&';
				else if (anEntName == L"gt")
					c = L'>';
				else if (anEntName == L"quot")
					c = L'"';
				else if (anEntName == L"apos")
					c = L'\'';
				else if (anEntName == L"nbsp")
					c = L' ';
				else if (anEntName == L"cr")
					c = L'\n';
			}
		}

		aNewString += c;
	}

	return aNewString;
}

std::string PopLib::XMLEncodeString(const std::string &theString)
{
	std::string aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		char c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += "&nbsp;";
				continue;
			}

			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{
			switch (c)
			{
			case '<':
				aNewString += "&lt;";
				break;
			case '&':
				aNewString += "&amp;";
				break;
			case '>':
				aNewString += "&gt;";
				break;
			case '"':
				aNewString += "&quot;";
				break;
			case '\'':
				aNewString += "&apos;";
				break;
			case '\n':
				aNewString += "&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

std::wstring PopLib::XMLEncodeString(const std::wstring &theString)
{
	std::wstring aNewString;

	bool hasSpace = false;

	for (ulong i = 0; i < theString.length(); i++)
	{
		wchar_t c = theString[i];

		if (c == ' ')
		{
			if (hasSpace)
			{
				aNewString += L"&nbsp;";
				continue;
			}

			hasSpace = true;
		}
		else
			hasSpace = false;

		/*if ((uchar) c >= 0x80)
		{
			// Convert to UTF
			aNewString += (char) (0xC0 | ((c >> 6) & 0xFF));
			aNewString += (char) (0x80 | (c & 0x3F));
		}
		else*/
		{
			switch (c)
			{
			case L'<':
				aNewString += L"&lt;";
				break;
			case L'&':
				aNewString += L"&amp;";
				break;
			case L'>':
				aNewString += L"&gt;";
				break;
			case L'"':
				aNewString += L"&quot;";
				break;
			case L'\'':
				aNewString += L"&apos;";
				break;
			case L'\n':
				aNewString += L"&cr;";
				break;
			default:
				aNewString += c;
				break;
			}
		}
	}

	return aNewString;
}

std::string PopLib::Upper(const std::string &_data)
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), toupper);
	return s;
}

std::wstring PopLib::Upper(const std::wstring &_data)
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towupper);
	return s;
}

std::string PopLib::Lower(const std::string &_data)
{
	std::string s = _data;
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::wstring PopLib::Lower(const std::wstring &_data)
{
	std::wstring s = _data;
	std::transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int PopLib::StrFindNoCase(const char *theStr, const char *theFind)
{
	int p1, p2;
	int cp = 0;
	const int len1 = (int)strlen(theStr);
	const int len2 = (int)strlen(theFind);
	while (cp < len1)
	{
		p1 = cp;
		p2 = 0;
		while (p1 < len1 && p2 < len2)
		{
			if (tolower(theStr[p1]) != tolower(theFind[p2]))
				break;

			p1++;
			p2++;
		}
		if (p2 == len2)
			return p1 - len2;

		cp++;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PopLib::StrPrefixNoCase(const char *theStr, const char *thePrefix, int maxLength)
{
	int i;
	char c1 = 0, c2 = 0;
	for (i = 0; i < maxLength; i++)
	{
		c1 = tolower(*theStr++);
		c2 = tolower(*thePrefix++);

		if (c1 == 0 || c2 == 0)
			break;

		if (c1 != c2)
			return false;
	}

	return c2 == 0 || i == maxLength;
}
