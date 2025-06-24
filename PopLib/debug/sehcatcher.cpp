#include "sehcatcher.hpp"
#include <AL/al.h>
#include <bass.h>
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_video.h"
#include "appbase.hpp"
#include <fstream>
#include "imgui/core/imgui_impl_sdlrenderer3.h"
#include "imgui/imguimanager.hpp"
#ifndef _WIN32
#include <csignal>
#include <execinfo.h>
#include <dlfcn.h>
#include <sstream>
#else
#include <process.h>
#endif

#include <string>
#include <locale>
#include <codecvt>

using namespace PopLib;

// This is the best i couldve come up with.
#ifdef _WIN32
#ifdef _WIN64
    #define Eip Rip
    #define Ebx Rbx
    #define Ecx Rcx
    #define Edx Rdx
    #define Esi Rsi
    #define Esp Rsp
    #define Edi Rdi
    #define Ebp Rbp
	#define Eax Rax
#else
    #define Eip Eip
    #define Ebx Ebx
    #define Ecx Ecx
    #define Edx Edx
    #define Esi Esi
    #define Esp Esp
    #define Edi Edi
    #define Ebp Ebp
	#define Eax Eax
#endif
#endif

AppBase*				SEHCatcher::mApp = NULL;
bool						SEHCatcher::mHasDemoFile = false;
bool						SEHCatcher::mDebugError = false;
std::string					SEHCatcher::mErrorTitle;
std::string					SEHCatcher::mErrorText;
std::string					SEHCatcher::mUserText;
std::string					SEHCatcher::mUploadFileName;
HTTPTransfer				SEHCatcher::mSubmitReportTransfer;
bool						SEHCatcher::mExiting = false;
bool						SEHCatcher::mShowUI = true;
bool						SEHCatcher::mAllowSubmit = true;
#ifdef _WIN32
HMODULE						SEHCatcher::mImageHelpLib = NULL;
SYMINITIALIZEPROC			SEHCatcher::mSymInitialize = NULL;
SYMSETOPTIONSPROC			SEHCatcher::mSymSetOptions = NULL;
UNDECORATESYMBOLNAMEPROC	SEHCatcher::mUnDecorateSymbolName = NULL;
SYMCLEANUPPROC				SEHCatcher::mSymCleanup = NULL;
STACKWALKPROC				SEHCatcher::mStackWalk = NULL;
SYMFUNCTIONTABLEACCESSPROC	SEHCatcher::mSymFunctionTableAccess = NULL;
SYMGETMODULEBASEPROC		SEHCatcher::mSymGetModuleBase = NULL;
SYMGETSYMFROMADDRPROC		SEHCatcher::mSymGetSymFromAddr = NULL;
LPTOP_LEVEL_EXCEPTION_FILTER SEHCatcher::mPreviousFilter;
#endif

std::wstring					SEHCatcher::mCrashMessage = L"An unexpected error has occured!\nSubmit an issue to the framework's GitHub page.\nPlease help out by providing as much information as you can about this crash.\nThe crash log has been copied to the clipboard.";
std::string						SEHCatcher::mIssueWebsite = "https://github.com/teampopwork/PopLib/issues";
std::wstring					SEHCatcher::mSubmitErrorMessage = L"Failed to open the URL: https://github.com/teampopwork/PopLib/issues.";

static bool gUseDefaultFonts = true;

#ifdef _WIN32
struct 
{
    DWORD   dwExceptionCode;
    const char    *szMessage;
} gMsgTable[] = {
   { STATUS_SEGMENT_NOTIFICATION,     "Segment Notification" },
   { STATUS_BREAKPOINT,               "Breakpoint" },
   { STATUS_SINGLE_STEP,              "Single step" },
   { STATUS_WAIT_0,                   "Wait 0" },
   { STATUS_ABANDONED_WAIT_0,         "Abandoned Wait 0" },
   { STATUS_USER_APC,                 "User APC" },
   { STATUS_TIMEOUT,                  "Timeout" },
   { STATUS_PENDING,                  "Pending" },
   { STATUS_GUARD_PAGE_VIOLATION,     "Guard Page Violation" },
   { STATUS_DATATYPE_MISALIGNMENT,    "Data Type Misalignment" },
   { STATUS_ACCESS_VIOLATION,         "Access Violation" },
   { STATUS_IN_PAGE_ERROR,            "In Page Error" },
   { STATUS_NO_MEMORY,                "No Memory" },
   { STATUS_ILLEGAL_INSTRUCTION,      "Illegal Instruction" },
   { STATUS_NONCONTINUABLE_EXCEPTION, "Noncontinuable Exception" },
   { STATUS_INVALID_DISPOSITION,      "Invalid Disposition" },
   { STATUS_ARRAY_BOUNDS_EXCEEDED,    "Array Bounds Exceeded" },
   { STATUS_FLOAT_DENORMAL_OPERAND,   "Float Denormal Operand" },
   { STATUS_FLOAT_DIVIDE_BY_ZERO,     "Divide by Zero" },
   { STATUS_FLOAT_INEXACT_RESULT,     "Float Inexact Result" },
   { STATUS_FLOAT_INVALID_OPERATION,  "Float Invalid Operation" },
   { STATUS_FLOAT_OVERFLOW,           "Float Overflow" },
   { STATUS_FLOAT_STACK_CHECK,        "Float Stack Check" },
   { STATUS_FLOAT_UNDERFLOW,          "Float Underflow" },
   { STATUS_INTEGER_DIVIDE_BY_ZERO,   "Integer Divide by Zero" },
   { STATUS_INTEGER_OVERFLOW,         "Integer Overflow" },
   { STATUS_PRIVILEGED_INSTRUCTION,   "Privileged Instruction" },
   { STATUS_STACK_OVERFLOW,           "Stack Overflow" },
   { STATUS_CONTROL_C_EXIT,           "Ctrl+C Exit" },
   { 0xFFFFFFFF,                      "" }
};
#else
struct {
    int sig;
    const char* msg;
} gSignalMsgTable[] = {
    { SIGSEGV, "Segmentation Fault" },
    { SIGABRT, "Abort Signal" },
    { SIGFPE,  "Floating Point Exception" },
    { SIGILL,  "Illegal Instruction" },
    { SIGBUS,  "Bus Error" },
    { SIGTERM, "Termination Signal" },
    { 0, nullptr }
};
#endif

SDL_Renderer *mRenderer;
SDL_Window *mWindow;
ImGuiContext *mImGuiContext;

SEHCatcher::SEHCatcher() 
{ 	
#ifdef _WIN32
	mPreviousFilter = SetUnhandledExceptionFilter(UnhandledExceptionFilter); 
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    int signals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTERM };
    for (int sig : signals) sigaction(sig, &sa, nullptr);
#endif
}

SEHCatcher::~SEHCatcher() noexcept
{ 
#ifdef _WIN32
	SetUnhandledExceptionFilter(mPreviousFilter); 
#else
    int signals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTERM };
    for (int sig : signals) sigaction(sig, nullptr, nullptr);
#endif
}

#ifdef _WIN32
long __stdcall SEHCatcher::UnhandledExceptionFilter(LPEXCEPTION_POINTERS lpExceptPtr)
{
	if (mApp != NULL)
		mApp->SEHOccured();	

	DoHandleDebugEvent(lpExceptPtr);
	
	if (!mDebugError)
		SetErrorMode(SEM_NOGPFAULTERRORBOX);
	
	return EXCEPTION_CONTINUE_SEARCH;	
}
#else
void SEHCatcher::signalHandler(int sig, siginfo_t* info, void* ucontext)
{
    const int maxFrames = 64;
    void* frames[maxFrames];
    int count = backtrace(frames, maxFrames);
    char** symbols = backtrace_symbols(frames, count);

    std::ostringstream oss;
    oss << "Signal " << sig << " (" << strsignal(sig) << ")\n";
    for (int i = 0; i < count; ++i) {
        Dl_info dli;
        if (dladdr(frames[i], &dli) && dli.dli_sname)
            oss << "  " << dli.dli_sname << "+0x" << std::hex
                << ((char*)frames[i] - (char*)dli.dli_saddr) << std::dec << "\n";
        else
            oss << "  " << symbols[i] << "\n";
    }
    free(symbols);
    std::string dump = oss.str();
    WriteToFile(dump);
    ShowErrorDialog("Crash Detected", dump);
    _exit(1);
}
#endif

static bool StrToLongHex(const std::string& aString, DWORD* theValue)
{	
	*theValue = 0;

	for (int i = 0; i < (int)aString.length(); i++)
	{
		char aChar = aString[i];

		int aValue;
		if ((aChar >= '0') && (aChar <= '9'))
			aValue = aChar - '0';
		else if ((aChar >= 'A') && (aChar <= 'F'))
			aValue = (aChar - 'A') + 10;
		else if ((aChar >= 'a') && (aChar <= 'f'))
			aValue = (aChar - 'a') + 10;
		else
			return false;

		*theValue += aValue << ((aString.length()-i-1)*4);
	}

	return true;
}

void SEHCatcher::GetSymbolsFromMapFile(std::string &theDebugDump)
{
#ifdef _WIN32
    DWORD aTick = GetTickCount();
	WIN32_FIND_DATAA aFindData;
	HANDLE aFindHandle = FindFirstFileA("*.map",&aFindData);
	if (aFindHandle==INVALID_HANDLE_VALUE)
		return;

	FindClose(aFindHandle);

	typedef std::pair<DWORD,DWORD> SymbolPair;
	typedef std::map<SymbolPair,std::string> SymbolMap;
	typedef std::pair<std::string,int> LineNumPair;
	typedef std::map<SymbolPair,LineNumPair> LineNumMap;

/**/
	SymbolMap aSymbolMap;
	LineNumMap aLineNumMap;
	std::fstream aFile(aFindData.cFileName, std::ios::in);

	if (!aFile.is_open())
		return;

	// Parse map file
	std::string aCurLinesFile;
	while (!aFile.eof())
	{
		char aStr[4096];
		aFile.getline(aStr, 4096);

		std::string aString = aStr;

		if ((aString.length() > 22) && (aString[5] == ':'))
		{
			std::string aFoundPreStr = aString.substr(1, 4);
			std::string aFoundPostStr = aString.substr(6, 8);

			DWORD aFoundPreVal;
			DWORD aFoundPostVal;

			if (StrToLongHex(aFoundPreStr, &aFoundPreVal) && StrToLongHex(aFoundPostStr, &aFoundPostVal))
			{
				int aSpacePos = aString.find(' ', 21);

				if ((aString[20] == ' ') && (aSpacePos >= 0))
					aSymbolMap[SymbolPair(aFoundPreVal,aFoundPostVal)] =  aString.substr(21, aSpacePos-21);
			}
		}
		else if (strcmp(aString.substr(0, strlen("Line numbers")).c_str(), "Line numbers") == 0)
		{
			int aSegmentPos = aString.rfind(')');
			if (aSegmentPos == -1)
				aSegmentPos = aString.length();

			int aPreLen = strlen("Line numbers for ");

			int aStartPos = aString.find('(');

			aCurLinesFile = aString.substr(aStartPos + 1, aSegmentPos - aStartPos - 1);
		}
		else if ((aCurLinesFile.length() > 0) && (aString.length() == 80))
		{	
			// Line number stuff

			for (int i = 0; i < 4; i++)
			{
				int aStartLoc = 20 * i;

				int aLine = atoi(aString.substr(aStartLoc, 6).c_str());

				std::string aFoundPreStr = aString.substr(aStartLoc + 7, 4);
				std::string aFoundPostStr = aString.substr(aStartLoc + 12, 8);

				DWORD aFoundPreVal;
				DWORD aFoundPostVal;

				if (StrToLongHex(aFoundPreStr, &aFoundPreVal) && StrToLongHex(aFoundPostStr, &aFoundPostVal))
				{
					aLineNumMap[SymbolPair(aFoundPreVal,aFoundPostVal)] = LineNumPair(aCurLinesFile,aLine);
				}
			}
		}
    }

	// Parse stack trace
	for (int i = 0; i < (int)theDebugDump.length(); i++)
	{
		if (theDebugDump.at(i) == ':')
		{
			std::string aFindPreStr = theDebugDump.substr(i-4, 4);
			std::string aFindPostStr = theDebugDump.substr(i+1, 8);

			DWORD aFindPreVal;
			DWORD aFindPostVal;

			if (StrToLongHex(aFindPreStr, &aFindPreVal) && StrToLongHex(aFindPostStr, &aFindPostVal))
			{

				int aBestDist = -1;
				SymbolMap::iterator aSymbolItr  = aSymbolMap.lower_bound(SymbolPair(aFindPreVal,aFindPostVal));
				if (aSymbolItr!=aSymbolMap.end() && aSymbolItr!=aSymbolMap.begin() && aSymbolItr->first.second!=aFindPostVal)
					--aSymbolItr;

				if (aSymbolItr!=aSymbolMap.end() && aSymbolItr->first.first==aFindPreVal)
					aBestDist = aFindPostVal - aSymbolItr->first.second;

				if (aBestDist != -1)
				{
					std::string &aBestName = aSymbolItr->second;

					char aSymbolName[4096];

					if (mUnDecorateSymbolName(aBestName.c_str(), aSymbolName, 4096, 
						UNDNAME_NO_ALLOCATION_MODEL | UNDNAME_NO_ACCESS_SPECIFIERS | 
						UNDNAME_NO_THROW_SIGNATURES | UNDNAME_NO_MEMBER_TYPE) == 0)
						strcpy(aSymbolName, aBestName.c_str());

					if (aBestDist != 0)
					{
						sprintf(aSymbolName+strlen(aSymbolName), "+0x%X", aBestDist);
					}

					std::string aNewText = aSymbolName;

					LineNumMap::iterator aLineNumItr = aLineNumMap.lower_bound(SymbolPair(aFindPreVal,aFindPostVal));
					if (aLineNumItr!=aLineNumMap.end() && aLineNumItr!=aLineNumMap.begin() && aLineNumItr->first.second!=aFindPostVal)
						--aLineNumItr;

					if (aLineNumItr!=aLineNumMap.end() && aLineNumItr->first.first==aFindPreVal)
					{							
						std::string &aBestFile = aLineNumItr->second.first;
						int aBestLine = aLineNumItr->second.second;
						int aBestLineDist =  aFindPostVal - aLineNumItr->first.second;

						char aDistStr[4096];
						sprintf(aDistStr, "\r\n    %s line %d +0x%X", aBestFile.c_str(), aBestLine, aBestLineDist);
						aNewText += aDistStr;
					}

					theDebugDump.erase(i-4, 13);
					theDebugDump.insert(i-4, aNewText.c_str());
				}
			}
		}
	}

//	MessageBox(NULL,StrFormat("%d",GetTickCount()-aTick).c_str(),"Time",MB_OK);
#endif
}

#ifdef _WIN32


std::string SEHCatcher::IntelWalk(PCONTEXT theContext, int theSkipCount)
{
	std::string aDebugDump;
	char aBuffer[2048];


#ifdef _WIN64
	DWORD64 pc = theContext->Eip; 
	PDWORD64 pFrame, pPrevFrame; 

	pFrame = (PDWORD64)theContext->Ebp;
#else
	DWORD pc = theContext->Eip;
	PDWORD pFrame, pPrevFrame;

	pFrame = (PDWORD)theContext->Ebp;
#endif

    for (;;)
    {
        char szModule[MAX_PATH] = "";
        DWORD section = 0, offset = 0;

        GetLogicalAddress((PVOID)pc, szModule, sizeof(szModule), section, offset);

        sprintf(aBuffer, "%08X  %08X  %04X:%08X %s\r\n",
                  pc, pFrame, section, offset, GetFilename(szModule).c_str());
		aDebugDump += aBuffer;

		if (pFrame == nullptr)
			break;

#ifdef _WIN64
		pc = pFrame[1];
		pPrevFrame = pFrame;
		pFrame = (PDWORD64)pFrame[0];// proceed to next higher frame on stack
#else
		pc = pFrame[1]; 
		pPrevFrame = pFrame;
		pFrame = (PDWORD)pFrame[0];
#endif


#ifdef _WIN64		
		if ((DWORD64)pFrame & 3)    // Frame pointer must be aligned on a
			break;                     // Bail if not aligned.
#else
		if ((DWORD)pFrame & 3)    // Frame pointer must be aligned on a
			break;                  // DWORD boundary.  Bail if not so.
#endif

        if (pFrame <= pPrevFrame)
            break;

        // Can two DWORDs be read from the supposed frame address?          
        if (IsBadWritePtr(pFrame, sizeof(PVOID)*2))
            break;
    };

	return aDebugDump;
}


void SEHCatcher::DoHandleDebugEvent(LPEXCEPTION_POINTERS lpEP)
{
	bool hasImageHelp = LoadImageHelp();

	std::string anErrorTitle;
	std::string aDebugDump;

	char aBuffer[2048];
   
    ///////////////////////////
	// first name the exception	
	const char  *szName = NULL;
    for (int i=0; gMsgTable[i].dwExceptionCode != 0xFFFFFFFF; i++) 
	{
        if (gMsgTable[i].dwExceptionCode == lpEP->ExceptionRecord->ExceptionCode) 
		{
            szName = gMsgTable[i].szMessage;
            break;
        }
    }	

    if (szName != NULL)
	{		
		sprintf(aBuffer,"Exception: %s (code 0x%x) at address %08X in thread %X\r\n",
				szName, lpEP->ExceptionRecord->ExceptionCode, 
				lpEP->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());
	}
	else
	{
		sprintf(aBuffer,"Unknown exception: (code 0x%x) at address %08X in thread %X\r\n",
				lpEP->ExceptionRecord->ExceptionCode, 
				lpEP->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());
	}

	aDebugDump += aBuffer;	

	///////////////////////////////////////////////////////////
	// Get logical address of the module where exception occurs
	DWORD section, offset;
	GetLogicalAddress(lpEP->ExceptionRecord->ExceptionAddress, aBuffer, sizeof(aBuffer), section, offset);
	aDebugDump += "Module: " + GetFilename(aBuffer) + "\r\n";		
	sprintf(aBuffer, "Logical Address: %04X:%08X\r\n", section, offset);
	aDebugDump += aBuffer;	
	
	aDebugDump += "\r\n";	

	anErrorTitle = StrFormat("Exception at %04X:%08X", section, offset);

	std::string aWalkString = "";	

	if (hasImageHelp)
		aWalkString = ImageHelpWalk(lpEP->ContextRecord, 0);

	if (aWalkString.length() == 0)
		aWalkString = IntelWalk(lpEP->ContextRecord, 0);

    aDebugDump += aWalkString;

	aDebugDump += "\r\n";
	sprintf(aBuffer, ("EAX:%08X EBX:%08X ECX:%08X EDX:%08X ESI:%08X EDI:%08X\r\n"),
            lpEP->ContextRecord->Eax, lpEP->ContextRecord->Ebx, lpEP->ContextRecord->Ecx, lpEP->ContextRecord->Edx, lpEP->ContextRecord->Esi, lpEP->ContextRecord->Edi);
	aDebugDump += aBuffer;
    sprintf(aBuffer, "EIP:%08X ESP:%08X  EBP:%08X\r\n", lpEP->ContextRecord->Eip, lpEP->ContextRecord->Esp, lpEP->ContextRecord->Ebp);	
	aDebugDump += aBuffer;
    sprintf(aBuffer, "CS:%04X SS:%04X DS:%04X ES:%04X FS:%04X GS:%04X\r\n", lpEP->ContextRecord->SegCs, lpEP->ContextRecord->SegSs, lpEP->ContextRecord->SegDs, lpEP->ContextRecord->SegEs, lpEP->ContextRecord->SegFs, lpEP->ContextRecord->SegGs );
	aDebugDump += aBuffer;
    sprintf(aBuffer, "Flags:%08X\r\n", lpEP->ContextRecord->EFlags );
	aDebugDump += aBuffer;

	aDebugDump += "\r\n";
	aDebugDump += GetSysInfo();	
    if (mApp != NULL)
	{
		std::string aGameSEHInfo = mApp->GetGameSEHInfo();
		if (aGameSEHInfo.length() > 0)
		{
			aDebugDump += "\r\n";
			aDebugDump += aGameSEHInfo;
		}
		mApp->CopyToClipboard(aDebugDump);
	}		

    // just for any case
    // some time we can't go through GetSymbolsFromMapFile, probably because of some memory corruption
    WriteToFile(aDebugDump);

	if (hasImageHelp)
		GetSymbolsFromMapFile(aDebugDump);
    
    // rewrite crash file
	WriteToFile(aDebugDump);

	if (mShowUI)
		ShowErrorDialog(anErrorTitle, aDebugDump);

	//::MessageBox(NULL, aDebugDump.c_str(), "ERROR", MB_ICONERROR);	

	UnloadImageHelp();	
}

bool SEHCatcher::LoadImageHelp()
{
	mImageHelpLib = LoadLibraryA("IMAGEHLP.DLL");
    if (!mImageHelpLib)
        return false;

    mSymInitialize = (SYMINITIALIZEPROC) GetProcAddress(mImageHelpLib, "SymInitialize");
    if (!mSymInitialize)
        return false;

	mSymSetOptions = (SYMSETOPTIONSPROC) GetProcAddress(mImageHelpLib, "SymSetOptions");
    if (!mSymSetOptions)
        return false;

    mSymCleanup = (SYMCLEANUPPROC) GetProcAddress(mImageHelpLib, "SymCleanup");
    if (!mSymCleanup)
        return false;

	mUnDecorateSymbolName = (UNDECORATESYMBOLNAMEPROC) GetProcAddress(mImageHelpLib, "UnDecorateSymbolName");
    if (!mUnDecorateSymbolName)
        return false;

    mStackWalk = (STACKWALKPROC) GetProcAddress(mImageHelpLib, "StackWalk");
    if (!mStackWalk)
        return false;

    mSymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC) GetProcAddress(mImageHelpLib, "SymFunctionTableAccess");
    if (!mSymFunctionTableAccess)
        return false;

    mSymGetModuleBase = (SYMGETMODULEBASEPROC) GetProcAddress(mImageHelpLib, "SymGetModuleBase");
    if (!mSymGetModuleBase)
        return false;

    mSymGetSymFromAddr = (SYMGETSYMFROMADDRPROC) GetProcAddress(mImageHelpLib, "SymGetSymFromAddr" );
    if (!mSymGetSymFromAddr)
        return false;    

	mSymSetOptions(SYMOPT_DEFERRED_LOADS);

	// Get image filename of the main executable
	char filepath[MAX_PATH], *lastdir, *pPath;
	DWORD filepathlen = GetModuleFileNameA ( NULL, filepath, sizeof(filepath));
        
    lastdir = strrchr (filepath, '/');
    if (lastdir == NULL) lastdir = strrchr (filepath, '\\');
    if (lastdir != NULL) lastdir[0] = '\0';

    // Initialize the symbol table routines, supplying a pointer to the path
    pPath = filepath;
    if (strlen (filepath) == 0) pPath = NULL;

     if (!mSymInitialize (GetCurrentProcess(), pPath, TRUE))
		return false;

    return true;
}

void SEHCatcher::UnloadImageHelp()
{
	if (mImageHelpLib != NULL)
		FreeLibrary(mImageHelpLib);
}


std::string SEHCatcher::ImageHelpWalk(PCONTEXT theContext, int theSkipCount)
{
	char aBuffer[2048];
	std::string aDebugDump;

	STACKFRAME sf;
	memset( &sf, 0, sizeof(sf) );	

	// Initialize the STACKFRAME structure for the first call.  This is only
	// necessary for Intel CPUs, and isn't mentioned in the documentation.
	sf.AddrPC.Offset       = theContext->Eip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = theContext->Esp;
	sf.AddrStack.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset    = theContext->Ebp;
	sf.AddrFrame.Mode      = AddrModeFlat;
	
	int aLevelCount = 0;

	for (;;)
	{
		if (!mStackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(),
						&sf, NULL  /*theContext*/, NULL, mSymFunctionTableAccess, mSymGetModuleBase, 0))
		{
			DWORD lastErr = GetLastError();
			sprintf(aBuffer, "StackWalk failed (error %d)\r\n", lastErr);
			aDebugDump += aBuffer;
			break;
		}

		if ((sf.AddrFrame.Offset == 0) || (sf.AddrPC.Offset == 0))
			break;

		if (theSkipCount > 0)
		{
			theSkipCount--;
			continue;
		}

		BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
		PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = 512;
			
		DWORD symDisplacement = 0;  // Displacement of the input address,
									// relative to the start of the symbol
			
		if (mSymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &symDisplacement, pSymbol))
		{
			char aUDName[256];
			mUnDecorateSymbolName(pSymbol->Name, aUDName, 256, 
								 UNDNAME_NO_ALLOCATION_MODEL | UNDNAME_NO_ALLOCATION_LANGUAGE | 
								 UNDNAME_NO_MS_THISTYPE | UNDNAME_NO_ACCESS_SPECIFIERS | 
								 UNDNAME_NO_THISTYPE | UNDNAME_NO_MEMBER_TYPE | 
								 UNDNAME_NO_RETURN_UDT_MODEL | UNDNAME_NO_THROW_SIGNATURES |
								 UNDNAME_NO_SPECIAL_SYMS);
				
			sprintf(aBuffer, "%08X %08X %hs+%X\r\n", 
					sf.AddrFrame.Offset, sf.AddrPC.Offset, aUDName, symDisplacement);
		}
		else // No symbol found.  Print out the logical address instead.
		{
			char szModule[MAX_PATH];            
			DWORD section = 0, offset = 0;

			GetLogicalAddress((PVOID)sf.AddrPC.Offset, szModule, sizeof(szModule), section, offset);				
			sprintf(aBuffer, "%08X %08X %04X:%08X %s\r\n", sf.AddrFrame.Offset, sf.AddrPC.Offset, section, offset, GetFilename(szModule).c_str());
		}
		aDebugDump += aBuffer;
		
		sprintf(aBuffer, "Params: %08X %08X %08X %08X\r\n", sf.Params[0], sf.Params[1], sf.Params[2], sf.Params[3]);
		aDebugDump += aBuffer;		
		aDebugDump += "\r\n";

		aLevelCount++;
    
        // check for loop
        if (aLevelCount > 1000)
            break;
	}

	return aDebugDump;
}


#endif

bool SEHCatcher::GetLogicalAddress(void* addr, char* szModule, DWORD len, DWORD& section, DWORD& offset)
{
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;

    if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
        return false;

    DWORD hMod = (DWORD)mbi.AllocationBase;

    if (!GetModuleFileNameA((HMODULE) hMod, szModule, len))
        return false;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER) hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS) (hMod + pDosHdr->e_lfanew);

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);

    DWORD rva = (DWORD) addr - hMod; // RVA is offset from module load address

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (unsigned i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++)
    {
        DWORD sectionStart = pSection->VirtualAddress;
        DWORD sectionEnd = sectionStart + std::max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

        // Is the address in this section???
        if ((rva >= sectionStart) && (rva <= sectionEnd))
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = rva - sectionStart;
            return true;
        }
    }
#endif

    return false; // Should never get here!
}

std::string SEHCatcher::GetFilename(const std::string& thePath)
{
	int aLastSlash = std::max((int) thePath.rfind('\\'), (int) thePath.rfind('/'));	

	if (aLastSlash >= 0)
	{
		return thePath.substr(aLastSlash+1);
	}
	else
		return thePath;
}

int aCount = 0;

void SEHCatcher::SEHWindowProc()
{
	SDL_Event e;
    while (SDL_PollEvent(&e))
	{
        if (e.type == SDL_EVENT_QUIT)
            mExiting = true;
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}

void SEHCatcher::WriteToFile(const std::string& theErrorText)
{
    std::fstream aStream("crash.txt", std::ios::out);
	aStream << theErrorText.c_str() << std::endl;
}

void SEHCatcher::SubmitReportThread(void *theArg)
{
	std::string aSeperator = "---------------------------7d3e1f30eec";
	
	DefinesMap aSEHWebParams;

	mApp->GetSEHWebParams(&aSEHWebParams);

	std::string aContent;

	DefinesMap::iterator anItr = aSEHWebParams.begin();
	while (anItr != aSEHWebParams.end())
	{
		aContent += "--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"" + anItr->first + "\"\r\n" +		
		"\r\n" +
		anItr->second + "\r\n";
		
		++anItr;
	}

	aContent += 
		
		/*"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"username\"\r\n" +		
		"\r\n" +
		mApp->mUserName + "\r\n" +	*/

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"prod\"\r\n" +		
		"\r\n" +
		mApp->mProdName + "\r\n" +	

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"version\"\r\n" +		
		"\r\n" +
		mApp->mProductVersion + "\r\n" +

		/*"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"buildnum\"\r\n" +		
		"\r\n" +
		StrFormat("%d", mApp->mBuildNum) + "\r\n" +

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"builddate\"\r\n" +		
		"\r\n" +
		mApp->mBuildDate + "\r\n" +

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"referid\"\r\n" +		
		"\r\n" +
		mApp->mReferId + "\r\n" +*/

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"usertext\"\r\n" +
		"\r\n" +
		mUserText + "\r\n" +

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"errortitle\"\r\n" +		
		"\r\n" +
		mErrorTitle + "\r\n" +

		"--" + aSeperator + "\r\n"
		"Content-Disposition: form-data; name=\"errortext\"\r\n" +		
		"\r\n" +
		mErrorText + "\r\n";

	if (mHasDemoFile)
	{
		Buffer aBuffer;
		mApp->ReadBufferFromFile(mUploadFileName, &aBuffer);

		aContent += 
			"--" + aSeperator + "\r\n"
			"Content-Disposition: form-data; name=\"demofile\"; filename=\"popcap.dmo\"\r\n" +
			"Content-Type: application/octet-stream\r\n" +
			"\r\n";		

		aContent.insert(aContent.end(), (char*) aBuffer.GetDataPtr(), (char*) aBuffer.GetDataPtr() + aBuffer.GetDataLen());			
				
		aContent += "\r\n";
	}

	aContent +=
		"--" + aSeperator + "--\r\n";

	std::string aSendString = 
		"POST /deluxe_error.php HTTP/1.1\r\n"
		"Content-Type: multipart/form-data; boundary=" + aSeperator + "\r\n"
		"User-Agent: Mozilla/4.0 (compatible; popcap)\r\n" +
		"Host: " + mIssueWebsite + "\r\n" +
		"Content-Length: " + StrFormat("%d", aContent.length()) + "\r\n" +
		"Connection: close\r\n" +
		"\r\n" + aContent;
	
	mSubmitReportTransfer.SendRequestString(mIssueWebsite, aSendString);
}

void SEHCatcher::ShowErrorDialog(const std::string& theErrorTitle, const std::string& theErrorText)
{
    mExiting = false;
    mErrorTitle = theErrorTitle;
    mErrorText  = theErrorText;

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string narrowStr = converter.to_bytes(mCrashMessage);
	const char* cstr = narrowStr.c_str();

	SDL_Init(SDL_INIT_VIDEO);
    mWindow = SDL_CreateWindow(mErrorTitle.c_str(),
        600, 400,
        0);
    mRenderer = SDL_CreateRenderer(mWindow, NULL);

    IMGUI_CHECKVERSION();
	mImGuiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(mImGuiContext);
	ImGuiIO &io = ImGui::GetIO();
	(void)io; // uhhhhhh
	ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(mWindow, mRenderer);
    ImGui_ImplSDLRenderer3_Init(mRenderer);

	SDL_SetClipboardText(cstr); // we're not lying.

    while (!mExiting)
    {
        SEHWindowProc();

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        int windowWidth;
        int windowHeight;
		SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);

        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("Fatal Error!", nullptr,
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

        ImGui::TextWrapped("%s", cstr);
        ImGui::Spacing();

        float contentHeight = ImGui::GetContentRegionAvail().y;
        float logHeight = 180.0f;
        float buttonHeight = 30.0f;
        float spacingE = 10.0f;
        float reservedBottom = logHeight + buttonHeight + spacingE * 3;
        ImGui::Dummy(ImVec2(0.0f, contentHeight - reservedBottom));

        ImGui::BeginChild("ErrorDetails", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextWrapped("%s", mErrorText.c_str());
        ImGui::PopFont();
        ImGui::EndChild();

        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        int numButtons = 1;
    #if defined(_DEBUG) || !defined(NDEBUG)
        numButtons += 1;
    #endif
        numButtons += 1;

        float totalWidth = numButtons * buttonWidth + (numButtons - 1) * spacing;
        float availWidth = ImGui::GetContentRegionAvail().x;
        float startX = (availWidth - totalWidth) * 0.5f;
        ImGui::SetCursorPosX(startX);

		if (ImGui::Button("Send Issue", ImVec2(buttonWidth, 0)))
		{
			if (!mApp || !mApp->OpenURL(mIssueWebsite))
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter2;
				std::string narrowStr2 = converter.to_bytes(mSubmitErrorMessage);
				const char* cstr2 = narrowStr2.c_str();

				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", cstr2, mWindow);
			}
			
			mExiting = true;
        }
#if defined(_DEBUG) || !defined(NDEBUG)
        ImGui::SameLine();
        if (ImGui::Button("Debug", ImVec2(buttonWidth, 0)))
		{
			mDebugError = true;
			mExiting = true;
        }
#endif
        ImGui::SameLine();
        if (ImGui::Button("Close Now", ImVec2(buttonWidth, 0)))
		{
            mExiting = true;
        }

        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(mRenderer, 240, 240, 240, 255);
        SDL_RenderClear(mRenderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
        SDL_RenderPresent(mRenderer);

        SDL_Delay(16);
    }
	
    try { ImGui_ImplSDLRenderer3_Shutdown(); } catch(...) {}
    try { ImGui_ImplSDL3_Shutdown(); }        catch(...) {}
    try { ImGui::DestroyContext(mImGuiContext); }          catch(...) {}
    try { if (mRenderer) SDL_DestroyRenderer(mRenderer); } catch(...) {}
    try { if (mWindow)   SDL_DestroyWindow(mWindow);   } catch(...) {}

	if (mExiting && !mDebugError)
		mApp->Shutdown();
}

std::string SEHCatcher::GetSysInfo()
{
	std::string aDebugDump;

#ifdef _WIN32
	OSVERSIONINFOA aVersionInfo;
	aVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExA(&aVersionInfo); 

	aDebugDump += "Windows Ver: ";
	if (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		aDebugDump += "NT ";
	else
		aDebugDump += "9x ";

	char aVersionStr[20];
	sprintf(aVersionStr, "%d.%d", aVersionInfo.dwMajorVersion, aVersionInfo.dwMinorVersion);
	aDebugDump += aVersionStr;
	aDebugDump += " ";
	aDebugDump += aVersionInfo.szCSDVersion;
	aDebugDump += " ";
	sprintf(aVersionStr, "%d", aVersionInfo.dwBuildNumber);
	aDebugDump += "Build ";
	aDebugDump += aVersionStr;
	aDebugDump += "\r\n";
#else
	aDebugDump += "Linux Ver: Unknown";
#endif

	char aPath[256];
	char aSDLStr[20];
	char aPopLibStr[40];
	char aOpenALStr[256];
	char aBassStr[20];
	if (mApp != NULL)
	{
		sprintf(aPopLibStr, "PopLib Ver: %s", POPLIB_VERSION);
		aDebugDump += aPopLibStr;
		aDebugDump += "\r\n";	

		sprintf(aSDLStr, "SDL Ver: %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
		aDebugDump += aSDLStr;
		aDebugDump += "\r\n";

		const char* version = alGetString(AL_VERSION);
		const char* renderer = alGetString(AL_RENDERER);
		const char* vendor = alGetString(AL_VENDOR);
		sprintf(aOpenALStr, "OpenAL Ver: %s \r\nOpenAL Renderer: %s \r\nOpenAL Vendor: %s", version, renderer, vendor);
		aDebugDump += aOpenALStr;
		aDebugDump += "\r\n";

		sprintf(aBassStr, "Bass Ver: %s", BASSVERSIONTEXT);
		aDebugDump += aBassStr;
		aDebugDump += "\r\n";	
	}	

	return aDebugDump;
}